// #pragma once
#include "avatarmk.hpp"
#include "atomicassets.hpp"

namespace avatarmk {

    avatar_mint_fee avatarmk_c::calculate_mint_price(const avatars& avatar, const config& cfg)
    {
        //this is untested code!!!!!!!!!!
        //still need to thinker more
        avatar_mint_fee result;
        // int core_precision_pow = pow(10, core_symbol.precision());
        const auto sec_passed = (eosio::current_time_point() - avatar.modified).to_seconds();
        const auto days_passed = std::floor(sec_passed / day_sec);

        //calculate next base price
        if (days_passed == 0) {
            //base price increases by 10% each time a mint happens before a day passes from the previous mint
            double nbp = avatar.base_price.amount * 1.10;
            result.next_base_price = {static_cast<int64_t>(nbp), core_symbol};
        }
        else {
            result.next_base_price = cfg.floor_mint_price * avatar.rarity;
        }

        //calculate mint fee based on current base price
        const uint64_t pv = avatar.base_price.amount;
        const double r = 0.01 * (5 / avatar.rarity);                     //rare avatars will decay slower
        const auto decay_step = days_passed <= 7 ? 0 : days_passed - 7;  //start decay after 1 week
        const uint64_t p = (uint64_t)(pv * pow(1 - r, decay_step));
        eosio::asset mint_price{static_cast<int64_t>(p), core_symbol};
        mint_price = std::max(mint_price, cfg.floor_mint_price);  //mint price can't be smaller than floor
        result.fee = {mint_price, extended_core_symbol.get_contract()};

        return result;
    }

    void avatarmk_c::sub_balance(const eosio::name& owner, const eosio::extended_asset& value)
    {
        deposits_table _deposits(get_self(), owner.value);
        auto idx = _deposits.get_index<"bycontrsym"_n>();
        const uint128_t composite_id = (uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
        const auto& itr = idx.get(composite_id, "No balance with this symbol and contract.");
        eosio::check(itr.balance >= value, "Overdrawn balance. " + owner.to_string() + " only owns " + itr.balance.to_string());

        if (owner != get_self() && itr.balance == value) {
            //auto close account if balance is zero
            _deposits.erase(itr);
            return;
        }

        _deposits.modify(itr, eosio::same_payer, [&](auto& n) {
            n.balance -= value;  //
        });
    }

    void avatarmk_c::add_balance(const eosio::name& owner, const eosio::extended_asset& value, const eosio::name& ram_payer)
    {
        deposits_table _deposits(get_self(), owner.value);
        auto idx = _deposits.get_index<"bycontrsym"_n>();
        const uint128_t composite_id = (uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
        auto itr = idx.find(composite_id);

        if (itr == idx.end()) {
            //new table entry
            eosio::check(ram_payer.value, owner.to_string() + " doesn't have a deposit account for this token yet. Use open action first.");
            _deposits.emplace(ram_payer, [&](auto& n) {
                n.id = _deposits.available_primary_key();
                n.balance = value;
            });
        }
        else {
            idx.modify(itr, eosio::same_payer, [&](auto& n) {
                n.balance += value;  //
            });
        }
    }

    std::optional<assemble_set> avatarmk_c::validate_assemble_set(std::vector<uint64_t> asset_ids, eosio::name owner, eosio::name collection_name, eosio::name schema_name)
    {
        assemble_set result;
        std::vector<uint8_t> rarities;
        std::vector<std::string> test_types;

        auto receiver_assets = atomicassets::get_assets(owner);

        auto collection_schemas = atomicassets::get_schemas(collection_name);
        auto schema = collection_schemas.get(schema_name.value, "Schema with name not found");
        auto templates = atomicassets::get_templates(collection_name);

        for (uint64_t asset_id : asset_ids) {
            auto asset = receiver_assets.get(asset_id, "Asset not found");
            auto t = templates.get(asset.template_id, "Template not found");
            auto des_data = atomicassets::deserialize(t.immutable_serialized_data, schema.format);

            auto body_type = std::get<std::string>(des_data["bodypart"]);  //type

            if (std::find(test_types.begin(), test_types.end(), body_type) != test_types.end()) {
                eosio::check(false, "Duplicate body part type " + body_type);
            }
            else {
                result.template_ids.push_back(asset.template_id);
                rarities.push_back(get<uint8_t>(des_data["rarityScore"]));
            }
        }

        //there must be 8 unique body part types
        if (result.template_ids.size() == 8) {
            sort(result.template_ids.begin(), result.template_ids.end());
            auto packed_parts = eosio::pack(result.template_ids);
            std::string sdata = std::string(packed_parts.begin(), packed_parts.end());

            result.identifier = eosio::sha256(sdata.c_str(), sdata.length());
            result.rarity_score = std::floor(std::accumulate(rarities.begin(), rarities.end(), 0) / 8);

            return result;
        }
        return std::nullopt;
    };

    //
    void avatarmk_c::burn_nfts(std::vector<uint64_t>& asset_ids)
    {
        for (auto asset_id : asset_ids) {
            const auto data = std::make_tuple(get_self(), asset_id);
            eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "burnasset"_n, data).send();
        }
    };

    void avatarmk_c::validate_avatar_name(std::string& avatar_name)
    {
        //implement blacklist here?
        int l = avatar_name.size();
        eosio::check(l >= 3, "Avatar name must have a minimum of 3 characters");
        eosio::check(l <= 20, "Avatar name can't have more then 20 characters");
    };

}  // namespace avatarmk
// #pragma once
// #include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include "avatarmk.hpp"

namespace avatarmk {
    void avatarmk_c::assemble(const eosio::name& creator, std::vector<int32_t>& part_template_ids)
    {
        //validate set (loop through vector)

        //calculate avatar identifier
        sort(part_template_ids.begin(), part_template_ids.end());
        auto packed_parts = eosio::pack(part_template_ids);
        std::string sdata = std::string(packed_parts.begin(), packed_parts.end());
        eosio::checksum256 identifier = eosio::sha256(sdata.c_str(), sdata.length());

        //init avatars table
        avatars_table _avatars(get_self(), get_self().value);
        auto idx = _avatars.get_index<eosio::name("byidf")>();

        //check if identifier already exists
        eosio::check(idx.find(identifier) == idx.end(), "Avatar with these body parts already available to mint.");

        //calculate rarity & max_mint

        //add new avatar to table
        _avatars.emplace(get_self(), [&](auto& n) {
            n.id = _avatars.available_primary_key();
            n.template_id = 1;
            n.creator = creator;
            n.identifier = identifier;
            n.ipfs_hash = "hashhashhahshash";
            n.rarity = 5;  //1-5
            n.mint = 0;
            n.max_mint = 0;
        });
    }

    eosio::extended_asset avatarmk_c::calculate_mint_price(const avatars& avatar)
    {
        config_table _config(get_self(), get_self().value);
        auto cfg = _config.get_or_create(get_self(), config());
        int core_precision_pow = pow(10, core_symbol.precision());
        //this is subject to change
        double p = (cfg.base_mint_price.amount * avatar.rarity) / avatar.mint;
        eosio::asset mint_price{static_cast<int64_t>(p * core_precision_pow), core_symbol};
        eosio::extended_asset extended_mint_price(mint_price, extended_core_symbol.get_contract());
        return extended_mint_price;
    }

    void avatarmk_c::sub_balance(const eosio::name& owner, const eosio::extended_asset& value)
    {
        deposits_table _deposits(get_self(), owner.value);
        auto idx = _deposits.get_index<"bycontrsym"_n>();
        uint128_t composite_id = (uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
        const auto& itr = idx.get(composite_id, "No balance with this symbol and contract.");
        eosio::check(itr.balance >= value, "Overdrawn balance. " + owner.to_string() + " only owns " + value.to_string());

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
        uint128_t composite_id = (uint128_t{value.contract.value} << 64) | value.quantity.symbol.raw();
        auto itr = idx.find(composite_id);

        if (itr == idx.end()) {
            //warning dust attack possible need to assert here
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

}  // namespace avatarmk
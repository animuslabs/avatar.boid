#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
//https://github.com/EOSIO/eosio.cdt/blob/master/libraries/eosiolib/core/eosio/crypto.hpp
// #include <eosio/crypto.hpp>
#include <eosio/bytes.hpp>
#include <token/token.hpp>
#include "avatarmk.hpp"
#include "functions.cpp"
#include "notification_handlers.cpp"
namespace avatarmk {

    void avatarmk_c::setconfig(std::optional<config> cfg)
    {
        require_auth(get_self());
        config_table _config(get_self(), get_self().value);
        cfg ? _config.set(cfg.value(), get_self()) : _config.remove();
    }

    void avatarmk_c::mintavatar(eosio::name& minter, uint64_t& avatar_id)
    {
        require_auth(minter);
        avatars_table _avatars(get_self(), get_self().value);
        auto itr = _avatars.require_find(avatar_id, "Avatar with this id doesn't exist.");
        eosio::check(itr->template_id > 0 && !itr->ipfs_hash.empty(), "Avatar not finalized yet. Try again later.");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        //for now 100% of fee goes to template owner
        avatar_mint_fee amf = calculate_mint_price(*itr, cfg);  //not needed to pass in full config
        sub_balance(minter, amf.fee);
        add_balance(itr->creator, amf.fee, get_self());  //let self pay for ram if new table entry?

        _avatars.modify(itr, eosio::same_payer, [&](auto& n) {
            n.mint += 1;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
            n.base_price = amf.next_base_price;
        });
        //atomic mint action
        const auto blank_data = atomicassets::ATTRIBUTE_MAP{};
        const std::vector<eosio::asset> tokens_to_back;
        const auto data = make_tuple(get_self(), cfg.collection_name, cfg.avatar_schema, itr->template_id, minter, blank_data, blank_data, tokens_to_back);
        eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "mintasset"_n, data).send();
    }

    void avatarmk_c::assemble(const eosio::name& creator, assemble_set& set_data, std::string& avatar_name)
    {
        require_auth(get_self());

        avatars_table _avatars(get_self(), get_self().value);
        auto idx = _avatars.get_index<eosio::name("byidf")>();

        eosio::check(idx.find(set_data.identifier) == idx.end(), "Avatar with these body parts already available to mint.");

        //todo: this function must catch abusive words
        validate_avatar_name(avatar_name);

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        //add new avatar to table
        _avatars.emplace(get_self(), [&](auto& n) {
            n.id = _avatars.available_primary_key();
            n.avatar_name = avatar_name;
            n.rarity = set_data.rarity_score;
            n.creator = creator;
            n.identifier = set_data.identifier;
            n.base_price = cfg.floor_mint_price * set_data.rarity_score;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
            n.bodyparts = set_data.template_ids;
            //this data needs to be added by the server in response to the assemble action
            // n.template_id
            // n.ipfs_hash;
            // n.max_mint = 0; //this can be calculated in the contract and added to set_data
        });
    }
    void avatarmk_c::finalize(eosio::checksum256& identifier, std::string& ipfs_hash, uint32_t& template_id)
    {
        require_auth(get_self());
        avatars_table _avatars(get_self(), get_self().value);
        auto idx = _avatars.get_index<eosio::name("byidf")>();
        auto itr = idx.require_find(identifier, "Identifier doesn't exists in avatars table.");

        idx.modify(itr, eosio::same_payer, [&](auto& n) {
            n.template_id = template_id;
            n.ipfs_hash = ipfs_hash;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
        });
    }

    void avatarmk_c::withdraw(const eosio::name& owner, const eosio::extended_asset& value)
    {
        require_auth(owner);
        eosio::check(value.quantity.amount > 0, "Withdraw amount must be possitive.");
        sub_balance(owner, value);
        token::actions::transfer{value.contract, get_self()}.send(get_self(), owner, value.quantity, "withdraw");
    }

    void avatarmk_c::open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer)
    {
        //can make ram_payer optional
        require_auth(ram_payer);
        eosio::check(is_account(owner), "Owner isn't associated with an on-chain account.");

        deposits_table _deposits(get_self(), owner.value);
        auto idx = _deposits.get_index<"bycontrsym"_n>();
        uint128_t composite_id = (uint128_t{token.contract.value} << 64) | token.get_symbol().raw();
        auto itr = idx.find(composite_id);
        eosio::check(itr == idx.end(), "Owner already has an open account for token");

        _deposits.emplace(ram_payer, [&](auto& n) {
            n.id = _deposits.available_primary_key();
            n.balance = eosio::extended_asset(0, token);
        });
    }

}  // namespace avatarmk

EOSIO_ACTION_DISPATCHER(avatarmk::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    actions(avatarmk::actions),
    table("avatars"_n, avatarmk::avatars),
    table("deposits"_n, avatarmk::deposits),
    table("config"_n, avatarmk::config))
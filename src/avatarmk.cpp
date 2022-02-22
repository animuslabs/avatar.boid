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
        auto avatar = _avatars.get(avatar_id, "Avatar with this id doesn't exist.");

        //sub_balance
        sub_balance(minter, calculate_mint_price(avatar));

        //mint action
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
        add_balance(owner, eosio::extended_asset(0, token), ram_payer);
    }

}  // namespace avatarmk

EOSIO_ACTION_DISPATCHER(avatarmk::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    actions(avatarmk::actions),
    table("avatars"_n, avatarmk::avatars),
    table("deposits"_n, avatarmk::deposits),
    table("config"_n, avatarmk::config))
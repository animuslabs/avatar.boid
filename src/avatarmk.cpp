#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
//https://github.com/EOSIO/eosio.cdt/blob/master/libraries/eosiolib/core/eosio/crypto.hpp
// #include <eosio/crypto.hpp>
#include <token/token.hpp>
#include <eosio/bytes.hpp>
#include "avatarmk.hpp"
#include "functions.cpp"




namespace avatarmk
{

   void avatarmk_c::setconfig(std::optional<config> cfg )
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

      //calculate mintprice based on avatar.mint and avatar.rarity
      eosio::asset mint_price = calculate_mint_price(avatar);

      //sub_balance

      //mint action
   }



}  // namespace avatarmk

EOSIO_ACTION_DISPATCHER(avatarmk::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    actions(avatarmk::actions),
    table("avatars"_n, avatarmk::avatars),
    table("deposits"_n, avatarmk::deposits),
    table("config"_n, avatarmk::config))
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
//https://github.com/EOSIO/eosio.cdt/blob/master/libraries/eosiolib/core/eosio/crypto.hpp
#include <eosio/crypto.hpp>
#include <token/token.hpp>
#include <eosio/bytes.hpp>


#include "avatarmk.hpp"



namespace avatarmk
{

   void avatarmk_c::setconfig(std::optional<config> cfg ){
      require_auth(get_self());
      config_table _config(get_self(), get_self().value);
      cfg ? _config.set(cfg.value(), get_self()) : _config.remove();
   }



}  // namespace avatarmk

EOSIO_ACTION_DISPATCHER(avatarmk::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    actions(avatarmk::actions),
    table("avatars"_n, avatarmk::avatars),
    table("config"_n, avatarmk::config))
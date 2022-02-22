#pragma once
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

namespace avatarmk
{

   inline constexpr eosio::symbol core_symbol{"EOS", 4};
   inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, eosio::name("eosio.token") };

   struct config {
      bool freeze = false;
      eosio::asset base_mint_price{1, core_symbol};
   };
   EOSIO_REFLECT(config, freeze, base_mint_price)
   typedef eosio::singleton<"config"_n, config
   > config_table;

   struct deposits {
      eosio::asset balance;
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
   };
   EOSIO_REFLECT(deposits, balance)
   typedef eosio::multi_index<"deposits"_n, deposits> deposits_table;

   struct avatars {
      uint64_t id;//this can hold the template_id so that the int32_t isn't needed however atomic assets uses type int32_t so need to cast in that case
      int32_t template_id; //atomic assets template_id
      eosio::name creator; //creator of the template
      eosio::checksum256 identifier; //checksum from sorted vector containing all part_template_ids
      std::string ipfs_hash; //link to image for easy access
      uint8_t rarity;
      uint32_t mint; //how many are minted
      uint32_t max_mint; //added for convenience

      uint64_t primary_key() const { return id; }
      eosio::checksum256 by_idf() const { return identifier; }
   };
   EOSIO_REFLECT(avatars, id, template_id, creator, identifier, ipfs_hash, rarity, mint, max_mint)
   typedef eosio::multi_index<"avatars"_n, avatars,
      eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<avatars, eosio::checksum256, &avatars::by_idf>>
   >avatars_table;


   struct avatarmk_c : public eosio::contract
   {
      using eosio::contract::contract;

      // void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
      
      void setconfig(std::optional<config> cfg);
      void mintavatar(eosio::name& minter, uint64_t& avatar_id);
      
      private:
      void assemble(const eosio::name& creator, std::vector<int32_t>& part_template_ids);
      eosio::asset calculate_mint_price(const avatars& avatar);
      //internal accounting
      void add_balance( const eosio::name& owner, const eosio::asset& value);
      void sub_balance( const eosio::name& owner, const eosio::asset& value);



   };

   EOSIO_ACTIONS(avatarmk_c,  //
                 "avatarmk"_n,       //
                 action(setconfig, cfg)
               //   notify(eosio::any_contract, transfer)
               )

}  // namespace avatarmk
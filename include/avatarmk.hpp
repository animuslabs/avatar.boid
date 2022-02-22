#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>

namespace avatarmk {

    inline constexpr eosio::symbol core_symbol{"EOS", 4};
    inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, "eosio.token"_n};
    inline constexpr auto atomic_contract = "atomicassets"_n;

    struct config {
        bool freeze = false;
        eosio::asset base_mint_price{1, core_symbol};
    };
    EOSIO_REFLECT(config, freeze, base_mint_price)
    typedef eosio::singleton<"config"_n, config> config_table;

    struct deposits {
        uint64_t id;
        eosio::extended_asset balance;
        uint64_t primary_key() const { return id; }
        uint128_t by_contr_sym() const { return (uint128_t{balance.contract.value} << 64) | balance.quantity.symbol.raw(); }
    };
    EOSIO_REFLECT(deposits, id, balance)
    typedef eosio::multi_index<"deposits"_n, deposits, eosio::indexed_by<"bycontrsym"_n, eosio::const_mem_fun<deposits, uint128_t, &deposits::by_contr_sym>>> deposits_table;

    struct avatars {
        uint64_t id;                    //this can hold the template_id so that the int32_t isn't needed however atomic assets uses type int32_t so need to cast in that case
        int32_t template_id;            //atomic assets template_id
        eosio::name creator;            //creator of the template
        eosio::checksum256 identifier;  //checksum from sorted vector containing all part_template_ids
        std::string ipfs_hash;          //link to image for easy access
        uint8_t rarity;
        uint32_t mint;      //how many are minted
        uint32_t max_mint;  //added for convenience

        uint64_t primary_key() const { return id; }
        eosio::checksum256 by_idf() const { return identifier; }
    };
    EOSIO_REFLECT(avatars, id, template_id, creator, identifier, ipfs_hash, rarity, mint, max_mint)
    typedef eosio::multi_index<"avatars"_n, avatars, eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<avatars, eosio::checksum256, &avatars::by_idf>>> avatars_table;

    struct avatarmk_c : public eosio::contract {
        using eosio::contract::contract;

        //actions
        void setconfig(std::optional<config> cfg);
        void mintavatar(eosio::name& minter, uint64_t& avatar_id);
        void withdraw(const eosio::name& owner, const eosio::extended_asset& value);
        void open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer);

        //notifications
        void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
        void notify_logtransfer(eosio::name collection_name, eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo);

       private:
        void assemble(const eosio::name& creator, std::vector<int32_t>& part_template_ids);
        eosio::extended_asset calculate_mint_price(const avatars& avatar);
        //internal accounting
        void add_balance(const eosio::name& owner, const eosio::extended_asset& value, const eosio::name& ram_payer = eosio::name(0));
        void sub_balance(const eosio::name& owner, const eosio::extended_asset& value);
    };

    EOSIO_ACTIONS(avatarmk_c,
                  "avatarmk"_n,                           //
                  action(setconfig, cfg),                 //
                  action(withdraw, owner, value),         //
                  notify(eosio::any_contract, transfer),  //
                  notify(atomic_contract, logtransfer)    //

    )

}  // namespace avatarmk
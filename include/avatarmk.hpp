#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <numeric>
#include <cmath>

namespace avatarmk {

    inline constexpr eosio::symbol core_symbol{"EOS", 4};
    inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, "eosio.token"_n};
    inline constexpr auto atomic_contract = "atomicassets"_n;
    inline constexpr int day_sec = 86400;

    struct assemble_set {
        std::vector<uint32_t> template_ids;
        uint8_t rarity_score;
        eosio::checksum256 identifier;
    };
    EOSIO_REFLECT(assemble_set, template_ids, rarity_score, identifier)

    struct avatar_mint_fee {
        eosio::extended_asset fee;
        eosio::asset next_base_price;
    };

    struct config {
        bool freeze = false;
        eosio::asset floor_mint_price{1, core_symbol};
        eosio::name collection_name = "avatars1235a"_n;
        eosio::name parts_schema = "bodyparts123"_n;
        eosio::name avatar_schema = "avatarschema"_n;
    };
    EOSIO_REFLECT(config, freeze, floor_mint_price)
    typedef eosio::singleton<"config"_n, config> config_table;

    struct deposits {
        uint64_t id;
        eosio::extended_asset balance;
        uint64_t primary_key() const { return id; }
        uint128_t by_contr_sym() const { return (uint128_t{balance.contract.value} << 64) | balance.quantity.symbol.raw(); }
    };
    EOSIO_REFLECT(deposits, id, balance)
    // clang-format off
    typedef eosio::multi_index<"deposits"_n, deposits, 
    eosio::indexed_by<"bycontrsym"_n, eosio::const_mem_fun<deposits, uint128_t, &deposits::by_contr_sym>>
    > deposits_table;
    // clang-format on

    struct avatars {
        uint64_t id;
        std::string avatar_name;
        uint32_t template_id;           //atomic assets template_id
        eosio::name creator;            //creator of the template
        eosio::checksum256 identifier;  //checksum from sorted vector containing all part_template_ids
        std::string ipfs_hash;          //link to image for easy access
        uint8_t rarity;
        uint32_t mint;                   //how many are minted
        uint32_t max_mint;               //added for convenience
        eosio::time_point_sec modified;  //timestamp that gets updated each time the row gets modified (assemble, finalize, mint)
        eosio::asset base_price;

        uint64_t primary_key() const { return id; }
        uint64_t by_creator() const { return creator.value; }
        eosio::checksum256 by_idf() const { return identifier; }
    };
    EOSIO_REFLECT(avatars, id, template_id, creator, identifier, ipfs_hash, rarity, mint, max_mint)
    // clang-format off
    typedef eosio::multi_index<"avatars"_n, avatars,
    eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<avatars, uint64_t, &avatars::by_creator>>,
    eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<avatars, eosio::checksum256, &avatars::by_idf>>
    >avatars_table;
    // clang-format on

    struct avatarmk_c : public eosio::contract {
        using eosio::contract::contract;

        //actions
        void setconfig(std::optional<config> cfg);
        void withdraw(const eosio::name& owner, const eosio::extended_asset& value);
        void open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer);

        void assemble(const eosio::name& creator, assemble_set& set_data, std::string& avatar_name);
        void finalize(eosio::checksum256& identifier, std::string& ipfs_hash, uint32_t& template_id);
        void mintavatar(eosio::name& minter, uint64_t& avatar_id);
        using assemble_action = eosio::action_wrapper<"assemble"_n, &avatarmk_c::assemble>;

        //notifications
        void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
        void notify_logtransfer(eosio::name collection_name, eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo);

       private:
        std::optional<assemble_set> validate_assemble_set(std::vector<uint64_t> asset_ids, eosio::name owner, eosio::name collection_name, eosio::name schema_name);
        avatar_mint_fee calculate_mint_price(const avatars& avatar, const config& cfg);
        void validate_avatar_name(std::string& avatar_name);
        //internal accounting
        void add_balance(const eosio::name& owner, const eosio::extended_asset& value, const eosio::name& ram_payer = eosio::name(0));
        void sub_balance(const eosio::name& owner, const eosio::extended_asset& value);

        void burn_nfts(std::vector<uint64_t>& asset_ids);
    };

    EOSIO_ACTIONS(avatarmk_c,
                  "avatarmk"_n,                           //
                  action(setconfig, cfg),                 //
                  action(withdraw, owner, value),         //
                  action(open, owner, token, ram_payer),  //
                  action(assemble, creator, set_data),    //
                  action(finalize, identifier, ipfs_hash, template_id),
                  notify("eosio.token"_n, transfer),    //
                  notify(atomic_contract, logtransfer)  //

    )

}  // namespace avatarmk
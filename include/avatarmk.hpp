#pragma once
// #include "atomicassets.hpp"
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <numeric>
#include <cmath>
#include <token/token.hpp>

#define DEBUG

//for some reason it's not possible to include atomicassets.hpp in avatarmk.hpp.
//hence the typedefs for ATTRIBUTE_MAP here.
typedef std::vector<int8_t> INT8_VEC;
typedef std::vector<int16_t> INT16_VEC;
typedef std::vector<int32_t> INT32_VEC;
typedef std::vector<int64_t> INT64_VEC;
typedef std::vector<uint8_t> UINT8_VEC;
typedef std::vector<uint16_t> UINT16_VEC;
typedef std::vector<uint32_t> UINT32_VEC;
typedef std::vector<uint64_t> UINT64_VEC;
typedef std::vector<float> FLOAT_VEC;
typedef std::vector<double> DOUBLE_VEC;
typedef std::vector<std::string> STRING_VEC;

typedef std::variant<int8_t,
                     int16_t,
                     int32_t,
                     int64_t,
                     uint8_t,
                     uint16_t,
                     uint32_t,
                     uint64_t,
                     float,
                     double,
                     std::string,
                     INT8_VEC,
                     INT16_VEC,
                     INT32_VEC,
                     INT64_VEC,
                     UINT8_VEC,
                     UINT16_VEC,
                     UINT32_VEC,
                     UINT64_VEC,
                     FLOAT_VEC,
                     DOUBLE_VEC,
                     STRING_VEC>
    ATOMIC_ATTRIBUTE;

typedef std::map<std::string, ATOMIC_ATTRIBUTE> ATTRIBUTE_MAP;

namespace avatarmk {

    inline constexpr eosio::symbol core_symbol{"WAX", 8};
    inline constexpr eosio::extended_symbol extended_core_symbol{core_symbol, "eosio.token"_n};
    inline constexpr auto atomic_contract = "atomicassets"_n;
    inline constexpr int day_sec = 86400;

    struct assemble_set {
        std::string avatar_name;
        std::vector<uint32_t> template_ids;
        uint8_t rarity_score;
        eosio::checksum256 identifier;
        uint32_t max_mint;
        std::vector<std::pair<std::string, std::string>> bodypart_names;
        eosio::name scope;
        eosio::asset base_price;
    };
    EOSIO_REFLECT(assemble_set, avatar_name, template_ids, rarity_score, identifier, max_mint, scope, base_price)

    struct avatar_mint_fee {
        eosio::extended_asset fee;
        eosio::asset next_base_price;
    };

    struct config {
        bool freeze = false;
        eosio::asset floor_mint_price{1, core_symbol};
        eosio::name collection_name = "boidavatars1"_n;
        eosio::name parts_schema = "cartoonparts"_n;
        eosio::name avatar_schema = "testavatarsc"_n;
    };
    EOSIO_REFLECT(config, freeze, floor_mint_price, collection_name, parts_schema, avatar_schema)
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
        uint8_t rarity;
        uint32_t mint;                   //how many are minted
        uint32_t max_mint;               //added for convenience
        eosio::time_point_sec modified;  //timestamp that gets updated each time the row gets modified (assemble, finalize, mint)
        eosio::asset base_price;
        std::vector<uint32_t> bodyparts;

        uint64_t primary_key() const { return id; }
        uint64_t by_creator() const { return creator.value; }
        eosio::checksum256 by_idf() const { return identifier; }
    };
    EOSIO_REFLECT(avatars, id, avatar_name, template_id, creator, identifier, rarity, mint, max_mint, modified, base_price, bodyparts)
    // clang-format off
    typedef eosio::multi_index<"avatars"_n, avatars,
    eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<avatars, uint64_t, &avatars::by_creator>>,
    eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<avatars, eosio::checksum256, &avatars::by_idf>>
    >avatars_table;
    // clang-format on

    ///////
    struct queue {
        uint64_t id;
        eosio::checksum256 identifier;  //checksum from sorted vector containing all part_template_ids
        eosio::name creator;            //creator of the template
        eosio::name scope;              //which pack
        assemble_set set_data;
        eosio::time_point_sec inserted;  //timestamp that gets updated each time the row gets modified (assemble, finalize, mint)

        uint64_t primary_key() const { return id; }
        eosio::checksum256 by_idf() const { return identifier; }
        uint64_t by_scope() const { return scope.value; }
    };
    EOSIO_REFLECT(queue, id, identifier, creator, scope, set_data, inserted);
    // clang-format off
    typedef eosio::multi_index<"queue"_n, queue,
    eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<queue, eosio::checksum256, &queue::by_idf>>,
    eosio::indexed_by<"byscope"_n, eosio::const_mem_fun<queue, uint64_t, &queue::by_scope>>
    >queue_table;
    // clang-format on

    struct avatarmk_c : public eosio::contract {
        using eosio::contract::contract;

#if defined(DEBUG)
        void clravatars();
        void clrqueue();
#endif

        //actions
        void setconfig(std::optional<config> cfg);
        void withdraw(const eosio::name& owner, const eosio::extended_asset& value);
        void open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer);

        void assemble(const eosio::name& creator, assemble_set& set_data);
        void finalize(eosio::checksum256& identifier, std::string& ipfs_hash);
        void mintavatar(eosio::name& minter, uint64_t& avatar_id);
        using assemble_action = eosio::action_wrapper<"assemble"_n, &avatarmk_c::assemble>;

        //notifications
        void notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo);
        void notify_logtransfer(eosio::name collection_name, eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo);
        void notify_lognewtempl(int32_t template_id,
                                eosio::name authorized_creator,
                                eosio::name collection_name,
                                eosio::name schema_name,
                                bool transferable,
                                bool burnable,
                                uint32_t max_supply,
                                ATTRIBUTE_MAP immutable_data);  //,atomicassets::ATTRIBUTE_MAP immutable_data

       private:
        assemble_set validate_assemble_set(std::vector<uint64_t> asset_ids, eosio::name owner, eosio::name collection_name);
        eosio::checksum256 calculateIdentifier(std::vector<uint32_t>& template_ids);
        avatar_mint_fee calculate_mint_price(const avatars& avatar, const config& cfg);
        void validate_avatar_name(std::string& avatar_name);
        //internal accounting
        void add_balance(const eosio::name& owner, const eosio::extended_asset& value, const eosio::name& ram_payer = eosio::name(0));
        void sub_balance(const eosio::name& owner, const eosio::extended_asset& value);

        void burn_nfts(std::vector<uint64_t>& asset_ids);
    };

    // clang-format off
    EOSIO_ACTIONS(
                avatarmk_c,
                "avatarmk"_n,
                #if defined(DEBUG)
                action(clravatars),
                action(clrqueue),
                #endif
                action(setconfig, cfg),
                action(withdraw, owner, value),
                action(open, owner, token, ram_payer),
                action(assemble, creator, set_data),
                action(finalize, identifier, ipfs_hash),
                action(mintavatar, minter, avatar_id),
                notify("eosio.token"_n, transfer),
                notify(atomic_contract, logtransfer),
                notify(atomic_contract, lognewtempl)
    )
    // clang-format on

}  // namespace avatarmk
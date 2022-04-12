#pragma once
// #include "atomicassets.hpp"
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
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
    inline constexpr auto rng_contract = "orng.wax"_n;
    inline constexpr int day_sec = 86400;

    //used as return value by mint price calculation
    struct avatar_mint_price {
        eosio::extended_asset price;
        eosio::asset next_base_price;
    };

    struct namepair {
        std::string bodypart;
        std::string name;
    };
    EOSIO_REFLECT(namepair, bodypart, name)

    struct assemble_set {
        eosio::name creator;
        eosio::name avatar_name;
        std::vector<uint32_t> template_ids;
        uint8_t rarity_score;
        eosio::checksum256 identifier;
        uint32_t max_mint;
        std::vector<namepair> bodypart_names;  ///////////////////
        eosio::name scope;
        eosio::asset base_price;
    };
    EOSIO_REFLECT(assemble_set, creator, avatar_name, template_ids, rarity_score, identifier, max_mint, bodypart_names, scope, base_price)

    struct config {
        bool freeze = false;
        bool auto_claim_packs = false;
        eosio::name collection_name = "boidavatars1"_n;
        eosio::name parts_schema = "avatarparts"_n;
        eosio::name avatar_schema = "testavatarsc"_n;
        eosio::name pack_schema = "partspacksch"_n;
    };
    EOSIO_REFLECT(config, freeze, auto_claim_packs, collection_name, parts_schema, avatar_schema, pack_schema)
    typedef eosio::singleton<"config"_n, config> config_table;

    struct pack_data {
        eosio::name edition;
        uint8_t pack_size;  //number of nfts in pack
    };
    EOSIO_REFLECT(pack_data, edition, pack_size)

    struct unpack {
        uint64_t pack_asset_id;
        eosio::name owner;
        pack_data pack_data;
        std::vector<uint32_t> claimable_template_ids;
        eosio::time_point_sec inserted;
        uint64_t primary_key() const { return pack_asset_id; }
        uint64_t by_owner() const { return owner.value; }
    };
    EOSIO_REFLECT(unpack, pack_asset_id, owner, pack_data, claimable_template_ids, inserted)
    // clang-format off
    typedef eosio::multi_index<"unpack"_n, unpack,
    eosio::indexed_by<"byowner"_n, eosio::const_mem_fun<unpack, uint64_t, &unpack::by_owner>>
    >unpack_table;
    // clang-format on

    //scoped by edition
    struct packs {
        uint64_t template_id;
        eosio::asset base_price;
        eosio::asset floor_price;
        eosio::time_point_sec last_sold;
        std::string pack_name;
        uint64_t primary_key() const { return template_id; }
    };
    EOSIO_REFLECT(packs, template_id, base_price, floor_price, last_sold, pack_name)
    // clang-format off
    typedef eosio::multi_index<"packs"_n, packs >packs_table;
    // clang-format on

    struct editions {
        eosio::name edition_scope;             //primary key, must be unique and function as identifier of different part groups (scope)
        eosio::asset avatar_floor_mint_price;  // min price to mint an avatar from this edition
        eosio::asset avatar_template_price;
        uint64_t avatar_template_count;
        std::vector<std::vector<uint32_t>> part_template_ids = {{}, {}, {}, {}, {}};  //index matches rarityscore -1

        uint64_t primary_key() const { return edition_scope.value; }
    };
    EOSIO_REFLECT(editions, edition_scope, avatar_floor_mint_price, avatar_template_price, avatar_template_count, part_template_ids)
    // clang-format off
    typedef eosio::multi_index<"editions"_n, editions >editions_table;
    // clang-format on

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

    //scoped by edition
    struct avatars {
        eosio::name avatar_name;
        uint32_t template_id;           //atomic assets template_id
        eosio::name creator;            //creator of the template
        eosio::checksum256 identifier;  //checksum from sorted vector containing all part_template_ids
        uint8_t rarity;
        uint32_t mint;                   //how many are minted
        uint32_t max_mint;               //added for convenience
        eosio::time_point_sec modified;  //timestamp that gets updated each time the row gets modified (assemble, finalize, mint)
        eosio::asset base_price;
        std::vector<uint32_t> bodyparts;

        uint64_t primary_key() const { return avatar_name.value; }
        uint64_t by_creator() const { return creator.value; }
        eosio::checksum256 by_idf() const { return identifier; }
    };
    EOSIO_REFLECT(avatars, avatar_name, template_id, creator, identifier, rarity, mint, max_mint, modified, base_price, bodyparts)
    // clang-format off
    typedef eosio::multi_index<"avatars"_n, avatars,
    eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<avatars, uint64_t, &avatars::by_creator>>,
    eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<avatars, eosio::checksum256, &avatars::by_idf>>
    >avatars_table;
    // clang-format on

    ///////
    struct queue {
        eosio::name avatar_name;
        eosio::checksum256 identifier;   //checksum from sorted vector containing all part_template_ids
        eosio::name work;                //[assemble, potion, etc]
        eosio::name scope;               //which pack
        assemble_set set_data;           //can make this a variant for future work types with different payload
        eosio::time_point_sec inserted;  //timestamp that gets updated each time the row gets modified (assemble, finalize, mint)

        uint64_t primary_key() const { return avatar_name.value; }
        eosio::checksum256 by_idf() const { return identifier; }
        uint64_t by_scope() const { return scope.value; }
    };
    EOSIO_REFLECT(queue, avatar_name, identifier, work, scope, set_data, inserted);
    // clang-format off
    typedef eosio::multi_index<"queue"_n, queue,
    eosio::indexed_by<"byidf"_n, eosio::const_mem_fun<queue, eosio::checksum256, &queue::by_idf>>,
    eosio::indexed_by<"byscope"_n, eosio::const_mem_fun<queue, uint64_t, &queue::by_scope>>
    >queue_table;
    // clang-format on

    struct avatarmk_c : public eosio::contract {
        using eosio::contract::contract;

#if defined(DEBUG)
        void clravatars(eosio::name& scope);
        void clrqueue();
        void clrunpack();
        void test(const uint64_t id);
#endif

        //actions
        void setparts(const eosio::name& edition_scope, const std::vector<uint32_t> template_ids, std::vector<uint8_t>& rarity_scores);
        void receiverand(uint64_t& assoc_id, const eosio::checksum256& random_value);

        void setconfig(std::optional<config> cfg);
        void packadd(eosio::name& edition_scope, uint64_t& template_id, eosio::asset& base_price, eosio::asset& floor_price, std::string& pack_name);
        void packdel(eosio::name& edition_scope, uint64_t& template_id);
        void editionset(eosio::name& edition_scope, eosio::asset& avatar_floor_mint_price, eosio::asset& avatar_template_price);
        void editiondel(eosio::name& edition_scope);
        void withdraw(const eosio::name& owner, const eosio::extended_asset& value);
        void open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer);

        void buypack(eosio::name& buyer, eosio::name& edition_scope, uint64_t& template_id);
        void claimpack(eosio::name& owner, uint64_t& pack_asset_id);
        void assemble(assemble_set& set_data);
        void finalize(eosio::checksum256& identifier, std::string& ipfs_hash);
        void mintavatar(eosio::name& minter, eosio::name& avatar_name, eosio::name& scope);
        void setowner(eosio::name& owner, eosio::name& new_owner, eosio::name& avatar_name, eosio::name& scope);
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
        pack_data validate_pack(const uint64_t& asset_id, const config& cfg);
        assemble_set validate_assemble_set(std::vector<uint64_t> asset_ids, config cfg);
        eosio::checksum256 calculateIdentifier(std::vector<uint32_t>& template_ids);
        avatar_mint_price calculate_mint_price(const avatars& avatar, const eosio::asset& avatar_floor_mint_price);

        //internal accounting
        void add_balance(const eosio::name& owner, const eosio::extended_asset& value, const eosio::name& ram_payer = eosio::name(0));
        void sub_balance(const eosio::name& owner, const eosio::extended_asset& value);

        void burn_nfts(std::vector<uint64_t>& asset_ids);
        eosio::checksum256 get_trx_id();
        void register_part(const eosio::name& edition_scope, const uint32_t& template_id, const uint8_t& rarity_score);
        bool in_range(unsigned low, unsigned high, unsigned x) { return ((x - low) <= (high - low)); }
    };

    // clang-format off
    EOSIO_ACTIONS(
                avatarmk_c,
                "avatarmk"_n,
                action(packadd, edition_scope, template_id, base_price, floor_price, pack_name),
                action(packdel, edition_scope, template_id),
                action(buypack, buyer, edition_scope, template_id),
                action(claimpack, owner, pack_asset_id),
                action(editionset, edition_scope, avatar_floor_mint_price, avatar_template_price),
                action(editiondel, edition_scope),
                action(setparts, edition_scope, template_ids, rarity_scores),
                action(setconfig, cfg),
                action(withdraw, owner, value),
                action(open, owner, token, ram_payer),
                action(assemble, set_data),
                action(finalize, identifier, ipfs_hash),
                action(mintavatar, minter, avatar_name, scope),
                action(receiverand, assoc_id, random_value),
                action(setowner, owner, new_owner, avatar_name, scope),
                
                #if defined(DEBUG)
                action(test, id),
                action(clravatars, scope),
                action(clrqueue),
                action(clrunpack),
                #endif
                notify("eosio.token"_n, transfer),
                notify(atomic_contract, logtransfer),
                notify(atomic_contract, lognewtempl)
    )
    // clang-format on

}  // namespace avatarmk
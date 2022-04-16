#pragma once
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

namespace avatarmk {
    struct config {
        bool freeze = false;
        bool auto_claim_packs = false;
        bool whitelist_enabled = true;
        eosio::name collection_name = "boidavatars1"_n;
        eosio::name parts_schema = "avatarparts"_n;
        eosio::name avatar_schema = "testavatarsc"_n;
        eosio::name pack_schema = "partspacksch"_n;
    };
    EOSIO_REFLECT(config, freeze, auto_claim_packs, whitelist_enabled, collection_name, parts_schema, avatar_schema, pack_schema)
    typedef eosio::singleton<"config"_n, config> config_table;
}
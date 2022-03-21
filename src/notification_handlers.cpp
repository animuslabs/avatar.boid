// #pragma once
#include "avatarmk.hpp"
#include "atomicassets.hpp"
namespace avatarmk {
    //
    //system core token transfer handler
    //
    void avatarmk_c::notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo)
    {
        //dispatcher accepts tokens from all contracts!
        if (from == get_self() || to != get_self()) return;
        if (from == "eosio"_n || from == "eosio.rex"_n || from == "eosio.stake"_n) return;

        if (memo == std::string("deposit")) {
            eosio::check(quantity.amount > 0, "Transfer amount must be positive");
            eosio::extended_asset extended_quantity(quantity, get_first_receiver());
            add_balance(from, extended_quantity);  //
        }
    }

    //
    //atomic asset handlers
    //
    void avatarmk_c::notify_logtransfer(eosio::name collection_name, eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo)
    {
        if (get_first_receiver() != atomic_contract) return;
        config_table _config(get_self(), get_self().value);
        const auto cfg = _config.get_or_create(get_self(), config());
        if (collection_name != cfg.collection_name) return;

        //incoming transfers
        if (to == get_self()) {
            if (memo.substr(0, 8) == "assemble") {
                //validate_assemble_set will assert when not a valid set
                auto assemble_set = validate_assemble_set(asset_ids, cfg.collection_name);
                assemble_set.creator = from;
                assemble_set.avatar_name = memo.substr(9);  //assemble:avatarname

                validate_avatar_name(assemble_set.avatar_name);
                const auto data = std::make_tuple(from, assemble_set);
                eosio::action(eosio::permission_level{get_self(), "active"_n}, get_self(), "assemble"_n, data).send();  //can be a function call instead of action
                burn_nfts(asset_ids);
            }
            if (memo == std::string("potion")) {
                //check if received nfts are 1x avatar + potion nfts
                //trigger inline "log" action that the server can intercept to render new nft
                //burn received assets
            }
        }
    }

    void avatarmk_c::notify_lognewtempl(int32_t template_id,
                                        eosio::name authorized_creator,
                                        eosio::name collection_name,
                                        eosio::name schema_name,
                                        bool transferable,
                                        bool burnable,
                                        uint32_t max_supply,
                                        atomicassets::ATTRIBUTE_MAP immutable_data)
    {
        if (get_first_receiver() != atomic_contract) return;

        config_table _config(get_self(), get_self().value);
        const auto cfg = _config.get_or_create(get_self(), config());
        if (collection_name != cfg.collection_name) return;

        schemacfg_table _schemacfg(get_self(), get_self().value);
        auto idx = _schemacfg.get_index<eosio::name("byavatar")>();
        schemacfg scfg = idx.get(schema_name.value, "schema_name during lognewtempl not in schemacfg");

        //catch avatar template creation

        auto template_ids = std::get<UINT32_VEC>(immutable_data["bodyparts"]);
        auto identifier = calculateIdentifier(template_ids);

        //auto scope = std::get<std::string>(immutable_data["edition"]);

        avatars_table _avatars(get_self(), scfg.part_schema_name.value);
        auto a_idx = _avatars.get_index<eosio::name("byidf")>();
        auto existing = a_idx.require_find(identifier, "Identifier not found in scoped avatars table during lognewtempl notify handler.");

        //update template_id
        a_idx.modify(existing, eosio::same_payer, [&](auto& n) {
            n.template_id = template_id;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
        });

        //
    }

}  // namespace avatarmk

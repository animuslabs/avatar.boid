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
            if (memo == std::string("assemble")) {
                auto assemble_set = validate_assemble_set(asset_ids, to, cfg.collection_name, cfg.parts_schema);
                if (assemble_set) {
                    //trigger inline assemble, this needs to be caught on the server which calls the finalize action
                    //after creating a new template for the assembled avatar.
                    const auto data = std::make_tuple(from, assemble_set.value());
                    eosio::action(eosio::permission_level{get_self(), "active"_n}, get_self(), "assemble"_n, data).send();
                }
                else {
                    eosio::check(false, "Received NFTs not viable for avatar assembly");
                }
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
        //
    }

}  // namespace avatarmk

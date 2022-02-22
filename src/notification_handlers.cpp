// #pragma once
#include "avatarmk.hpp"

namespace avatarmk {
    //
    //system core token transfer handler
    //
    void avatarmk_c::notify_transfer(eosio::name from, eosio::name to, const eosio::asset& quantity, std::string memo)
    {
        static_cast<void>(memo);
        //dispatcher accepts tokens from all contracts!
        if (from == get_self() || to != get_self())
            return;
        if (from == "eosio"_n || from == "eosio.rex"_n || from == "eosio.stake"_n)
            return;

        eosio::check(quantity.amount > 0, "Transfer amount must be positive");
        eosio::extended_asset extended_quantity(quantity, get_first_receiver());
        add_balance(from, extended_quantity);  //
    }

    //
    //atomic asset handlers
    //
    void avatarmk_c::notify_logtransfer(eosio::name collection_name, eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo) {}

}  // namespace avatarmk

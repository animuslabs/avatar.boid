// #pragma once
// #include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include "avatarmk.hpp"

namespace avatarmk
{
    void avatarmk_c::assemble(const eosio::name& creator, std::vector<int32_t>& part_template_ids)
    {
        //validate set (loop through vector)
        
        //calculate avatar identifier
        sort(part_template_ids.begin(), part_template_ids.end());
        auto packed_parts = eosio::pack(part_template_ids);
        std::string sdata = std::string(packed_parts.begin(), packed_parts.end());
        eosio::checksum256 identifier = eosio::sha256(sdata.c_str(), sdata.length() );

        //init avatars table
        avatars_table _avatars(get_self(), get_self().value);
        auto idx = _avatars.get_index<eosio::name("byidf")>();

        //check if identifier already exists
        eosio::check(idx.find(identifier) == idx.end(), "Avatar with these body parts already available to mint.");

        //calculate rarity & max_mint

        //add new avatar to table
        _avatars.emplace(get_self(),[&](auto& n){
            n.id = _avatars.available_primary_key();
            n.template_id = 1;
            n.creator = creator;
            n.identifier = identifier;
            n.ipfs_hash = "hashhashhahshash";
            n.rarity = 5; //1-5 
            n.mint = 0;
            n.max_mint = 0;
        });

    }

    eosio::asset avatarmk_c::calculate_mint_price(const avatars& avatar)
    {
        config_table _config(get_self(), get_self().value);
        auto cfg = _config.get_or_create(get_self(), config());
        int core_precision_pow = pow(10, core_symbol.precision());
        //this is wip
        double p = (cfg.base_mint_price.amount * avatar.rarity) / avatar.mint;
        eosio::asset mint_price{static_cast<int64_t>(p * core_precision_pow), core_symbol};
        return mint_price;
    }

    void avatarmk_c::sub_balance( const eosio::name& owner, const eosio::asset& value) 
    {
        deposits_table _deposits( get_self(), owner.value);

        const auto& itr = _deposits.get( value.symbol.code().raw(), "No balance with this symbol.");
        eosio::check( itr.balance.amount >= value.amount, "Overdrawn balance need "+ value.to_string() );

        _deposits.modify( itr, eosio::same_payer, [&]( auto& a) {
            a.balance -= value;
        });
    }

    void avatarmk_c::add_balance( const eosio::name& owner, const eosio::asset& value)
    {
        deposits_table _deposits( get_self(), owner.value);
        auto itr = _deposits.find( value.symbol.code().raw() );

        if( itr == _deposits.end() ) {
            //new receiver, can assert here and request to open a deposits account
            _deposits.emplace( get_self(), [&]( auto& a){
                a.balance = value;
            });
        } 
        else {
            //existing
            _deposits.modify( itr, eosio::same_payer, [&]( auto& a) {
                a.balance += value;
            });
        }
    }


}
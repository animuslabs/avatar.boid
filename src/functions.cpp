// #pragma once
// #include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include "avatarmk.hpp"

namespace avatarmk
{
    void avatarmk_c::assemble(eosio::name& creator, std::vector<int32_t>& part_template_ids)
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

}
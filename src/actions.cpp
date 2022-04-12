#include "avatarmk.hpp"
#include "atomicassets.hpp"
#include "randomness_provider.hpp"

namespace avatarmk {

    void avatarmk_c::editionset(eosio::name& edition_scope, eosio::asset& avatar_floor_mint_price, eosio::asset& avatar_template_price)
    {
        //warning no input validation!
        require_auth(get_self());
        editions_table _editions(get_self(), get_self().value);
        auto itr = _editions.find(edition_scope.value);

        if (itr == _editions.end()) {
            //new edition
            _editions.emplace(get_self(), [&](auto& n) {
                n.edition_scope = edition_scope;
                n.avatar_template_price = avatar_template_price;
                n.avatar_floor_mint_price = avatar_floor_mint_price;
            });
        }
        else {
            //existing edition
            _editions.modify(itr, eosio::same_payer, [&](auto& n) {
                n.avatar_template_price = avatar_template_price;
                n.avatar_floor_mint_price = avatar_floor_mint_price;
            });
        }
    }
    void avatarmk_c::editiondel(eosio::name& edition_scope)
    {
        require_auth(get_self());
        editions_table _editions(get_self(), get_self().value);
        auto itr = _editions.require_find(edition_scope.value, "Edition with this scope not registered");
        _editions.erase(itr);
    }

    void avatarmk_c::setconfig(std::optional<config> cfg)
    {
        require_auth(get_self());
        config_table _config(get_self(), get_self().value);
        cfg ? _config.set(cfg.value(), get_self()) : _config.remove();
    }

    void avatarmk_c::mintavatar(eosio::name& minter, eosio::name& avatar_name, eosio::name& scope)
    {
        require_auth(minter);
        avatars_table _avatars(get_self(), scope.value);
        auto itr = _avatars.require_find(avatar_name.value, "Avatar with this name doesn't exist.");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        eosio::time_point_sec now(eosio::current_time_point());

        if (itr->mint == 0 && now < itr->modified + 3600) {
            eosio::check(itr->creator == minter, "Only the creator can mint the first avatar within the first hour the template is finalized");
        }

        editions_table _editions(get_self(), get_self().value);
        editions edition_cfg = _editions.get(scope.value, "Scope is not a valid edition");

        //billing logic
        avatar_mint_price amp = calculate_mint_price(*itr, edition_cfg.avatar_floor_mint_price);
        sub_balance(minter, amp.price);

        if (minter == itr->creator || itr->creator == get_self()) {
            //don't reward the template creator if he is the minter or if the owner is self.
            add_balance(get_self(), amp.price, get_self());
        }
        else {
            double pct_cut = 0.5;  //50%
            auto contract_share = eosio::extended_asset((uint64_t)(amp.price.quantity.amount * pct_cut), amp.price.get_extended_symbol());
            auto user_reward = amp.price - contract_share;
            add_balance(itr->creator, user_reward, get_self());
            add_balance(get_self(), contract_share, get_self());
        }

        //atomic mint action
        const auto mutable_data = atomicassets::ATTRIBUTE_MAP{};
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        immutable_data["mint"] = itr->mint + 1;

        _avatars.modify(itr, eosio::same_payer, [&](auto& n) {
            n.mint += 1;
            n.modified = now;
            n.base_price = amp.next_base_price;
        });

        const std::vector<eosio::asset> tokens_to_back;
        const auto data = make_tuple(get_self(), cfg.collection_name, cfg.avatar_schema, itr->template_id, minter, immutable_data, mutable_data, tokens_to_back);
        eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "mintasset"_n, data).send();
    }

    void avatarmk_c::setowner(eosio::name& owner, eosio::name& new_owner, eosio::name& avatar_name, eosio::name& scope)
    {
        require_auth(owner);
        avatars_table _avatars(get_self(), scope.value);
        auto itr = _avatars.require_find(avatar_name.value, "Avatar with this name doesn't exist in this scope.");
        eosio::check(itr->creator == owner, "this avatar isn't owned by owner.");
        eosio::check(eosio::is_account(new_owner), "new_owner isn't a valid account.");
        _avatars.modify(itr, eosio::same_payer, [&](auto& n) { n.creator = new_owner; });
    }

    void avatarmk_c::assemble(assemble_set& set_data)
    {
        //assemble adds work to the queue
        require_auth(get_self());
        avatars_table _avatars(get_self(), set_data.scope.value);
        auto a_idx = _avatars.get_index<eosio::name("byidf")>();
        eosio::check(a_idx.find(set_data.identifier) == a_idx.end(), "Avatar with these body parts already available to mint.");

        queue_table _queue(get_self(), get_self().value);
        auto q_idx = _queue.get_index<eosio::name("byidf")>();
        eosio::check(q_idx.find(set_data.identifier) == q_idx.end(), "Avatar with these body parts already in queue.");

        auto q_name = _queue.find(set_data.avatar_name.value);
        eosio::check(q_name == _queue.end(), "avatar with this name already in queue");

        auto a_name = _avatars.find(set_data.avatar_name.value);
        eosio::check(a_name == _avatars.end(), "avatar with this name already exists in edition");

        //add to queue
        _queue.emplace(get_self(), [&](auto& n) {
            n.avatar_name = set_data.avatar_name;
            n.scope = set_data.scope;
            n.identifier = set_data.identifier;
            n.work = eosio::name("assemble");
            n.set_data = set_data;
            n.inserted = eosio::time_point_sec(eosio::current_time_point());
        });
    }
    void avatarmk_c::finalize(eosio::checksum256& identifier, std::string& ipfs_hash)
    {
        //finalize will remove from queue
        require_auth(get_self());
        eosio::check(ipfs_hash.size() > 0, "ipfs_hash required");  //this validation can be done better

        queue_table _queue(get_self(), get_self().value);
        auto q_idx = _queue.get_index<eosio::name("byidf")>();
        auto queue_entry = q_idx.require_find(identifier, "Avatar with this identifier not in the queue.");

        eosio::name scope = queue_entry->scope;  // = edition

        avatars_table _avatars(get_self(), scope.value);
        auto a_idx = _avatars.get_index<eosio::name("byidf")>();
        eosio::check(a_idx.find(identifier) == a_idx.end(), "Avatar with this identifier already finalized.");

        //create template inline action
        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        const eosio::name authorized_creator = get_self();
        const eosio::name collection_name = cfg.collection_name;
        const eosio::name schema_name = cfg.avatar_schema;
        const bool transferable = true;
        const bool burnable = true;
        const uint32_t max_supply = queue_entry->set_data.max_mint;
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        //must match avatar schema!!!!!!!!
        immutable_data["name"] = queue_entry->set_data.avatar_name.to_string();
        immutable_data["edition"] = scope.to_string();
        immutable_data["img"] = ipfs_hash;
        immutable_data["rarityScore"] = queue_entry->set_data.rarity_score;
        immutable_data["bodyparts"] = queue_entry->set_data.template_ids;  //vector template_ids

        //add part names to immutable_data
        for (auto p : queue_entry->set_data.bodypart_names) {
            immutable_data[p.bodypart] = p.name;
        }

        const auto data = make_tuple(authorized_creator, collection_name, schema_name, transferable, burnable, max_supply, immutable_data);
        eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "createtempl"_n, data).send();

        //destructure set_data in avatar table scope + complete with finalize args (ipfs_hash)..
        _avatars.emplace(get_self(), [&](auto& n) {
            n.avatar_name = queue_entry->set_data.avatar_name;
            n.rarity = queue_entry->set_data.rarity_score;
            n.creator = queue_entry->set_data.creator;
            n.identifier = queue_entry->identifier;
            n.base_price = queue_entry->set_data.base_price;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
            n.bodyparts = queue_entry->set_data.template_ids;
            n.max_mint = queue_entry->set_data.max_mint;
        });
        //delete queue entry, not needed anymore.
        q_idx.erase(queue_entry);

        //update template counter in editions table
        editions_table _editions(get_self(), get_self().value);
        auto edition_itr = _editions.require_find(scope.value, "Scope is not a valid edition");
        _editions.modify(edition_itr, eosio::same_payer, [&](auto& n) { n.avatar_template_count += 1; });
    }

    void avatarmk_c::packadd(eosio::name& edition_scope, uint64_t& template_id, eosio::asset& base_price, eosio::asset& floor_price, std::string& pack_name)
    {
        require_auth(get_self());
        packs_table _packs(get_self(), edition_scope.value);
        auto p_itr = _packs.find(template_id);
        eosio::check(p_itr == _packs.end(), "Pack with this template_id already in table");
        _packs.emplace(get_self(), [&](auto& n) {
            n.template_id = template_id;
            n.base_price = base_price;
            n.floor_price = floor_price;
            n.pack_name = pack_name;
        });
    }
    void avatarmk_c::packdel(eosio::name& edition_scope, uint64_t& template_id)
    {
        require_auth(get_self());
        packs_table _packs(get_self(), edition_scope.value);
        auto p_itr = _packs.find(template_id);
        eosio::check(p_itr != _packs.end(), "Pack with this template_id not found");
        _packs.erase(p_itr);
    }

    void avatarmk_c::buypack(eosio::name& buyer, eosio::name& edition_scope, uint64_t& template_id)
    {
        require_auth(buyer);
        packs_table _packs(get_self(), edition_scope.value);
        auto p_itr = _packs.require_find(template_id, "Pack with this template_id not found for this scope");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        //calculate price
        eosio::extended_asset p = {p_itr->base_price, extended_core_symbol.get_contract()};
        sub_balance(buyer, p);
        add_balance(get_self(), p, get_self());

        _packs.modify(p_itr, eosio::same_payer, [&](auto& n) { n.last_sold = eosio::time_point_sec(eosio::current_time_point()); });

        const auto mutable_data = atomicassets::ATTRIBUTE_MAP{};
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        const std::vector<eosio::asset> tokens_to_back;

        const auto data = make_tuple(get_self(), cfg.collection_name, cfg.pack_schema, (uint32_t)template_id, buyer, immutable_data, mutable_data, tokens_to_back);

        eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "mintasset"_n, data).send();
    }

    void avatarmk_c::claimpack(eosio::name& owner, uint64_t& pack_asset_id)
    {
        eosio::check(has_auth(get_self()) || has_auth(owner), "Need authorization of owner or contract");
        unpack_table _unpack(get_self(), get_self().value);

        auto itr = _unpack.find(pack_asset_id);
        eosio::check(itr != _unpack.end(), "pack with this id not in the unpack table");
        eosio::check(itr->owner == owner, "you are not the owner of this pack");
        eosio::check(itr->claimable_template_ids.size() != 0, "pack not ready to claim yet. waiting for oracle to draw random cards");
        eosio::check(itr->claimable_template_ids.size() == itr->pack_data.pack_size, "Claimable templates count doesn't equal pack_size");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        const auto mutable_data = atomicassets::ATTRIBUTE_MAP{};
        const auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        const std::vector<eosio::asset> tokens_to_back;
        for (const auto template_id : itr->claimable_template_ids) {
            const auto data = make_tuple(get_self(), cfg.collection_name, cfg.parts_schema, template_id, owner, immutable_data, mutable_data, tokens_to_back);
            eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "mintasset"_n, data).send();
        }
        _unpack.erase(itr);
    }

    void avatarmk_c::receiverand(uint64_t& assoc_id, const eosio::checksum256& random_value)
    {
        eosio::check(has_auth(get_self()) || has_auth(rng_contract), "Need authorization of owner or contract");
        std::vector<uint32_t> result;
        unpack_table _unpack(get_self(), get_self().value);
        auto itr = _unpack.require_find(assoc_id, "error");

        editions_table _editions(get_self(), get_self().value);
        editions edition_cfg = _editions.get(itr->pack_data.edition.value, "Edition not found");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        RandomnessProvider RP(random_value);
        //draw cards
        for (int i = 0; i < itr->pack_data.pack_size; i++) {
            int rarity_index = 1;
            uint32_t r = RP.get_rand(100);
            if (in_range(1, 5, r)) {  //5% chance
                rarity_index = 5;
            }
            else if (in_range(6, 15, r)) {  //10% chance
                rarity_index = 4;
            }
            else if (in_range(16, 35, r)) {  //20% chance
                rarity_index = 3;
            }
            else if (in_range(36, 60, r)) {  //25% chance
                rarity_index = 2;
            }
            else if (in_range(61, 100, r)) {  //40% chance
                rarity_index = 1;
            }
            auto r2 = RP.get_rand(edition_cfg.part_template_ids[rarity_index - 1].size());
            result.push_back(edition_cfg.part_template_ids[rarity_index - 1][r2]);
        }

        _unpack.modify(itr, eosio::same_payer, [&](auto& n) {
            n.claimable_template_ids = result;
            n.inserted = eosio::time_point_sec(eosio::current_time_point());
        });
        if (cfg.auto_claim_packs) {
            //untested
            auto data = std::make_tuple(itr->owner, assoc_id);
            eosio::action(eosio::permission_level{get_self(), "active"_n}, get_self(), "claimpack"_n, data).send();
        }
    }

    void avatarmk_c::setparts(const eosio::name& edition_scope, const std::vector<uint32_t> template_ids, std::vector<uint8_t>& rarity_scores)
    {
        require_auth(get_self());
        editions_table _editions(get_self(), get_self().value);
        auto itr = _editions.find(edition_scope.value);
        eosio::check(itr != _editions.end(), "configure edition before creating new part templates.");
        eosio::check(template_ids.size() == rarity_scores.size(), "invalid input. vectors must be of equal size.");

        if (template_ids.size() == 0) {
            _editions.modify(itr, eosio::same_payer, [&](auto& n) { n.part_template_ids = {{}, {}, {}, {}, {}}; });
            return;
        }

        _editions.modify(itr, eosio::same_payer, [&](auto& n) {
            for (unsigned i = 0; i < template_ids.size(); ++i) {
                auto rarity_index = rarity_scores[i] - 1;
                if (template_ids.size() == 1) {
                    auto existing = std::find(n.part_template_ids[rarity_index].begin(), n.part_template_ids[rarity_index].end(), template_ids[i]);
                    if (existing != n.part_template_ids[rarity_index].end()) {
                        n.part_template_ids[rarity_index].erase(existing);
                    }
                    else {
                        n.part_template_ids[rarity_index].push_back(template_ids[i]);
                    }
                }
                else {
                    n.part_template_ids[rarity_index].push_back(template_ids[i]);
                }
            }
        });
    }

}  // namespace avatarmk
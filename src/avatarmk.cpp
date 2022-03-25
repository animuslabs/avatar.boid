#include "avatarmk.hpp"
#include "atomicassets.hpp"
#include "functions.cpp"
#include "notification_handlers.cpp"
#include "randomness_provider.hpp"

namespace avatarmk {

    void avatarmk_c::editionadd(eosio::name& edition_scope, eosio::asset& avatar_floor_mint_price)
    {
        //warning no input validation!
        require_auth(get_self());
        editions_table _editions(get_self(), get_self().value);
        auto itr = _editions.find(edition_scope.value);
        eosio::check(itr == _editions.end(), "Edition with this scope already registered");

        _editions.emplace(get_self(), [&](auto& n) {
            n.edition_scope = edition_scope;
            n.avatar_floor_mint_price = avatar_floor_mint_price;
        });
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

    void avatarmk_c::mintavatar(eosio::name& minter, uint64_t& avatar_id, eosio::name& scope)
    {
        require_auth(minter);
        avatars_table _avatars(get_self(), scope.value);
        auto itr = _avatars.require_find(avatar_id, "Avatar with this id doesn't exist.");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        editions_table _editions(get_self(), get_self().value);
        editions edition_cfg = _editions.get(scope.value, "Scope is not a valid edition");

        //for now 100% of fee goes to template owner
        avatar_mint_fee amf = calculate_mint_price(*itr, edition_cfg.avatar_floor_mint_price);
        sub_balance(minter, amf.fee);
        add_balance(itr->creator, amf.fee, get_self());  //let self pay for ram if new table entry?

        //atomic mint action
        const auto mutable_data = atomicassets::ATTRIBUTE_MAP{};
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        immutable_data["mint"] = itr->mint + 1;

        _avatars.modify(itr, eosio::same_payer, [&](auto& n) {
            n.mint += 1;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
            n.base_price = amf.next_base_price;
        });

        const std::vector<eosio::asset> tokens_to_back;
        const auto data = make_tuple(get_self(), cfg.collection_name, cfg.avatar_schema, itr->template_id, minter, immutable_data, mutable_data, tokens_to_back);
        eosio::action(eosio::permission_level{get_self(), "active"_n}, atomic_contract, "mintasset"_n, data).send();
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

        //add to queue
        _queue.emplace(get_self(), [&](auto& n) {
            n.id = _queue.available_primary_key();
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
        immutable_data["name"] = queue_entry->set_data.avatar_name;
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
            n.id = _avatars.available_primary_key();
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
    }

    void avatarmk_c::withdraw(const eosio::name& owner, const eosio::extended_asset& value)
    {
        require_auth(owner);
        eosio::check(value.quantity.amount > 0, "Withdraw amount must be possitive.");
        sub_balance(owner, value);
        token::actions::transfer{value.contract, get_self()}.send(get_self(), owner, value.quantity, "withdraw");
    }

    void avatarmk_c::open(const eosio::name& owner, eosio::extended_symbol& token, const eosio::name& ram_payer)
    {
        //can make ram_payer optional
        require_auth(ram_payer);
        eosio::check(is_account(owner), "Owner isn't associated with an on-chain account.");

        deposits_table _deposits(get_self(), owner.value);
        auto idx = _deposits.get_index<"bycontrsym"_n>();
        uint128_t composite_id = (uint128_t{token.contract.value} << 64) | token.get_symbol().raw();
        auto itr = idx.find(composite_id);
        eosio::check(itr == idx.end(), "Owner already has an open account for token");

        _deposits.emplace(ram_payer, [&](auto& n) {
            n.id = _deposits.available_primary_key();
            n.balance = eosio::extended_asset(0, token);
        });
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
        require_auth(owner);
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
        require_auth(rng_contract);
        RandomnessProvider RP(random_value);
        uint32_t random1 = RP.get_rand(200);
        uint32_t random2 = RP.get_rand(200);
        uint32_t random3 = RP.get_rand(200);

        unpack_table _unpack(get_self(), get_self().value);
        _unpack.emplace(get_self(), [&](auto& n) {
            n.pack_asset_id = assoc_id;
            n.claimable_template_ids.push_back(random1);
            n.claimable_template_ids.push_back(random2);
            n.claimable_template_ids.push_back(random3);
            n.inserted = eosio::time_point_sec(eosio::current_time_point());
        });
    }

#if defined(DEBUG)
    template <typename T>
    void cleanTable(eosio::name code, uint64_t account, const uint32_t batchSize)
    {
        T db(code, account);
        uint32_t counter = 0;
        auto itr = db.begin();
        while (itr != db.end() && counter++ < batchSize) {
            itr = db.erase(itr);
        }
    }
    void avatarmk_c::clravatars(eosio::name& scope) { cleanTable<avatars_table>(get_self(), scope.value, 100); }
    void avatarmk_c::clrparts(eosio::name& scope) { cleanTable<parts_table>(get_self(), scope.value, 200); }
    void avatarmk_c::clrqueue() { cleanTable<queue_table>(get_self(), get_self().value, 100); }
    void avatarmk_c::clrunpack() { cleanTable<unpack_table>(get_self(), get_self().value, 100); }
    void avatarmk_c::test(uint64_t id)
    {
        const auto tx_id = get_trx_id();
        uint64_t signing_value;
        memcpy(&signing_value, tx_id.data(), sizeof(signing_value));
        const auto data = std::make_tuple(id, signing_value, get_self());
        eosio::action({get_self(), "active"_n}, rng_contract, "requestrand"_n, data).send();
    }
#endif

}  // namespace avatarmk

EOSIO_ACTION_DISPATCHER(avatarmk::actions)

EOSIO_ABIGEN(
    // Include the contract actions in the ABI
    actions(avatarmk::actions),
    table("avatars"_n, avatarmk::avatars),
    table("queue"_n, avatarmk::queue),
    table("deposits"_n, avatarmk::deposits),
    table("config"_n, avatarmk::config),
    table("editions"_n, avatarmk::editions),
    table("parts"_n, avatarmk::parts),
    table("packs"_n, avatarmk::packs),
    table("unpack"_n, avatarmk::unpack))
#include "avatarmk.hpp"
#include "atomicassets.hpp"
#include "functions.cpp"
#include "notification_handlers.cpp"

namespace avatarmk {

    void avatarmk_c::addgroup(eosio::name& part_schema_name,
                              eosio::name& avatar_schema_name,
                              eosio::name& pack_schema_name,
                              eosio::asset& pack_base_price,
                              eosio::asset& floor_mint_price)
    {
        //warning no input validation!
        require_auth(get_self());
        schemacfg_table _schemacfg(get_self(), get_self().value);
        auto itr = _schemacfg.find(part_schema_name.value);
        eosio::check(itr == _schemacfg.end(), "part schema name already used to identify subcollection");

        _schemacfg.emplace(get_self(), [&](auto& n) {
            n.part_schema_name = part_schema_name;
            n.avatar_schema_name = avatar_schema_name;
            n.pack_schema_name = pack_schema_name;
            n.pack_base_price = pack_base_price;
            n.floor_mint_price = floor_mint_price;
        });
    }
    void avatarmk_c::delgroup(eosio::name& part_schema_name)
    {
        require_auth(get_self());
        schemacfg_table _schemacfg(get_self(), get_self().value);
        auto itr = _schemacfg.require_find(part_schema_name.value, "part_schema_name not in table");
        _schemacfg.erase(itr);
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
        eosio::check(itr->template_id > 0, "Avatar not finalized yet. Try again later.");

        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        schemacfg_table _schemacfg(get_self(), get_self().value);
        schemacfg scfg = _schemacfg.get(scope.value, "schema not found in schemacfg");

        //for now 100% of fee goes to template owner
        avatar_mint_fee amf = calculate_mint_price(*itr, scfg.floor_mint_price);  //not needed to pass in full config
        sub_balance(minter, amf.fee);
        add_balance(itr->creator, amf.fee, get_self());  //let self pay for ram if new table entry?

        //atomic mint action
        const auto blank_data = atomicassets::ATTRIBUTE_MAP{};
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        immutable_data["mint"] = itr->mint + 1;

        _avatars.modify(itr, eosio::same_payer, [&](auto& n) {
            n.mint += 1;
            n.modified = eosio::time_point_sec(eosio::current_time_point());
            n.base_price = amf.next_base_price;
        });

        const std::vector<eosio::asset> tokens_to_back;
        const auto data = make_tuple(get_self(), cfg.collection_name, scfg.avatar_schema_name, itr->template_id, minter, immutable_data, blank_data, tokens_to_back);
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
        eosio::check(ipfs_hash.size() > 0, "ipfs_hash required");

        queue_table _queue(get_self(), get_self().value);
        auto q_idx = _queue.get_index<eosio::name("byidf")>();
        auto queue_entry = q_idx.require_find(identifier, "Avatar with this identifier not in the queue.");

        eosio::name scope = queue_entry->scope;  // = part_schema_name

        avatars_table _avatars(get_self(), scope.value);
        auto a_idx = _avatars.get_index<eosio::name("byidf")>();
        eosio::check(a_idx.find(identifier) == a_idx.end(), "Avatar with this identifier already finalized.");

        //create template inline action
        config_table _config(get_self(), get_self().value);
        auto const cfg = _config.get_or_create(get_self(), config());

        const eosio::name authorized_creator = get_self();
        const eosio::name collection_name = cfg.collection_name;
        const eosio::name schema_name = scope;
        const bool transferable = true;
        const bool burnable = true;
        const uint32_t max_supply = queue_entry->set_data.max_mint;
        auto immutable_data = atomicassets::ATTRIBUTE_MAP{};
        //must match avatar schema!!!!!!!!
        immutable_data["name"] = queue_entry->set_data.avatar_name;
        immutable_data["img"] = ipfs_hash;
        immutable_data["rarityScore"] = queue_entry->set_data.rarity_score;
        immutable_data["bodyparts"] = queue_entry->set_data.template_ids;  //vector template_ids

        //add part names to immutable_data
        for (auto p : queue_entry->set_data.bodypart_names) {
            immutable_data[p.first] = p.second;
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
    void avatarmk_c::clrqueue() { cleanTable<queue_table>(get_self(), get_self().value, 100); }
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
    table("schemacfg"_n, avatarmk::schemacfg))
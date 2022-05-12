const conf = require('./eosioConfig')
const env = require('./.env.js')
const { api, tapos, doAction, authorization } = require('./lib/eosjs')()
const activeChain = process.env.CHAIN || env.defaultChain
const contractAccount = conf.accountName[activeChain]
const contract = require('./do.js')
const meta = require('./nftMetadata')
const collectionName = 'boidavatar13'
const defaultCollectionData = [
  { key: 'name', value: ['string', 'Boid Avatar NFTs'] },
  { key: 'img', value: ['string', 'QmZ4w4grmzJVDEr2mr9NNgegmsjzBJtyhw7TmbkPzJS1Pv'] },
  { key: 'description', value: ['string', 'Avatar NFTs for the Boid ecosystem.'] },
  { key: 'url', value: ['string', 'https://avatar.boid.com'] },
]

const methods = {
  async updateAuth(pubkey) {
    if (!pubkey) pubkey = "EOS6FoTSwiKk27SJ1kANdJFmso3KbECASAMDpEka4dG9p1ub6GqiH"
    const auth = {
      "threshold": 1,
      "keys": [{ key: pubkey, weight: 1 }],
      "accounts": [{ "permission": { "actor": contractAccount, "permission": "eosio.code" }, "weight": 1 }],
      "waits": []
    }
    await doAction('updateauth', { account: contractAccount, auth, parent: 'owner', permission: 'active' }, 'eosio', contractAccount)
  },
  async buyRam(bytes) {
    const data = {
      payer: contractAccount,
      receiver: contractAccount,
      bytes
    }
    await doAction('buyrambytes', data, 'eosio', contractAccount)
  },
  async powerUp(account = contractAccount, cpu_frac = 1e9) {
    const data = {
      "payer": account,
      "receiver": account,
      "days": 1,
      cpu_frac,
      "net_frac": 12076497,
      "max_payment": "0.1000 EOS"
    }
    await doAction('powerup', data, 'eosio', account)
  },
  async createCollection() {
    await doAction('createcol', {
      author: contractAccount,
      collection_name: collectionName,
      allow_notify: true,
      authorized_accounts: [contractAccount],
      notify_accounts: [contractAccount],
      market_fee: 0.1,
      data: defaultCollectionData,
    }, 'atomicassets', contractAccount)
  },
  async createSchemas() {
    const collection_name = collectionName
    const schemas = [
      {
        name: 'avatarparts',
        format: meta.part.schema
      },
      {
        name: 'boidavatars',
        format: meta.avatar.schema
      },
      {
        name: 'partpacks',
        format: meta.pack.schema
      }
    ]
    let actions = []
    for (const schemaData of schemas) {
      actions.push({
        name: "createschema",
        account: "atomicassets",
        authorization,
        data: {
          authorized_creator:contractAccount,
          collection_name,
          schema_name:schemaData.name,
          schema_format:schemaData.format
        },
      })
    }
    const result = await api.transact({ actions }, tapos)
    console.log(result.transaction_id)
  },
  async createPackTemplates(){
    const collection_name = collectionName
    const schema_name = 'partpacks'
    const templates = [
      {
        max_supply:3000,
        data:meta.pack.small
      },
      {
        max_supply:2000,
        data:meta.pack.medium
      },
      {
        max_supply:1000,
        data:meta.pack.large
      },
      {
        max_supply:500,
        data:meta.pack.rare
      },
      {
        max_supply:100,
        data:meta.pack.ultrarare
      },
    ]
    let actions = []
    for (const template of templates) {
      actions.push({
        name: "createtempl",
        account: "atomicassets",
        authorization,
        data: {
          authorized_creator: contractAccount,
          collection_name,
          schema_name,
          transferable: true,
          burnable: true,
          max_supply: template.max_supply,
          immutable_data:template.data
        },
      })
    }
    const result = await api.transact({ actions }, tapos)
    console.log(result.processed.action_traces.map(el=>({name:el.act.data.immutable_data[0].value[1], template_id: el.inline_traces[0].act.data.template_id})))
    console.log('use template_id from this result in registerPacks()')
  },
  async registerPacks() {
    const edition_scope = 'cartoon'
    const packs = [
      {
        name:"Small",
        template_id:421672,
        price:"0.10000000 WAX",
        rarity_distribution:[1, 4, 15, 30, 50]
      },
      {
        name:"Medium",
        template_id:421673,
        price:"0.15000000 WAX",
        rarity_distribution:[1, 4, 15, 30, 50]
      },
      {
        name:"Large",
        template_id:421674,
        price:"0.22000000 WAX",
        rarity_distribution:[1, 4, 15, 30, 50]
      },
      {
        name:"Rare",
        template_id:421675,
        price:"0.55000000 WAX",
        rarity_distribution:[5, 35, 50, 10, 0]
      },
      {
        name:"Ultra Rare",
        template_id:421676,
        price:"0.99000000 WAX",
        rarity_distribution:[10, 40, 50, 0, 0]
      },
    ]
    let actions = []
    for (const packData of packs) {
      actions.push({
        name: "packadd",
        account:contractAccount,
        authorization,
        data: {
          edition_scope,
          template_id:packData.template_id,
          base_price:packData.price,
          floor_price:packData.price,
          pack_name:packData.name,
          rarity_distribution:packData.rarity_distribution
        },
      })
    }
    const result = await api.transact({ actions }, tapos)
    console.log(result.transaction_id)
  },
}


if (require.main == module) {
  if (Object.keys(methods).find(el => el === process.argv[2])) {
    console.log("Starting:", process.argv[2])
    methods[process.argv[2]](...process.argv.slice(3)).catch((error) => console.error(error))
      .then((result) => console.log('Finished'))
  } else {
    console.log("Available Commands:")
    console.log(JSON.stringify(Object.keys(methods), null, 2))
  }
}
module.exports = methods

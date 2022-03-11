
const conf = require('./eosioConfig')
const env = require('./.env.js')
const { api, tapos, doAction, getFullTable } = require('./lib/eosjs')()
const activeChain = process.env.CHAIN || env.defaultChain
const defaultParams = { scope: conf.accountName[activeChain], code: conf.accountName[activeChain] }
const meta = require('./nftMetadata')
const defaultCollectionData = [
  { key: "name", value: ["string", "Boid Avatar NFTs"] },
  { key: "img", value: ["string", "QmZ4w4grmzJVDEr2mr9NNgegmsjzBJtyhw7TmbkPzJS1Pv"] },
  { key: "description", value: ["string", "Avatar NFTs for the Boid ecosystem."] },
  { key: "url", value: ["string", "https://boid.com"] }
]
const methods = {
  async mintMany(account = defaultParams.code, new_asset_owner = account) {
    const authorization = [{ actor: account, permission: 'active' }]
    let actions = []
    const data = {
      authorized_minter: account,
      collection_name: 'powerup.nfts',
      schema_name: 'elements',
      template_id: '3648',
      new_asset_owner: '.gems',
      immutable_data: [],
      mutable_data: [],
      tokens_to_back: []
    }
    let i = 0
    while (i < 33) {
      actions.push({ account: 'atomicassets', name: 'mintasset', data, authorization })
      i++
    }
    const result = await api.transact({ actions }, tapos).catch(err => console.error(err))
    console.log(result)
  },
  async burnAll(account = defaultParams.code) {
    const result = await getFullTable({ code: 'atomicassets', scope: account, table: 'assets' })
    console.log(result.length);
    const authorization = [{ actor: account, permission: 'active' }]
    let actions = []
    for (const nft of result) {
      const data = { account: 'atomicassets', name: 'burnasset', data: { asset_owner: account, asset_id: nft.asset_id }, authorization }
      actions.push(data)
      // console.log(data)
    }
    if (actions.length == 0) throw ('no NFTs to burn')
    const txresult = await api.transact({ actions }, tapos)
    console.log(txresult.transactionId);
  },
  async transfer(from, to, asset, memo) {
    await doAction('transfer', {
      from,
      to,
      asset_ids: [asset],
      memo
    }, 'atomicassets', from)
  },
  async transferMany(from, to, asset, memo) {
    await doAction('transfer', {
      from,
      to,
      asset_ids: [2199023260587, 2199023260588],
      memo
    }, 'atomicassets', from)
  },
  async createCollection() {
    await doAction('createcol', {
      author: defaultParams.code,
      collection_name: 'boidavatars1',
      allow_notify: true,
      authorized_accounts: [defaultParams.code],
      notify_accounts: [defaultParams.code],
      market_fee: 0.1,
      data: defaultCollectionData
    }, 'atomicassets', defaultParams.code)
  },
  async setCollectionData() {
    await doAction('setcoldata', {
      collection_name: "powerup.nfts",
      data: [
        { key: "name", value: ["string", "EOS PowerUp NFTs"] },
        { key: "img", value: ["string", "QmbncqcDy6pdNH6J7qTtdkmYrhyQ9BfaUwE4bbUPXURpE8"] },
        { key: "description", value: ["string", "NFTs created by eospowerup.io."] },
        { key: "url", value: ["string", "https://eospowerup.io"] }
      ]
    }, 'atomicassets', defaultParams.code)
  },
  async createSchema() {
    await doAction('createschema', {
      authorized_creator: defaultParams.code,
      collection_name: 'boidavatars1',
      schema_name: 'cartoonparts',
      schema_format: meta.cartoonparts.schema
    }, 'atomicassets', defaultParams.code)
  },
  async createTemplate() {
    await doAction('createtempl', {
      authorized_creator: defaultParams.code,
      collection_name: 'powerup.nfts',
      schema_name: "elements",
      transferable: true,
      burnable: true,
      max_supply: 0,
      immutable_data: meta.elements.gold
    }, 'atomicassets', defaultParams.code)
  },
  async mint() {
    await doAction('mintasset', {
      authorized_minter: defaultParams.code,
      collection_name: defaultParams.code,
      schema_name: 'elements',
      template_id: '3650',
      new_asset_owner: 'imjohnatboid',
      immutable_data: [],
      mutable_data: [],
      tokens_to_back: []
    }, 'atomicassets', defaultParams.code)
  },
  async burn(asset_owner, asset_id) {
    const data = {
      asset_owner,
      asset_id
    }
    await doAction('burnasset', data, 'atomicassets', asset_owner)
  }
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

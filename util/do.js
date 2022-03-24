const conf = require('./eosioConfig')
const env = require('./.env.js')
const utils = require('@deltalabs/eos-utils')
const { api, tapos, doAction } = require('./lib/eosjs')()
const activeChain = process.env.CHAIN || env.defaultChain
const contractAccount = conf.accountName[activeChain]

const methods = {
  async setconfig() {
    let cfg = null
    // let cfg = {
    //   freeze: false,
    //   collection_name: "boidavatars1",
    //   parts_schema: "avatarparts",
    //   avatar_schema: "testavatarsc",
    //   pack_schema: "partspacksch"
    // }
    await doAction('setconfig', { cfg })
  },
  async finalize() {
    await doAction('finalize', {
      identifier: '45529d3195e7fe611cba650f994b60ec8b7c3d4a1ec2a61a1aa79b39dfdd6f48',
      ipfs_hash: 'Qmbv7MCgcBUU2FezxCZ59u7uCm947cECHAwoJyEAfH2hoh'
    })
  },
}


if (require.main == module) {
  if (Object.keys(methods).find(el => el === process.argv[2])) {
    console.log('Starting:', process.argv[2])
    methods[process.argv[2]](...process.argv.slice(3)).catch(error => console.error(error))
      .then(result => console.log('Finished'))
  } else {
    console.log('Available Commands:')
    console.log(JSON.stringify(Object.keys(methods), null, 2))
  }
}
module.exports = methods

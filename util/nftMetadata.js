const nftMetadata = {
  part: {
    schema: [
      { name: 'name', type: 'string' },
      { name: 'edition', type: 'string' },
      { name: 'rarityScore', type: 'uint8' },
      { name: 'bodypart', type: 'string' },
      { name: 'info', type: 'string' },
      { name: 'url', type: 'string' },
      { name: 'img', type: 'image' }
    ]
  },
  avatar:{
    schema:[
      {name:'name', type:'string'},
      {name:'edition', type:'string'},
      {name:'rarityScore', type: 'uint8' },
      {name:'mint', type: 'uint64' },
      {name:'head', type:'string'},
      {name:'eyes', type:'string'},
      {name:'mouth', type:'string'},
      {name:'top', type:'string'},//old hair
      {name:'torso', type:'string'},//old top
      {name:'legs', type:'string'},//old bottom
      {name:'big-wings', type:'string'},
      {name:'small-wings', type:'string'},
      {name:'bodyparts', type:'uint32[]'},
      {name:'img', type:'image'}
    ]
  },
  pack:{
    schema:[
      {name:'name', type:'string'},
      {name:'edition', type:'string'},
      {name:'size', type:'uint8'},
      {name:'img', type:'image'}
    ],
    small:[
      { key: "name", value: ["string", "Small Pack"] },
      { key: "edition", value: ["string", "cartoon"] },
      { key: "size", value: ["uint8", 10] },
    ],
    medium:[
      { key: "name", value: ["string", "Medium Pack"] },
      { key: "edition", value: ["string", "cartoon"] },
      { key: "size", value: ["uint8", 20] },
    ],
    large:[
      { key: "name", value: ["string", "Large Pack"] },
      { key: "edition", value: ["string", "cartoon"] },
      { key: "size", value: ["uint8", 30] },
    ]
  },
  genesis: {
    schema: [
      { name: 'name', type: 'string' },
      { name: 'rarity', type: 'string' },
      { name: 'details', type: 'string' },
      { name: 'url', type: 'string' },
      { name: 'video', type: 'ipfs' },
    ],
    bronze: [
      { key: "name", value: ["string", "EOS PowerUp Genesis Bronze"] },
      { key: "rarity", value: ["string", "Bronze"] },
      { key: "details", value: ["string", "This Bronze Genesis NFT is awarded to you in recognition of your early support of EOS PowerUp."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmNcncNUjyetvWHWGFcBeWsC8uZVvuidYrshNCRAcS6jrT"] },
    ],
    silver: [
      { key: "name", value: ["string", "EOS PowerUp Genesis Silver"] },
      { key: "rarity", value: ["string", "Silver"] },
      { key: "details", value: ["string", "This Silver Genesis NFT is awarded to you in recognition of your early support of EOS PowerUp."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmUiAzVE4WeA3behh16ZdDSxgJEQSeQ56NhGyFBy7ZzXbj"] },
    ],
    gold: [
      { key: "name", value: ["string", "EOS PowerUp Genesis Gold"] },
      { key: "rarity", value: ["string", "Gold"] },
      { key: "details", value: ["string", "This Gold Genesis NFT is awarded to you in recognition of your early support of EOS PowerUp."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmRjyxDXxxsA4eVkt2q7xkq9QVaeGUnBRWaxY13Ac7pEgc"] },
    ],
  },
  elements: {
    schema: [
      { name: 'name', type: 'string' },
      { name: 'rarity', type: 'string' },
      { name: 'details', type: 'string' },
      { name: 'url', type: 'string' },
      { name: 'video', type: 'ipfs' },
    ],
    bronze: [
      { key: "name", value: ["string", "EOS PowerUp Utility Bronze"] },
      { key: "rarity", value: ["string", "Bronze"] },
      { key: "details", value: ["string", "This Bronze Utility NFT unlocks additional free PowerUps on the eospowerup.io homepage."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmNuTcoZzu9kVRmTxCaprund9vVXofJYnGYEeyCEeTU8dD"] },
    ],
    silver: [
      { key: "name", value: ["string", "EOS PowerUp Utility Silver"] },
      { key: "rarity", value: ["string", "Silver"] },
      { key: "details", value: ["string", "This Silver Utility NFT unlocks referral link functionality on the eospowerup.io service."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmRohf2G82CPVoevY3NuCFUtvJqc8oDb3nUwAYj57iqy5k"] },
    ],
    gold: [
      { key: "name", value: ["string", "EOS PowerUp Utility Gold"] },
      { key: "rarity", value: ["string", "Gold"] },
      { key: "details", value: ["string", "This Gold Utility NFT removes all platform fees when receiving PowerUps from the eospowerup.io automated PowerUp service."] },
      { key: "url", value: ["string", "https://eospowerup.io/nft"] },
      { key: "video", value: ["string", "QmVPEQSAoC8qy5kmYDyY8MfWH71uUjb3hbRWdRGiNLF7HM"] },
    ],
  }
}

module.exports = nftMetadata

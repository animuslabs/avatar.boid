const nftMetadata = {
  cartoonparts: {
    schema: [
      { name: 'name', type: 'string' },
      { name: 'rarity', type: 'string' },
      { name: 'bodypart', type: 'string' },
      { name: 'url', type: 'string' },
      { name: 'card', type: 'ipfs' },
      { name: 'img', type: 'ipfs' },
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

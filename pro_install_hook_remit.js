const xahau = require('xahau');
const fs = require('fs');
const crypto = require('crypto');

const seed = 'yourSeed';
const network = "wss://xahau.network";

async function connectAndQuery() {
  const client = new xahau.Client(network);
  const my_wallet = xahau.Wallet.fromSeed(seed, {algorithm: 'secp256k1'});
  console.log(`Account: ${my_wallet.address}`);

  try {
    await client.connect();
    console.log('Connected to Xahau');

    const prepared = await client.autofill({
      "TransactionType": "SetHook",
      "Account": my_wallet.address,
      "Flags": 0,
      "Hooks": [
        {
          "Hook": {
            "HookHash": "C0C0C581C3F05BE7EA568D6FEC8C8E89C5BFB35537483ED696B608BC17CFB1D4",
            "HookOn": 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBFFFFE', //Only Payments https://richardah.github.io/xrpl-hookon-calculator/
            "HookCanEmit": "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFFFFFFFFFFBFFFFF", //Can emit remits
            "HookNamespace": crypto.createHash('sha256').update('remit').digest('hex').toUpperCase(),
            "Flags": 1,
            "HookParameters": [
              {
              "HookParameter": {
              "HookParameterName": "555249",
              "HookParameterValue": "697066733A2F2F62616679626569646278636A7261636B6E746668736A35686835776D6777346472616C6A6E706F3574327A7336623669346E6262696F6A616873712F616E6369656E7443727970742E6A736F6E"
              }
              },
              {
              "HookParameter": {
              "HookParameterName": "414D4F554E54",
              "HookParameterValue": "0000000000989680"
              }
              }
              ]
          }
        }
      ],
    });

    const { tx_blob } = my_wallet.sign(prepared);
    const tx = await client.submitAndWait(tx_blob);
    console.log("Info tx:", JSON.stringify(tx, null, 2));
  } catch (error) {
    console.error('Error:', error);
  } finally {
    await client.disconnect();
    console.log('Disconnecting from Xahau node');
  }
}

connectAndQuery();

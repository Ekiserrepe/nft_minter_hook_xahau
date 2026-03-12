# NFT Minter Hook — Xahau (Remit)

A Xahau Hook written in C that automatically mints and sends a URIToken (NFT) to any account that sends an exact XAH payment to the hook account. The token URI is built dynamically by appending the sender's account ID (as hex) to a base IPFS URI configured at install time.

> Based on the original work by [@Handy_4ndy](https://x.com/Handy_4ndy) in [XahauHooks101](https://github.com/Handy4ndy/XahauHooks101).

---

## How it works

1. A user sends a **Payment** transaction (XAH only) to the hook account with the exact amount configured in the `AMOUNT` parameter.
2. The hook validates the payment type and amount.
3. It builds a final URI by concatenating the base URI from the `URI` parameter with `?a=<sender_account_hex>`.
4. It emits a **Remit** transaction back to the sender, minting a URIToken to their account with the dynamically built URI.

The hook rejects:
- Any transaction type other than Payment.
- IOU / token payments (only native XAH is accepted).
- Payments that do not match the exact required amount.
- Missing or invalid `URI` / `AMOUNT` hook parameters.

---

## Hook Parameters

Both parameters must be set at install time as part of the `SetHook` transaction.

| Parameter | Key (hex) | Key (ASCII) | Description |
|-----------|-----------|-------------|-------------|
| `URI`     | `555249`  | `URI`       | Base IPFS (or any) URI for the URIToken metadata. The hook appends `?a=<sender_account_hex>` automatically. Max 200 bytes. |
| `AMOUNT`  | `414D4F554E54` | `AMOUNT` | Exact XAH amount (in drops) required for minting. Encoded as **8-byte big-endian unsigned integer**. |

### Example values (used in the scripts)

| Parameter | Hex Value | Decoded Value |
|-----------|-----------|---------------|
| `URI`     | `697066733A2F2F62616679626569646278636A7261636B6E746668736A35686835776D6777346472616C6A6E706F3574327A7336623669346E6262696F6A616873712F616E6369656E7443727970742E6A736F6E` | `ipfs://bafybeidbxcjrackntfhsj5hh5wmgw4draljnpo5t2zs6b6i4nbbiojahsq/ancientCrypt.json` |
| `AMOUNT`  | `0000000000989680` | `10000000` drops = **10 XAH** |

### Encoding your own values

**URI** — convert the UTF-8 string to hex:
```js
Buffer.from('ipfs://your-cid/metadata.json').toString('hex').toUpperCase()
```

**AMOUNT** — encode drops as an 8-byte big-endian buffer:
```js
const drops = BigInt(10_000_000); // 10 XAH
drops.toString(16).padStart(16, '0').toUpperCase()
// => "0000000000989680"
```

---

## Hook Hash

The compiled WASM has been deployed on-chain. Its hash is:

```
C0C0C581C3F05BE7EA568D6FEC8C8E89C5BFB35537483ED696B608BC17CFB1D4
```

This hash is the same on **both Xahau Testnet and Xahau Mainnet**, allowing installation via `HookHash` without re-uploading the WASM bytecode.

---

## Installation

### Prerequisites

```bash
npm install xahau
```

Make sure you have:
- A funded Xahau account (Testnet or Mainnet).
- The account's seed (family seed format, `secp256k1`).

### Option A — Install from HookHash (recommended)

Use this method to install the hook without uploading the WASM. It works on both **Testnet** and **Mainnet** because the hook is already deployed on-chain.

**Files:**
- Testnet: `pre_install_hook_remit.js`
- Mainnet: `pro_install_hook_remit.js`

1. Open the relevant file and replace `'yourSeed'` with your account seed:
   ```js
   const seed = 'sYourAccountSeedHere';
   ```
2. Optionally update `HookParameterValue` for `URI` and `AMOUNT` to match your project.
3. Run:
   ```bash
   # Testnet
   node pre_install_hook_remit.js

   # Mainnet
   node pro_install_hook_remit.js
   ```

### Option B — Deploy from source WASM

Use this method if you want to upload your own compiled WASM binary.

**Files:**
- Testnet: `pre_create_hook_remit.js`
- Mainnet: `pro_create_hook_remit.js`

1. Place your compiled `remit.wasm` in the same directory.
2. Replace `'yourSeed'` with your account seed.
3. Optionally update the `URI` and `AMOUNT` parameter values.
4. Run:
   ```bash
   # Testnet
   node pre_create_hook_remit.js

   # Mainnet
   node pro_create_hook_remit.js
   ```

---

## Network endpoints

| Network | WebSocket |
|---------|-----------|
| Xahau Mainnet | `wss://xahau.network` |
| Xahau Testnet | `wss://xahau-test.net` |

---

## Hook configuration

| Field | Value | Notes |
|-------|-------|-------|
| `HookOn` | `FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBFFFFE` | Fires on Payment transactions only |
| `HookCanEmit` | `FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFFFFFFFFFFBFFFFF` | Allows emitting Remit transactions |
| `HookNamespace` | SHA-256 of `"remit"` | Isolates hook state |
| `Flags` | `1` (`hsfOverride`) | Allows replacing an existing hook |
| `HookApiVersion` | `0` | |

## Disclaimer

This hook is only for educational purposes. I am not responsible for any malfunction it may cause. Review and modify the hook as you wish.

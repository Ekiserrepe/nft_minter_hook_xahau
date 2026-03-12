#include "hookapi.h"

#define MAX_URI_LEN 200
#define MAX_FINAL_URI_LEN 255

// Macro to copy account ID to buffer
#define ACCOUNT_TO_BUF(buf_raw, i)\
{\
    unsigned char* buf = (unsigned char*)buf_raw;\
    *(uint64_t*)(buf + 0) = *(uint64_t*)(i +  0);\
    *(uint64_t*)(buf + 8) = *(uint64_t*)(i +  8);\
    *(uint32_t*)(buf + 16) = *(uint32_t*)(i + 16);\
}

// Fixed macro to set vlURI (len, data, end marker 0xE1)
#define URI_TO_BUF(buf_raw, uri, len) \
{ \
    unsigned char* buf = (unsigned char*)buf_raw; \
    buf[0] = (uint8_t)(len); \
    for (int i = 0; GUARD(MAX_FINAL_URI_LEN), i < (int)(len); ++i) \
        buf[1 + i] = uri[i]; \
    buf[1 + (len)] = 0xE1U; \
}

// Convert 20 bytes of AccountID to 40 chars hex ASCII
static int append_hex20(uint8_t *out, const uint8_t *acc20)
{
    const uint8_t hex[] = "0123456789ABCDEF";

    for (int i = 0; GUARD(20), i < 20; ++i)
    {
        out[i * 2]     = hex[(acc20[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[(acc20[i] >> 0) & 0x0F];
    }

    return 40;
}

// Reads AMOUNT value (8 bytes big-endian uint64, drops value)
// and returns as int64_t. Returns -1 if it doesnt exists or invalid.
static int64_t read_amount_param(void)
{
    uint8_t key[] = {'A','M','O','U','N','T'};
    uint8_t buf[8];

    int64_t len = hook_param(buf, sizeof(buf), key, sizeof(key));
    if (len != 8)
        return -1;

    // Decodes big-endian uint64
    uint64_t drops =
        ((uint64_t)buf[0] << 56) |
        ((uint64_t)buf[1] << 48) |
        ((uint64_t)buf[2] << 40) |
        ((uint64_t)buf[3] << 32) |
        ((uint64_t)buf[4] << 24) |
        ((uint64_t)buf[5] << 16) |
        ((uint64_t)buf[6] <<  8) |
        ((uint64_t)buf[7]);

    return (int64_t)drops;
}

// Remit tx skeleton (clang-format off)
uint8_t txn[1000] =
{
/* size,upto */
/*   3,   0 */   0x12U, 0x00U, 0x5FU,
/*   5,   3 */   0x22U, 0x80U, 0x00U, 0x00U, 0x00U,
/*   5,   8 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   5,  13 */   0x99U, 0x99U, 0x99U, 0x99U, 0x99U,
/*   6,  18 */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/*   6,  24 */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/*   9,  30 */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*  35,  39 */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*  22,  74 */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*  22,  96 */   0x83U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/* 116, 118 */   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

/*   3, 234 */  0xE0U, 0x5CU, 0x75U,
/*   1, 237 */  0xE1U,
/*   0, 238 */
};
// clang-format on

// Tx builder defines
#define BYTES_LEN 238U
#define FLS_OUT   (txn + 20U)
#define LLS_OUT   (txn + 26U)
#define DTAG_OUT  (txn + 14U)
#define FEE_OUT   (txn + 31U)
#define HOOK_ACC  (txn + 76U)
#define OTX_ACC   (txn + 98U)
#define URI_OUT   (txn + 237U)
#define EMIT_OUT  (txn + 118U)

// Prepare Remit macro
#define PREPARE_REMIT_TXN(hook_acc, dest_acc, uri, uri_len) do { \
    etxn_reserve(1); \
    if (otxn_field(DTAG_OUT, 4, sfSourceTag) == 4) \
        *(DTAG_OUT - 1) = 0x2EU; \
    uint32_t fls = (uint32_t)ledger_seq() + 1; \
    *((uint32_t *)(FLS_OUT)) = FLIP_ENDIAN_32(fls); \
    uint32_t lls = fls + 4; \
    *((uint32_t *)(LLS_OUT)) = FLIP_ENDIAN_32(lls); \
    ACCOUNT_TO_BUF(HOOK_ACC, hook_acc); \
    ACCOUNT_TO_BUF(OTX_ACC, dest_acc); \
    URI_TO_BUF(URI_OUT, uri, uri_len); \
    etxn_details(EMIT_OUT, 116U); \
    int64_t fee = etxn_fee_base(txn, BYTES_LEN + uri_len + 1); \
    uint8_t *b = FEE_OUT; \
    *b++ = 0b01000000 + ((fee >> 56) & 0b00111111); \
    *b++ = (fee >> 48) & 0xFFU; \
    *b++ = (fee >> 40) & 0xFFU; \
    *b++ = (fee >> 32) & 0xFFU; \
    *b++ = (fee >> 24) & 0xFFU; \
    *b++ = (fee >> 16) & 0xFFU; \
    *b++ = (fee >> 8) & 0xFFU; \
    *b++ = (fee >> 0) & 0xFFU; \
} while(0)

int64_t hook(uint32_t reserved)
{
    TRACESTR("HUR :: URI + otx_acc Remit :: Called.");

    // Hook account
    uint8_t hook_acc[20];
    if (hook_account(hook_acc, 20) != 20)
        rollback(SBUF("HUR :: Error :: hook_account failed."), __LINE__);

    // Origin tx account (payer)
    uint8_t otx_acc[20];
    if (otxn_field(otx_acc, 20, sfAccount) != 20)
        rollback(SBUF("HUR :: Error :: sfAccount read failed."), __LINE__);

    // Only Payments
    int64_t tt = otxn_type();
    if (tt != 0)
        accept(SBUF("HUR :: Not a Payment. Accepting."), __LINE__);

    int equal = 0;
    BUFFER_EQUAL(equal, hook_acc, otx_acc, 20);

    // If destination is NOT the hook account, allow the tx
    if (equal)
        accept(SBUF("HUR :: Outgoing or unrelated Payment. Accepting."), __LINE__);

    // ── AMOUNT validation ────────────────────────────────────────────────────
    // Read the AMOUNT (drops required, big-endian uint64, 8 bytes)
    int64_t required_drops = read_amount_param();
    if (required_drops < 0)
        rollback(SBUF("HUR :: Error :: Hook Parameter 'AMOUNT' not found or invalid."), __LINE__);

    // Read the incoming payment Amount
    uint8_t amount_buf[8];
    int64_t amount_len = otxn_field(SBUF(amount_buf), sfAmount);

    // sfAmount == 8 bytes => XAH payment. If its IOU (48 bytes), rollback.
    if (amount_len != 8)
        rollback(SBUF("HUR :: Error :: Payment must be native XAH, not IOU."), __LINE__);

    // Extracts drops from buffer 
    int64_t incoming_drops = AMOUNT_TO_DROPS(amount_buf);

    TRACEVAR(required_drops);
    TRACEVAR(incoming_drops);

    // Compares the amounts
    if (incoming_drops != required_drops)
        rollback(SBUF("HUR :: Error :: Payment amount does not match required AMOUNT."), __LINE__);

    // ── URI parameter ────────────────────────────────────────────────────────
    // Read install-time parameter "URI"
    uint8_t uri_key[] = {'U','R','I'};
    uint8_t base_uri[MAX_URI_LEN];

    int64_t base_len = hook_param(base_uri, sizeof(base_uri), uri_key, sizeof(uri_key));
    if (base_len <= 0)
        rollback(SBUF("HUR :: Error :: Hook Parameter 'URI' not found."), __LINE__);

    if (base_len > MAX_URI_LEN)
        rollback(SBUF("HUR :: Error :: Base URI too long."), __LINE__);

    // Build final URI = base_uri + "?a=" + hex(otx_acc)
    uint8_t final_uri[MAX_FINAL_URI_LEN];
    int64_t final_len = 0;

    for (int i = 0; GUARD(MAX_URI_LEN), i < base_len; ++i)
        final_uri[final_len++] = base_uri[i];

    if (final_len + 3 + 40 > MAX_FINAL_URI_LEN)
        rollback(SBUF("HUR :: Error :: Final URI too long."), __LINE__);

    final_uri[final_len++] = '?';
    final_uri[final_len++] = 'a';
    final_uri[final_len++] = '=';

    final_len += append_hex20(final_uri + final_len, otx_acc);

    trace(SBUF("HUR :: Base URI: "), base_uri, base_len, 0);
    trace(SBUF("HUR :: Final URI: "), final_uri, final_len, 0);

    // Prepare Remit tx
    PREPARE_REMIT_TXN(hook_acc, otx_acc, final_uri, final_len);

    // Emit tx
    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), txn, BYTES_LEN + final_len + 1);

    if (emit_result > 0)
        accept(SBUF("HUR :: Success :: URIToken minted!"), __LINE__);

    rollback(SBUF("HUR :: Error :: Emit failed."), __LINE__);
    return 0;
}

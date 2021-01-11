//
//  BRWallet.h
//
//  Created by Aaron Voisine on 9/1/15.
//  Copyright (c) 2015 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRWallet_h
#define BRWallet_h

#include "BRTransaction.h"
#include "BRAddress.h"
#include "BRBIP32Sequence.h"
#include "BRInt.h"
#include <string.h>

#define wallet_log(...) _wallet_log("%s:%"PRIu16" " _va_first(__VA_ARGS__, NULL) "\n", _va_rest(__VA_ARGS__, NULL))
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__

#if defined(TARGET_OS_MAC)
#include <Foundation/Foundation.h>
#define _wallet_log(...) NSLog(__VA_ARGS__)
#elif defined(__ANDROID__)
#include <android/log.h>
#define _wallet_log(...) __android_log_print(ANDROID_LOG_DEBUG, "noirwallet", __VA_ARGS__)
#else
#include <stdio.h>
    #ifdef DEBUG
        #define _wallet_log(...) printf(__VA_ARGS__)
    #else
        #define _wallet_log(...)
    #endif
#endif

#if defined(TARGET_OS_MAC)
    #include <Foundation/Foundation.h>
    #define debug_log(...) NSLog(__VA_ARGS__)
#elif defined(__ANDROID__)
    #include <android/log.h>
    #define debug_log(...) __android_log_print(ANDROID_LOG_DEBUG, "noirwallet", __VA_ARGS__)
#else
    #include <stdio.h>
    #ifdef DEBUG
        #define debug_log(...) printf(__VA_ARGS__)
    #else
        #define debug_log(...)
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FEE_PER_KB ((5000ULL*1000 + 99)/100) // bitcoind 0.11 min relay fee on 100bytes
#define MIN_FEE_PER_KB     ((TX_FEE_PER_KB*1000 + 190)/191) // minimum relay fee on a 191byte tx
#define MAX_FEE_PER_KB     ((1000100ULL*1000 + 190)/191) // slightly higher than a 10000bit fee on a 191byte tx

typedef struct {
    UInt256 hash;
    uint32_t n;
} BRUTXO;
    


inline static size_t BRUTXOHash(const void *utxo)
{
    // (hash xor n)*FNV_PRIME
    return (size_t)((((const BRUTXO *)utxo)->hash.u32[0] ^ ((const BRUTXO *)utxo)->n)*0x01000193);
}

inline static int BRUTXOEq(const void *utxo, const void *otherUtxo)
{
    return (utxo == otherUtxo || (UInt256Eq(((const BRUTXO *)utxo)->hash, ((const BRUTXO *)otherUtxo)->hash) &&
                                  ((const BRUTXO *)utxo)->n == ((const BRUTXO *)otherUtxo)->n));
}

typedef struct BRWalletStruct BRWallet;
    
// allocates and populates a BRWallet struct that must be freed by calling BRWalletFree()
BRWallet *BRWalletNew(BRTransaction *transactions[], size_t txCount, BRMasterPubKey mpk);

// not thread-safe, set callbacks once after BRWalletNew(), before calling other BRWallet functions
// info is a void pointer that will be passed along with each callback call
// void balanceChanged(void *, uint64_t) - called when the wallet balance changes
// void txAdded(void *, BRTransaction *) - called when transaction is added to the wallet
// void txUpdated(void *, const UInt256[], size_t, uint32_t, uint32_t)
//   - called when the blockHeight or timestamp of previously added transactions are updated
// void txDeleted(void *, UInt256, int, int) - called when a previously added transaction is removed from the wallet
void BRWalletSetCallbacks(BRWallet *wallet, void *info,
                          void (*balanceChanged)(void *info, uint64_t balance),
                          void (*txAdded)(void *info, BRTransaction *tx),
                          void (*txUpdated)(void *info, const UInt256 txHashes[], size_t txCount, uint32_t blockHeight,
                                            uint32_t timestamp),
                          void (*txDeleted)(void *info, UInt256 txHash, int notifyUser, int recommendRescan));

// wallets are composed of chains of addresses
// each chain is traversed until a gap of a number of addresses is found that haven't been used in any transactions
// this function writes to addrs an array of <gapLimit> unused addresses following the last used address in the chain
// the internal chain is used for change addresses and the external chain for receive addresses
// addrs may be NULL to only generate addresses for BRWalletContainsAddress()
// returns the number addresses written to addrs
size_t BRWalletUnusedAddrs(BRWallet *wallet, BRAddress addrs[], uint32_t gapLimit, int internal);

// returns the first unused external address
BRAddress BRWalletReceiveAddress(BRWallet *wallet);

// writes all addresses previously genereated with BRWalletUnusedAddrs() to addrs
// returns the number addresses written, or total number available if addrs is NULL
size_t BRWalletAllAddrs(BRWallet *wallet, BRAddress addrs[], size_t addrsCount);

// true if the address was previously generated by BRWalletUnusedAddrs() (even if it's now used)
int BRWalletContainsAddress(BRWallet *wallet, const char *addr);

// true if the address was previously used as an input or output in any wallet transaction
int BRWalletAddressIsUsed(BRWallet *wallet, const char *addr);

// writes transactions registered in the wallet, sorted by date, oldest first, to the given transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BRWalletTransactions(BRWallet *wallet, BRTransaction *transactions[], size_t txCount);

// writes transactions registered in the wallet, and that were unconfirmed before blockHeight, to the transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BRWalletTxUnconfirmedBefore(BRWallet *wallet, BRTransaction *transactions[], size_t txCount,
                                   uint32_t blockHeight);

// current wallet balance, not including transactions known to be invalid
uint64_t BRWalletBalance(BRWallet *wallet);

// total amount spent from the wallet (exluding change)
uint64_t BRWalletTotalSent(BRWallet *wallet);

// total amount received by the wallet (exluding change)
uint64_t BRWalletTotalReceived(BRWallet *wallet);

// writes unspent outputs to utxos and returns the number of outputs written, or number available if utxos is NULL
size_t BRWalletUTXOs(BRWallet *wallet, BRUTXO utxos[], size_t utxosCount);

// fee-per-kb of transaction size to use when creating a transaction
uint64_t BRWalletFeePerKb(BRWallet *wallet);
void BRWalletSetFeePerKb(BRWallet *wallet, uint64_t feePerKb);

// returns an unsigned transaction that sends the specified amount from the wallet to the given address
// result must be freed using BRTransactionFree()
BRTransaction *BRWalletCreateTransaction(BRWallet *wallet, uint64_t amount, const char *addr);

// returns an unsigned transaction that satisifes the given transaction outputs
// result must be freed using BRTransactionFree()
BRTransaction *BRWalletCreateTxForOutputs(BRWallet *wallet, const BRTxOutput outputs[], size_t outCount);

// signs any inputs in tx that can be signed using private keys from the wallet
// forkId is 0 for bitcoin, 0x40 for b-cash
// seed is the master private key (wallet seed) corresponding to the master public key given when the wallet was created
// returns true if all inputs were signed, or false if there was an error or not all inputs were able to be signed
int BRWalletSignTransaction(BRWallet *wallet, BRTransaction *tx, int forkId, const void *seed, size_t seedLen);

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BRWalletContainsTransaction(BRWallet *wallet, const BRTransaction *tx);

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BRWalletRegisterTransaction(BRWallet *wallet, BRTransaction *tx);

// removes a tx from the wallet and calls BRTransactionFree() on it, along with any tx that depend on its outputs
void BRWalletRemoveTransaction(BRWallet *wallet, UInt256 txHash);

// returns the transaction with the given hash if it's been registered in the wallet
BRTransaction *BRWalletTransactionForHash(BRWallet *wallet, UInt256 txHash);

// true if no previous wallet transaction spends any of the given transaction's inputs, and no inputs are invalid
int BRWalletTransactionIsValid(BRWallet *wallet, const BRTransaction *tx);

// true if transaction cannot be immediately spent (i.e. if it or an input tx can be replaced-by-fee)
int BRWalletTransactionIsPending(BRWallet *wallet, const BRTransaction *tx);

// true if tx is considered 0-conf safe (valid and not pending, timestamp is greater than 0, and no unverified inputs)
int BRWalletTransactionIsVerified(BRWallet *wallet, const BRTransaction *tx);

void BRFixAssetInputs(BRWallet *wallet, BRTransaction *assetTransaction);

// set the block heights and timestamps for the given transactions
// use height TX_UNCONFIRMED and timestamp 0 to indicate a tx should remain marked as unverified (not 0-conf safe)
void BRWalletUpdateTransactions(BRWallet *wallet, const UInt256 txHashes[], size_t txCount, uint32_t blockHeight,
                                uint32_t timestamp);
    
// marks all transactions confirmed after blockHeight as unconfirmed (useful for chain re-orgs)
void BRWalletSetTxUnconfirmedAfter(BRWallet *wallet, uint32_t blockHeight);

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
uint64_t BRWalletAmountReceivedFromTx(BRWallet *wallet, const BRTransaction *tx);

// returns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BRWalletAmountSentByTx(BRWallet *wallet, const BRTransaction *tx);

// returns the fee for the given transaction if all its inputs are from wallet transactions, UINT64_MAX otherwise
uint64_t BRWalletFeeForTx(BRWallet *wallet, const BRTransaction *tx);

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BRWalletBalanceAfterTx(BRWallet *wallet, const BRTransaction *tx);

// fee that will be added for a transaction of the given size in bytes
uint64_t BRWalletFeeForTxSize(BRWallet *wallet, size_t size);

// fee that will be added for a transaction of the given amount
uint64_t BRWalletFeeForTxAmount(BRWallet *wallet, uint64_t amount);

// outputs below this amount are uneconomical due to fees (TX_MIN_OUTPUT_AMOUNT is the absolute minimum output amount)
uint64_t BRWalletMinOutputAmount(BRWallet *wallet);

// maximum amount that can be sent from the wallet to a single address after fees
uint64_t BRWalletMaxOutputAmount(BRWallet *wallet);

// frees memory allocated for wallet, and calls BRTransactionFree() for all registered transactions
void BRWalletFree(BRWallet *wallet);

// returns the given amount (in satoshis) in local currency units (i.e. pennies, pence)
// price is local currency units per bitcoin
int64_t BRLocalAmount(int64_t amount, double price);

// returns the given local currency amount in satoshis
// price is local currency units (i.e. pennies, pence) per bitcoin
int64_t BRBitcoinAmount(int64_t localAmount, double price);

/*
              DIGI-
      _                _
     /_\  ___ ___  ___| |_ ___
    //_\\/ __/ __|/ _ \ __/ __|
   /  _  \__ \__ \  __/ |_\__ \
   \_/ \_/___/___/\___|\__|___/
 */

// First word must be zero, second must not be zero
#define DA_IS_ISSUANCE(byte) ((~(byte) & 0xF0) && ((byte) & 0x0F))
// First word must be 1
#define DA_IS_TRANSFER(byte) ((byte) & 0x10)
// First word must be 2
#define DA_IS_BURN(byte)     ((byte) & 0x20)

#define DA_TYPE_SHA1_META_SHA256      0x01
#define DA_TYPE_SHA1_MS12_SHA256      0x02
#define DA_TYPE_SHA1_MS13_SHA256      0x03
#define DA_TYPE_SHA1_META             0x04
#define DA_TYPE_SHA1_NO_META_LOCKED   0x05
#define DA_TYPE_SHA1_NO_META_UNLOCKED 0x06

#define DA_ASSET_DUST_AMOUNT 700

typedef enum {
    DA_UNDEFINED,
    DA_ISSUANCE,
    DA_TRANSFER,
    DA_BURN
} BRAssetOperation;

typedef struct {
    uint16_t info_hash[20];
    uint16_t metadata[32];
    
    uint8_t version;
    uint8_t has_metadata;
    uint8_t has_infohash;
    BRAssetOperation type;
    uint8_t locked;
} BRAssetData;

BRUTXO * BRGetUTXO(BRWallet *wallet);

BRTransaction * BRGetTxForUTXO(BRWallet *wallet, BRUTXO utxo);

uint8_t BRTXContainsAsset(BRTransaction *tx);

uint8_t BRContainsAsset(const BRTxOutput *outputs, size_t outCount);

uint8_t BROutpointIsAsset(const BRTxOutput* output);

BRTransaction* BRGetTransactions(BRWallet *wallet);

uint8_t BROutputSpendable(BRWallet *wallet, const BRTxOutput output);

void BRWalletUpdateBalance(BRWallet *wallet);

#ifdef __cplusplus
}
#endif

#endif // BRWallet_h

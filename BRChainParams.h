//
//  BRChainParams.h
//
//  Created by Aaron Voisine on 1/10/18.
//  Copyright (c) 2019 breadwallet LLC
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

#ifndef BRChainParams_h
#define BRChainParams_h

#include "BRMerkleBlock.h"
#include <assert.h>

typedef struct {
    uint32_t height;
    UInt256 hash;
    uint32_t timestamp;
    uint32_t target;
} BRCheckPoint;

typedef struct {
    const char * const *dnsSeeds; // NULL terminated array of dns seeds
    uint16_t standardPort;
    uint32_t magicNumber;
    uint64_t services;
    int (*verifyDifficulty)(const BRMerkleBlock *block, const BRMerkleBlock *previous, uint32_t transitionTime);
    const BRCheckPoint *checkpoints;
    size_t checkpointsCount;
} BRChainParams;

static const char *BRMainNetDNSSeeds[] = {
        "flo071.com", NULL
};

static const char *BRTestNetDNSSeeds[] = {
};

// blockchain checkpoints - these are also used as starting points for partial chain downloads, so they must be at
// difficulty transition boundaries in order to verify the block difficulty at the immediately following transition
static const BRCheckPoint BRMainNetCheckpoints[] = {
        {       0, uint256("000006a09dc95e0eb30c677e1c8a01080e2c49d1dd22ad1479492c61ffde9177"), 1609599446, 0x1e0ffff0 }
};

static const BRCheckPoint BRTestNetCheckpoints[] = {
        //   {     0, "852c475c605e1f20bbe60219c811abaeef08bf0d4ff87eef59200fd7a7567fa7", 1413145109, 0x1b336ce6 },
        // Sitt 2016-02-18 Use Checkpoint from the First day of digiwallet fork (from breadWallet)
        {  145000, uint256("f8d650dda836d5e3809b928b8523f050891c3bb9fa2c201bb04824a8a2fe7df6"), 1409596362, 0x1c01f271},
        { 1800000, uint256("72f46e1fff56518dce7e540b407260ea827cb1c4652f24eb1d1917f54b95d65a"), 1454769372, 0x1c021355},
        { 2149922, uint256("557846763a5f1eb3205d175724bd26ba7123c17c49eaaadf20b67c7e20e3118a"), 1460001303, 0x1c012a26},
        { 4444444, uint256("0000000000000114de2ba1462056d2a9bd9ccfbd406cd2dfedaaef2c12910659"), 1494132592, 0x1a01152f}
};

static int BRTestNetVerifyDifficulty(const BRMerkleBlock *block, const BRMerkleBlock *previous, uint32_t transitionTime)
{
    int r = 1;
    
    assert(block != NULL);
    
    if (! previous || !UInt256Eq(block->prevBlock, previous->blockHash) || block->height != previous->height + 1)
        r = 0;
    
    return r;
}

static const BRChainParams BRMainNetParams = {
    BRMainNetDNSSeeds,
    6022,       // standardPort
    0x72696f4e, // magicNumber
    0,          // services
    BRMerkleBlockVerifyDifficulty,
    BRMainNetCheckpoints,
    sizeof(BRMainNetCheckpoints)/sizeof(*BRMainNetCheckpoints)
};

static const BRChainParams BRTestNetParams = {
    BRTestNetDNSSeeds,
    12025,      // standardPort
    0xdab6c3fa, // magicNumber
    0,          // services
    BRTestNetVerifyDifficulty,
    BRTestNetCheckpoints,
    sizeof(BRTestNetCheckpoints)/sizeof(*BRTestNetCheckpoints)
};

#endif // BRChainParams_h

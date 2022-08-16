// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2015-2022 Bean Core www.beancash.org
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "txdb.h"
#include "init.h"
#include "miner.h"
#include "bitbeanrpc.h"

using namespace json_spirit;
using namespace std;

Value getsubsidy(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getsubsidy [nTarget]\n"
            "Returns proof-of-work subsidy value for the specified value of target.");

    return (uint64_t)GetProofOfWorkReward(0);
}

Value getmininginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
             "\nReturns a json object containing (PoW) mining-related information."
             "\nProof-of-Work (PoW) ended at block 10,000"
             "\nResult:\n"
             "{\n"
             "  \"blocks\": nnn,             (numeric) The current block\n"
             "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
             "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"
             "  \"Proof of Work\": xxx.xxxxx (numeric) The current difficulty for Proof of Work\n"
             "  \"Proof of Bean\": xxx.xxxxx (numeric) The current difficulty for Proof of Bean\n"
             "  \"Search Interval\": nnn,    (numeric) The last current search time period for kernel hash\n"
             "  \"difficulty\": xxx.xxxxx    (numeric) The current difficulty\n"
             "  \"Block Value\": n,          (numeric) Proof of Work Reward\n"
             "  \"Net MH/s\": : n,           (numeric) Network Proof of Work Mega Hashes per second\n"
             "  \"Net Bean Weight\": n,      (numeric) Current Network Bean Weight\n"
             "  \"errors\": \"...\"          (string) Current errors\n"
             "  \"Memory Pool Tx Size\": n,  (numeric) Current Memory Pool Size\n"
             "  \"Bean Weight\": {\n"
             "    \" Minimum\": n,           (numeric) Minimum Bean Weight\n"
             "    \" Maximum\": n,           (numeric) Maximum Bean Weight\n"
             "    \" Combined\": n,          (numeric) Combined Bean Weight\n"
             "  \" }\n"
             "  \"Bean Year Reward\": n,     (numeric) Estimate of Bean rewards earned in a year\n"
             "  \"testnet\": true|false      (boolean) If using testnet or not\n"
             "}\n"
             "\nExamples:\n"
             "  \nBeancashd getmininginfo\n"
         );

    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;
    pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);

    Object obj, diff, weight;
    obj.push_back(Pair("Blocks",        (int)nBestHeight));
    obj.push_back(Pair("Current Block Size",(uint64_t)nLastBlockSize));
    obj.push_back(Pair("Current Block Tx",(uint64_t)nLastBlockTx));

    diff.push_back(Pair("Proof of Work",        GetDifficulty()));
    diff.push_back(Pair("Proof of Bean",       GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    diff.push_back(Pair("Search Interval",      (int)nLastBeanStakeSearchInterval));
    obj.push_back(Pair("Difficulty",    diff));

    obj.push_back(Pair("Block Value",    (uint64_t)GetProofOfWorkReward(0)));
    obj.push_back(Pair("Net MH/s",     GetPoWMHashPS()));
    obj.push_back(Pair("Net Bean Weight", GetPoSKernelPS()));
    obj.push_back(Pair("Errors",        GetWarnings("statusbar")));
    obj.push_back(Pair("Pooled Tx",      (uint64_t)mempool.size()));

    weight.push_back(Pair("Minimum",    (uint64_t)nMinWeight));
    weight.push_back(Pair("Maximum",    (uint64_t)nMaxWeight));
    weight.push_back(Pair("Combined",  (uint64_t)nWeight));
    obj.push_back(Pair("Bean Weight", weight));

    obj.push_back(Pair("Bean Year Reward",    (uint64_t)bean_YEAR_REWARD));
    obj.push_back(Pair("Testnet",       fTestNet));
    return obj;
}

Value getsproutinginfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "\nReturns a json object containing Sprouting related information."
                "\nResult:\n"
                "{\n"
                "  \"Enabled\":                 (boolean) The current configuration of Sprouting\n"
                "  \"Sprouting\":               (boolean) The current status of Sprouting\n"
                "  \"errors\": \"...\"          (string) Current errors\n"
                "  \"currentblocksize\": nnn,   (numeric) The last block size\n"
                "  \"currentblocktx\": nnn,     (numeric) The last block transaction\n"               
                "  \"Memory Pool Tx Size\": n,  (numeric) Current Memory Pool Size\n"
                "  \"difficulty\": xxx.xxxxx    (numeric) The current difficulty\n"
                "  \"Search Interval\": nnn,    (numeric) The last current search time period for kernel hash\n"
                "  \"BeanWeight\": xxx.xxxxx    (numeric) Bean Weight of the node\n"
                "  \"Net Bean Weight\": xxx.xxx (numeric) Network Bean Weight\n"
                "  \"Expected Time\": ttt       (numeric) Estimated Time in seconds to find a Sprout\n"
                "\nExamples:\n"
                "  \nBeancashd getsproutininfo\n"
         );

    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;
    pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);

    uint64_t nNetworkWeight = GetPoSKernelPS();
    bool staking = nLastBeanStakeSearchInterval && nWeight;
    int nExpectedTime = staking ? (nTargetSpacing * nNetworkWeight / nWeight) : -1;

    Object obj;

    obj.push_back(Pair("Enabled", GetBoolArg("-sprouting", true)));
    obj.push_back(Pair("Sprouting", staking));
    obj.push_back(Pair("Errors", GetWarnings("statusbar")));

    obj.push_back(Pair("Current Block Size", (uint64_t)nLastBlockSize));
    obj.push_back(Pair("Current Block Tx", (uint64_t)nLastBlockTx));
    obj.push_back(Pair("Pooled Tx", (uint64_t)mempool.size()));

    obj.push_back(Pair("Difficulty", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("Search Interval", (int)nLastBeanStakeSearchInterval));

    obj.push_back(Pair("BeanWeight", (uint64_t)nWeight));
    obj.push_back(Pair("Net Bean Weight", (uint64_t)nNetworkWeight));

    obj.push_back(Pair("Expected Time", nExpectedTime));

    return obj;
}

Value getworkex(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getworkex [data, beanbase]\n"
            "\nIf [data, beanbase] is not specified, returns extended work data (useful for mining with external apps or merge mining).\n"
            "If 'data' or 'beanbase' is specified, tries to solve the block and returns true if it was successful.\n"
            "\nArguments:\n"
            "1. \"data\"       (string, optional) The hex encoded data to solve\n"
            "2. \"beanbase\"   (string, optional) A beanbase with its own nonce inserted\n"
            "\nResult (when 'data' is not specified):\n"
            "{\n"
            "  \"data\" : \"xxxxx\",      (string) The block data\n"
            "  \"target\" : \"xxxx\"      (string) The little endian hash target\n"
            "  \"beanbase\":              (string) beanbase\n"
            "  \"merkle\":                (string) merkle branch\n"
            "}\n"
            "\ngetworkex can be used to add or modify the beanbase transaction and give you the merkle branch needed to submit work to another blockchain that is merged mined./n"
            "\nResult (when 'data' is specified):\n"
            "true|false       (boolean) If solving the block specified in the 'data' was successfull\n"
            "\nExamples:\n"
            "  \nBeancashd getworkex\n"
       );

    if (vNodes.empty())
        throw JSONRPCError(-9, "Bean Core is not connected to the Bean Cash network!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(-10, "Bean Core is downloading blocks from the Bean Cash network...");

    if (pindexBest->nHeight >= LAST_POW_BLOCK)
        throw JSONRPCError(RPC_MISC_ERROR, "No more Proof of Work blocks, PoW ended at block 10,000. Bean Cash is now 100% Proof of Bean!");

    typedef map<uint256, pair<CBlock*, CScript> > mapNewBlock_t;
    static mapNewBlock_t mapNewBlock;
    static vector<CBlock*> vNewBlock;
    static CReserveKey reservekey(pwalletMain);

    if (params.size() == 0)
    {
        // Update block
        static unsigned int nTransactionsUpdatedLast;
        static CBlockIndex* pindexPrev;
        static int64_t nStart;
        static CBlock* pblock;
        if (pindexPrev != pindexBest ||
            (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60))
        {
            if (pindexPrev != pindexBest)
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                for (CBlock* pblock : vNewBlock)
                    delete pblock;
                vNewBlock.clear();
            }
            nTransactionsUpdatedLast = nTransactionsUpdated;
            pindexPrev = pindexBest;
            nStart = GetTime();

            // Create new block
            pblock = CreateNewBlock(pwalletMain);
            if (!pblock)
                throw JSONRPCError(-7, "Out of memory");
            vNewBlock.push_back(pblock);
        }

        // Update nTime
        pblock->nTime = max(pindexPrev->GetPastTimeLimit()+1, GetAdjustedTime());
        pblock->nNonce = 0;

        // Update nExtraNonce
        static unsigned int nExtraNonce = 0;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

        // Save
        mapNewBlock[pblock->hashMerkleRoot] = make_pair(pblock, pblock->vtx[0].vin[0].scriptSig);

        // Prebuild hash buffers
        char pmidstate[32];
        char pdata[128];
        char phash1[64];
        FormatHashBuffers(pblock, pmidstate, pdata, phash1);

        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

        CTransaction beanbaseTx = pblock->vtx[0];
        std::vector<uint256> merkle = pblock->GetMerkleBranch(0);

        Object result;
        result.push_back(Pair("data",     HexStr(BEGIN(pdata), END(pdata))));
        result.push_back(Pair("target",   HexStr(BEGIN(hashTarget), END(hashTarget))));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << beanbaseTx;
        result.push_back(Pair("beanbase", HexStr(ssTx.begin(), ssTx.end())));

        Array merkle_arr;

        for (uint256 merkleh : merkle) {
            merkle_arr.push_back(HexStr(BEGIN(merkleh), END(merkleh)));
        }

        result.push_back(Pair("merkle", merkle_arr));


        return result;
    }
    else
    {
        // Parse parameters
        vector<unsigned char> vchData = ParseHex(params[0].get_str());
        vector<unsigned char> beanbase;

        if(params.size() == 2)
            beanbase = ParseHex(params[1].get_str());

        if (vchData.size() != 128)
            throw JSONRPCError(-8, "Invalid parameter");

        CBlock* pdata = (CBlock*)&vchData[0];

        // Byte reverse
        for (int i = 0; i < 128/4; i++)
            ((unsigned int*)pdata)[i] = ByteReverse(((unsigned int*)pdata)[i]);

        // Get saved block
        if (!mapNewBlock.count(pdata->hashMerkleRoot))
            return false;
        CBlock* pblock = mapNewBlock[pdata->hashMerkleRoot].first;

        pblock->nTime = pdata->nTime;
        pblock->nNonce = pdata->nNonce;

        if(beanbase.size() == 0)
            pblock->vtx[0].vin[0].scriptSig = mapNewBlock[pdata->hashMerkleRoot].second;
        else
            CDataStream(beanbase, SER_NETWORK, PROTOCOL_VERSION) >> pblock->vtx[0]; // FIXME - HACK!

        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        return CheckWork(pblock, *pwalletMain, reservekey);
    }
}


Value getwork(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
             "getwork ( \"data\" )\n"
             "\nIf 'data' is not specified, it returns the formatted hash data to work on.\n"
             "If 'data' is specified, tries to solve the block and returns true if it was successful.\n"
             "\nArguments:\n"
             "1. \"data\"       (string, optional) The hex encoded data to solve\n"
             "\nResult (when 'data' is not specified):\n"
             "{\n"
             "  \"midstate\" : \"xxxx\",   (string) The precomputed hash state after hashing the first half of the data (DEPRECATED)\n" // deprecated
             "  \"data\" : \"xxxxx\",      (string) The block data\n"
             "  \"hash1\" : \"xxxxx\",     (string) The formatted hash buffer for second hash (DEPRECATED)\n" // deprecated
             "  \"target\" : \"xxxx\"      (string) The little endian hash target\n"
             "}\n"
             "\nResult (when 'data' is specified):\n"
             "true|false       (boolean) If solving the block specified in the 'data' was successfull\n"
             "\nExamples:\n"
             "  \nBeancashd getwork\n"
        );

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Bean Cash is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Bean Cash is downloading blocks...");

    if (pindexBest->nHeight >= LAST_POW_BLOCK)
        throw JSONRPCError(RPC_MISC_ERROR, "No more Proof of Work blocks, PoW ended at block 10,000. Bean Cash is now 100% Proof of Bean!");

    typedef map<uint256, pair<CBlock*, CScript> > mapNewBlock_t;
    static mapNewBlock_t mapNewBlock;    // FIXME: thread safety
    static vector<CBlock*> vNewBlock;
    static CReserveKey reservekey(pwalletMain);

    if (params.size() == 0)
    {
        // Update block
        static unsigned int nTransactionsUpdatedLast;
        static CBlockIndex* pindexPrev;
        static int64_t nStart;
        static CBlock* pblock;
        if (pindexPrev != pindexBest ||
            (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60))
        {
            if (pindexPrev != pindexBest)
            {
                // Deallocate old blocks since they're obsolete now
                mapNewBlock.clear();
                for (CBlock* pblock : vNewBlock)
                    delete pblock;
                vNewBlock.clear();
            }

            // Clear pindexPrev so future getworks make a new block, despite any failures from here on
            pindexPrev = NULL;

            // Store the pindexBest used before CreateNewBlock, to avoid races
            nTransactionsUpdatedLast = nTransactionsUpdated;
            CBlockIndex* pindexPrevNew = pindexBest;
            nStart = GetTime();

            // Create new block
            pblock = CreateNewBlock(pwalletMain);
            if (!pblock)
                throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");
            vNewBlock.push_back(pblock);

            // Need to update only after we know CreateNewBlock succeeded
            pindexPrev = pindexPrevNew;
        }

        // Update nTime
        pblock->UpdateTime(pindexPrev);
        pblock->nNonce = 0;

        // Update nExtraNonce
        static unsigned int nExtraNonce = 0;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

        // Save
        mapNewBlock[pblock->hashMerkleRoot] = make_pair(pblock, pblock->vtx[0].vin[0].scriptSig);

        // Pre-build hash buffers
        char pmidstate[32];
        char pdata[128];
        char phash1[64];
        FormatHashBuffers(pblock, pmidstate, pdata, phash1);

        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

        Object result;
        result.push_back(Pair("midstate", HexStr(BEGIN(pmidstate), END(pmidstate)))); // deprecated
        result.push_back(Pair("data",     HexStr(BEGIN(pdata), END(pdata))));
        result.push_back(Pair("hash1",    HexStr(BEGIN(phash1), END(phash1)))); // deprecated
        result.push_back(Pair("target",   HexStr(BEGIN(hashTarget), END(hashTarget))));
        return result;
    }
    else
    {
        // Parse parameters
        vector<unsigned char> vchData = ParseHex(params[0].get_str());
        if (vchData.size() != 128)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
        CBlock* pdata = (CBlock*)&vchData[0];

        // Byte reverse
        for (int i = 0; i < 128/4; i++)
            ((unsigned int*)pdata)[i] = ByteReverse(((unsigned int*)pdata)[i]);

        // Get saved block
        if (!mapNewBlock.count(pdata->hashMerkleRoot))
            return false;
        CBlock* pblock = mapNewBlock[pdata->hashMerkleRoot].first;

        pblock->nTime = pdata->nTime;
        pblock->nNonce = pdata->nNonce;
        pblock->vtx[0].vin[0].scriptSig = mapNewBlock[pdata->hashMerkleRoot].second;
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        assert(pwalletMain != NULL);
        return CheckWork(pblock, *pwalletMain, reservekey);
    }
}


Value getblocktemplate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
             "getblocktemplate ( \"jsonrequestobject\" )\n"
             "\nIf the request parameters include a 'mode' key, that is used to explicitly select between the default 'template' request or a 'proposal'.\n"
             "It returns data needed to construct a block to work on.\n"
             "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

             "\nArguments:\n"
             "1. \"jsonrequestobject\"       (string, optional) A json object in the following spec\n"
             "     {\n"
             "       \"mode\":\"template\"   (string, optional) This must be set to \"template\" or omitted\n"
             "       \"capabilities\":[      (array, optional) A list of strings\n"
             "           \"support\"         (string) client side supported feature, 'longpoll', 'beanbasetxn', 'beanbasevalue', 'proposal', 'serverlist', 'workid'\n"
             "           ,...\n"
             "         ]\n"
             "     }\n"
             "\n"

             "\nResult:\n"
             "{\n"
             "  \"version\" : n,                    (numeric) The block version\n"
             "  \"previousblockhash\" : \"xxxx\",   (string) The hash of current highest block\n"
             "  \"transactions\" : [                (array) contents of non-coinbase transactions that should be included in the next block\n"
             "      {\n"
             "         \"data\" : \"xxxx\",         (string) transaction data encoded in hexadecimal (byte-for-byte)\n"
             "         \"hash\" : \"xxxx\",         (string) hash/id encoded in little-endian hexadecimal\n"
             "         \"depends\" : [              (array) array of numbers \n"
             "             n                        (numeric) transactions before this one (by 1-based index in 'transactions' list) that must be present in the final block if this one is\n"
             "             ,...\n"
             "         ],\n"
             "         \"fee\": n,                   (numeric) difference in value between transaction inputs and outputs (in Adzukis); for beanbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one\n"
             "         \"sigops\" : n,               (numeric) total number of SigOps, as counted for purposes of block limits; if key is not present, sigop count is unknown and clients MUST NOT assume there aren't any\n"
             "         \"required\" : true|false     (boolean) if provided and true, this transaction must be in the final block\n"
             "      }\n"
             "      ,...\n"
             "  ],\n"
             "  \"beannbaseaux\" : {                 (json object) data that should be included in the beanbase's scriptSig content\n"
             "      \"flags\" : \"flags\"            (string) \n"
             "  },\n"
             "  \"beanbasevalue\" : n,               (numeric) maximum allowable input to beanbase transaction, including the generation award and transaction fees (in Satoshis)\n"
             "  \"beanbasetxn\" : { ... },           (json object) information for beanbase transaction\n"
             "  \"target\" : \"xxxx\",               (string) The hash target\n"
             "  \"mintime\" : xxx,                   (numeric) The minimum timestamp appropriate for next block time in seconds since epoch (Jan 1 1970 GMT)\n"
             "  \"mutable\" : [                      (array of string) list of ways the block template may be changed \n"
             "     \"value\"                         (string) A way the block template may be changed, e.g. 'time', 'transactions', 'prevblock'\n"
             "     ,...\n"
             "  ],\n"
             "  \"noncerange\" : \"00000000ffffffff\",   (string) A range of valid nonces\n"
             "  \"sigoplimit\" : n,                 (numeric) limit of sigops in blocks\n"
             "  \"sizelimit\" : n,                  (numeric) limit of block size\n"
             "  \"curtime\" : ttt,                  (numeric) current timestamp in seconds since epoch (Feb 13 2015 GMT)\n"
             "  \"bits\" : \"xxx\",                 (string) compressed target of next block\n"
             "  \"height\" : n                      (numeric) The height of the next block\n"
             "}\n"

             "\nExamples:\n"
             "  \nBeancashd getblocktemplate\n"
        );



    std::string strMode = "template";
    if (params.size() > 0)
    {
        const Object& oparam = params[0].get_obj();
        const Value& modeval = find_value(oparam, "mode");
        if (modeval.type() == str_type)
            strMode = modeval.get_str();
        else if (modeval.type() == null_type)
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (vNodes.empty())
        throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Bean Cash is not connected!");

    if (IsInitialBlockDownload())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Bean Cash is downloading blocks...");

    if (pindexBest->nHeight >= LAST_POW_BLOCK)
        throw JSONRPCError(RPC_MISC_ERROR, "No more PoW blocks");

    static CReserveKey reservekey(pwalletMain);

    // Update block
    static unsigned int nTransactionsUpdatedLast;
    static CBlockIndex* pindexPrev;
    static int64_t nStart;
    static CBlock* pblock;
    if (pindexPrev != pindexBest ||
        (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = NULL;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = nTransactionsUpdated;
        CBlockIndex* pindexPrevNew = pindexBest;
        nStart = GetTime();

        // Create new block
        if(pblock)
        {
            delete pblock;
            pblock = NULL;
        }
        pblock = CreateNewBlock(pwalletMain);
        if (!pblock)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }

    // Update nTime
    pblock->UpdateTime(pindexPrev);
    pblock->nNonce = 0;

    Array transactions;
    map<uint256, int64_t> setTxIndex;
    int i = 0;
    CTxDB txdb("r");
    for (CTransaction& tx : pblock->vtx)
    {
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsBeanBase() || tx.IsBeanStake())
            continue;

        Object entry;

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;
        entry.push_back(Pair("data", HexStr(ssTx.begin(), ssTx.end())));

        entry.push_back(Pair("hash", txHash.GetHex()));

        MapPrevTx mapInputs;
        map<uint256, CTxIndex> mapUnused;
        bool fInvalid = false;
        if (tx.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
        {
            entry.push_back(Pair("fee", (int64_t)(tx.GetValueIn(mapInputs) - tx.GetValueOut())));

            Array deps;
            for (MapPrevTx::value_type& inp : mapInputs)
            {
                if (setTxIndex.count(inp.first))
                    deps.push_back(setTxIndex[inp.first]);
            }
            entry.push_back(Pair("depends", deps));

            int64_t nSigOps = tx.GetLegacySigOpCount();
            nSigOps += tx.GetP2SHSigOpCount(mapInputs);
            entry.push_back(Pair("sigops", nSigOps));
        }

        transactions.push_back(entry);
    }

    Object aux;
    aux.push_back(Pair("flags", HexStr(beanBASE_FLAGS.begin(), beanBASE_FLAGS.end())));

    uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

    static Array aMutable;
    if (aMutable.empty())
    {
        aMutable.push_back("time");
        aMutable.push_back("transactions");
        aMutable.push_back("prevblock");
    }

    Object result;
    result.push_back(Pair("version", pblock->nVersion));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("transactions", transactions));
    result.push_back(Pair("beanbaseaux", aux));
    result.push_back(Pair("beanbasevalue", (int64_t)pblock->vtx[0].vout[0].nValue));
    result.push_back(Pair("target", hashTarget.GetHex()));
    result.push_back(Pair("mintime", (int64_t)pindexPrev->GetPastTimeLimit()+1));
    result.push_back(Pair("mutable", aMutable));
    result.push_back(Pair("noncerange", "00000000ffffffff"));
    result.push_back(Pair("sigoplimit", (int64_t)MAX_BLOCK_SIGOPS));
    result.push_back(Pair("sizelimit", (int64_t)MAX_BLOCK_SIZE));
    result.push_back(Pair("curtime", (int64_t)pblock->nTime));
    result.push_back(Pair("bits", HexBits(pblock->nBits)));
    result.push_back(Pair("height", (int64_t)(pindexPrev->nHeight+1)));

    return result;
}

Value submitblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
             "submitblock \"hexdata\" ( \"jsonparametersobject\" )\n"
             "\nAttempts to submit new block to network.\n"
             "The 'jsonparametersobject' parameter is currently ignored.\n"
             "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n"

             "\nArguments\n"
             "1. \"hexdata\"    (string, required) the hex-encoded block data to submit\n"
             "2. \"jsonparametersobject\"     (string, optional) object of optional parameters\n"
             "    {\n"
             "      \"workid\" : \"id\"    (string, optional) if the server provided a workid, it MUST be included with submissions\n"
             "    }\n"
             "\nResult:\n"
             "\nExamples:\n"
             "  \nBeancashd submitblock <hexdata> <json_object>\n"
        );

    vector<unsigned char> blockData(ParseHex(params[0].get_str()));
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    CBlock block;
    try {
        ssBlock >> block;
    }
    catch (const std::exception& e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    bool fAccepted = ProcessBlock(NULL, &block);
    if (!fAccepted)
        return "rejected";

    return Value::null;
}


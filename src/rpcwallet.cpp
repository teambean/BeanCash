// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2015-2022 Bean Core www.beancash.org
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "wallet.h"
#include "walletdb.h"
#include "bitbeanrpc.h"
#include "init.h"
#include "base58.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

int64_t nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

static void accountingDeprecationCheck()
{
    if (!GetBoolArg("-enableaccounts", false))
        throw runtime_error(
            "Accounting API is deprecated and will be removed in future.\n"
            "It can easily result in negative or odd balances if misused or misunderstood, which has happened in the field.\n"
            "If you still want to enable it, add to your config file enableaccounts=1\n");

    if (GetBoolArg("-sprouting", true))
        throw runtime_error("If you want to use accounting API, Sprouting must be disabled, add to your Beancash.conf file sprouting=0\n");
}

std::string HelpRequiringPassphrase()
{
    return pwalletMain && pwalletMain->IsCrypted()
        ? "\nRequires wallet passphrase to be set with walletpassphrase call first"
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (fWalletUnlockStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet is unlocked for sprouting only.");
}

void WalletTxToJSON(const CWalletTx& wtx, Object& entry)
{
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsBeanBase() || wtx.IsBeanStake())
        entry.push_back(Pair("generated", true));
    if (confirms > 0)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
        entry.push_back(Pair("blocktime", (int64_t)(mapBlockIndex[wtx.hashBlock]->nTime)));
    }
    entry.push_back(Pair("txid", wtx.GetHash().GetHex()));
    entry.push_back(Pair("time", (int64_t)wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (int64_t)wtx.nTimeReceived));
    for (const std::pair<string,string>& item : wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

string AccountFromValue(const Value& value)
{
    string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}

// Getinfo
Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
             "Returns an object containing various state info.\n"
             "\nResult:\n"
             "{\n"
             "  \"version\": xxxxx,           (numeric) the server version\n"
             "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
             "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
             "  \"balance\": xxxxxxx,         (numeric) the total BITB balance of the wallet\n"
             "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
             "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
             "  \"connections\": xxxxx,       (numeric) the number of connections\n"
             "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
             "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
             "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
             "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
             "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
             "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in bitb\n"
             "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Feb 13 2015 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
             "  \"errors\": \"...\"           (string) any error messages\n"
             "}\n"
             "\nExamples:\n"
             "  \nBeancashd getinfo\n"
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj, diff;
    obj.push_back(Pair("version",       FormatFullVersion()));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
	 if (pwalletMain)
	 {    
    	obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    	obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
     }
    obj.push_back(Pair("maturing-sprouts",     ValueFromAmount(pwalletMain->GetStake())));
    obj.push_back(Pair("blocks",        (int)nBestHeight));
    obj.push_back(Pair("timeoffset",    (int64_t)GetTimeOffset()));
    obj.push_back(Pair("moneysupply",   ValueFromAmount(pindexBest->nMoneySupply)));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    // obj.push_back(Pair("ip",            GetLocalAddress(NULL).ToStringIP()));
    obj.push_back(Pair("ip",            addrSeenByPeer.ToStringIP()));

    diff.push_back(Pair("proof-of-work",  GetDifficulty()));
    diff.push_back(Pair("proof-of-bean", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("difficulty",    diff));

    obj.push_back(Pair("testnet",       fTestNet));
	 if (pwalletMain)    
	 {    
    	obj.push_back(Pair("keypoololdest", (int64_t)pwalletMain->GetOldestKeyPoolTime()));
    	obj.push_back(Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    }
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
    obj.push_back(Pair("mininput",      ValueFromAmount(nMinimumInputValue)));
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", (int64_t)nWalletUnlockTime / 1000));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}

// Get new pubkey
Value getnewpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewpubkey [account]\n"
            "Returns new public key for beanbase generation.");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return HexStr(newKey.begin(), newKey.end());
}

// Get new address
Value getnewaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
             "getnewaddress ( \"account\" )\n"
             "\nReturns a new Bean Cash address for receiving payments.\n"
             "If 'account' is specified (recommended), it is added to the address book \n"
             "so payments received with the address will be credited to 'account'.\n"
             "\nArguments:\n"
             "1. \"account\"        (string, optional) The account name for the address to be linked to. if not provided, the default account \"\" is used. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created if there is no account by the given name.\n"
             "\nResult:\n"
             "\"beancashaddress\"    (string) The new Bean Cash address\n"
             "\nExamples:\n"
             "  \nBeancashd getnewaddress <myaccountname>\n"
             "  \nBeancashd getnewaddress\n"
        );

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return CBitbeanAddress(keyID).ToString();
}


CBitbeanAddress GetAccountAddress(string strAccount, bool bForceNew=false)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid())
    {
        CScript scriptPubKey;
        scriptPubKey.SetDestination(account.vchPubKey.GetID());
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it)
        {
            const CWalletTx& wtx = (*it).second;
            for (const CTxOut& txout : wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed)
    {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBookName(account.vchPubKey.GetID(), strAccount);
        walletdb.WriteAccount(strAccount, account);
    }

    return CBitbeanAddress(account.vchPubKey.GetID());
}

// Get account address
Value getaccountaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "getaccountaddress \"account\"\n"
             "\nReturns the current Bean Cash address for receiving payments to this account.\n"
             "\nArguments:\n"
             "1. \"account\"       (string, required) The account name for the address. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created and a new address created  if there is no account by the given name.\n"
             "\nResult:\n"
             "\"beancashaddress\"   (string) The account Bean Cash address\n"
             "\nExamples:\n"
             "  \nBeancashd getaccountaddress\n"
             "  \nBeancashd getaccountaddress <myaccountname>\n"
        );

    // Parse the account first so we don't generate a key if there's an error
    string strAccount = AccountFromValue(params[0]);

    Value ret;

    ret = GetAccountAddress(strAccount).ToString();

    return ret;
}


// Set Account with a given address
Value setaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
             "setaccount \"beancashaddress\" \"account\"\n"
             "\nSets the account associated with the given address.\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"  (string, required) The Bean Cash address to be associated with an account.\n"
             "2. \"account\"         (string, required) The account to assign the address to.\n"
             "\nExamples:\n"
             "  \nBeancashd setaccount 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 teambean\n"
        );



    CBitbeanAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");


    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Detect when changing the account of an address that is the 'unused current key' of another account:
    if (pwalletMain->mapAddressBook.count(address.Get()))
    {
        string strOldAccount = pwalletMain->mapAddressBook[address.Get()].name;
        if (address == GetAccountAddress(strOldAccount))
            GetAccountAddress(strOldAccount, true);
    }

    pwalletMain->SetAddressBookName(address.Get(), strAccount);

    return Value::null;
}

// Get account associated with a given address
Value getaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "getaccount \"beancashaddress\"\n"
             "\nReturns the account associated with the given address.\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"  (string, required) The Bean Cash address for account lookup.\n"
             "\nResult:\n"
             "\"accountname\"        (string) the account address\n"
             "\nExamples:\n"
             "  \nBeancashd getaccount 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\n"
        );

    CBitbeanAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");

    string strAccount;
    map<CTxDestination, CAddressBookData>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.name.empty())
        strAccount = (*mi).second.name;
    return strAccount;
}

// Get addresses for a given account
Value getaddressesbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "getaddressesbyaccount \"account\"\n"
             "\nReturns the list of addresses for the given account.\n"
             "\nArguments:\n"
             "1. \"account\"  (string, required) The account name.\n"
             "\nResult:\n"
             "[                     (json array of string)\n"
             "  \"beancashaddress\"  (string) a Bean Cash address associated with the given account\n"
             "  ,...\n"
             "]\n"
             "\nExamples:\n"
             "  \nBeancashd getaddressesbyaccount teambean\n"
        );

    string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    Array ret;
    for (const std::pair<CBitbeanAddress, CAddressBookData>& item : pwalletMain->mapAddressBook)
    {
        const CBitbeanAddress& address = item.first;
        const string& strName = item.second.name;
        if (strName == strAccount)
            ret.push_back(address.ToString());
    }
    return ret;
}

// Send to address an amount of bitb
Value sendtoaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
             "sendtoaddress \"beancashaddress\" amount ( \"comment\" \"comment-to\" )\n"
             "\nSent an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n"
             + HelpRequiringPassphrase() +
             "\nArguments:\n"
             "1. \"beancashaddress\"  (string, required) The Bean Cash address to send to.\n"
             "2. \"amount\"      (numeric, required) The amount in bitb to send. eg 0.1\n"
             "3. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
             "                             This is not part of the transaction, just kept in your wallet.\n"
             "4. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
             "                             to which you're sending the transaction. This is not part of the \n"
             "                             transaction, just kept in your wallet.\n"
             "\nResult:\n"
             "\"transactionid\"  (string) The transaction id. (view at https://chainz.cryptoid.info/bean/api.dws?t=[transactionID])\n"
             "\nExamples:\n"
             "  \nBeancashd sendtoaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 1000000 \"donation\" \"Bean Core\"\n"
            );

    CBitbeanAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["to"]      = params[3].get_str();

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

// List address groupings with common ownership
Value listaddressgroupings(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listaddressgroupings\n"
            "\nLists groups of addresses which have had their common ownership\n"
            "made public by common use as inputs or as the resulting change\n"
            "in past transactions\n"
            "\nResult:\n"
            "[\n"
            "  [\n"
            "    [\n"
            "      \"beancashaddress\",    (string) The Bean Cash address\n"
            "      amount,                 (numeric) The amount in bitb\n"
            "      \"account\"             (string, optional) The account\n"
            "    ]\n"
            "    ,...\n"
            "  ]\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            "  \nBeancashd listaddressgroupings\n"
        );

    Array jsonGroupings;
    map<CTxDestination, int64_t> balances = pwalletMain->GetAddressBalances();
    for (set<CTxDestination> grouping : pwalletMain->GetAddressGroupings())
    {
        Array jsonGrouping;
        for (CTxDestination address : grouping)
        {
            Array addressInfo;
            addressInfo.push_back(CBitbeanAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CBitbeanAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                    addressInfo.push_back(pwalletMain->mapAddressBook.find(CBitbeanAddress(address).Get())->second.name);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}

// Sign a message with a Bean Cash key
Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
             "signmessage \"beancashaddress\" \"message\"\n"
             "\nSign a message with the private key of an address"
             + HelpRequiringPassphrase() + "\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"  (string, required) The Bean Cash address to use for the private key.\n"
             "2. \"message\"         (string, required) The message to create a signature of.\n"
             "\nResult:\n"
             "\"signature\"          (string) The signature of the message encoded in base 64\n"
             "\nExamples:\n"
             "\nUnlock the wallet for 30 seconds\n"
             "  \nBeancashd walletpassphrase <yourpassphrase> 30\n"
             "\nCreate the signature\n"
             "  \nBeancashd signmessage 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 \"your message\"\n"
             "\nVerify the signature\n"
             "   \nBeancashd verifymessage 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 \"signature\" \"your message\"\n"
         );

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CBitbeanAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

// Verify a message with a Bean Cash key
Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
             "verifymessage \"beancashaddress\" \"signature\" \"message\"\n"
             "\nVerify a signed message\n"
             "\nArguments:\n"
             "1. \"beancashaddress\" (string, required) The Bean Cash address to use for the signature.\n"
             "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
             "3. \"message\"         (string, required) The message that was signed.\n"
             "\nResult:\n"
             "true|false   (boolean) If the signature is verified or not.\n"
             "\nExamples:\n"
             "\nUnlock the wallet for 30 seconds\n"
             "  \nBeancashd walletpassphrase <your passphrase> 30\n"
             "\nCreate the signature\n"
             "  \nBeancashd signmessage 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 \"your message\"\n"
             "\nVerify the signature\n"
             "  \nBeancashd verifymessage 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 \"signature\" \"your message\"\n"
        );

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CBitbeanAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(Hash(ss.begin(), ss.end()), vchSig))
    return false;

    return (pubkey.GetID() == keyID);
}

// Get amount received by an address with a given number of confirmations
Value getreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
             "getreceivedbyaddress \"beancashaddress\" ( minconf )\n"
             "\nReturns the total amount received by the given beancashaddress in transactions with at least minconf confirmations.\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"  (string, required) The Bean Cash address for transactions.\n"
             "2. minconf             (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
             "\nResult:\n"
             "amount   (numeric) The total amount in bitb received at this address.\n"
             "\nExamples:\n"
             "\nThe amount from transactions with at least 1 confirmation\n"
             "  \nBeancashd getreceivedbyaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\n"
             "\nThe amount including unconfirmed transactions, zero confirmations\n"
             "  \nBeancashd getreceivedbyaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 0\n"
             "\nThe amount with at least 6 confirmation, very safe\n"
             "  \nBeancashd getreceivedbyaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 6\n"
        );

    // Bitbean address
    CBitbeanAddress address = CBitbeanAddress(params[0].get_str());
    CScript scriptPubKey;
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");
    scriptPubKey.SetDestination(address.Get());
    if (!IsMine(*pwalletMain,scriptPubKey))
        return (double)0.0;

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsBeanBase() || wtx.IsBeanStake() || !wtx.IsFinal())
            continue;

        for (const CTxOut& txout : wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return  ValueFromAmount(nAmount);
}

// Get account address
void GetAccountAddresses(string strAccount, set<CTxDestination>& setAddress)
{
    for (const std::pair<CTxDestination, CAddressBookData>& item : pwalletMain->mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second.name;
        if (strName == strAccount)
            setAddress.insert(address);
    }
}

// Get total amount received by addresses with a given account, with a given amount of transactions
Value getreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
             "getreceivedbyaccount \"account\" ( minconf )\n"
             "\nReturns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.\n"
             "\nArguments:\n"
             "1. \"account\"      (string, required) The selected account, may be the default account using \"\".\n"
             "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
             "\nResult:\n"
             "amount              (numeric) The total amount in bitb received for this account.\n"
             "\nExamples:\n"
             "\nAmount received by the default account with at least 1 confirmation\n"
             "  \nBeancashd getreceivedbyaccount \"\".\n"
             "\nAmount received at the teambean account including unconfirmed amounts with zero confirmations\n"
             "  \nBeancashd getreceivedbyaccount teambean 0\n"
             "\nThe amount with at least 6 confirmation, very safe\n"
             "  \nBeancashd getreceivedbyaccount teambean 6\n"
        );

    accountingDeprecationCheck();

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    string strAccount = AccountFromValue(params[0]);
    set<CTxDestination> setAddress;
    GetAccountAddresses(strAccount, setAddress);

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsBeanBase() || wtx.IsBeanStake() || !wtx.IsFinal())
            continue;

        for (const CTxOut& txout : wtx.vout)
        {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)bean;
}


int64_t GetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64_t nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!wtx.IsFinal() || wtx.GetDepthInMainChain() < 0)
            continue;

        int64_t nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

int64_t GetAccountBalance(const string& strAccount, int nMinDepth)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth);
}

// Get BITB balance
Value getbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
             "getbalance ( \"account\" minconf )\n"
             "\nIf account is not specified, returns the server's total available balance.\n"
             "If account is specified, returns the balance in the account.\n"
             "Note that the account \"\" is not the same as leaving the parameter out.\n"
             "The server total may be different to the balance in the default \"\" account.\n"
             "\nArguments:\n"
             "1. \"account\"      (string, optional) The selected account. It may be the default account using \"\".\n"
             "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
             "\nResult:\n"
             "amount              (numeric) The total amount in bitb received for this account.\n"
             "\nExamples:\n"
             "\nThe total amount in the server across all accounts\n"
             "  \nBeancashd getbalance \n"
             "\nThe total amount in the default account with at least 1 confirmation\n"
             "  \nBeancashd getbalance \"\".\n"
             "\nThe total amount in the account named teambean with at least 6 confirmations\n"
             "  \nBeancashd getbalance teambean 6\n"
        );

    if (params.size() == 0)
        return  ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    if (params[0].get_str() == "*") {
        // Calculate total balance a different way from GetBalance()
        // (GetBalance() sums up all unspent TxOuts)
        // getbalance and getbalance '*' 0 should return the same number.
        int64_t nBalance = 0;
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (!wtx.IsTrusted())
                continue;

            int64_t allFee;
            string strSentAccount;
            list<pair<CTxDestination, int64_t> > listReceived;
            list<pair<CTxDestination, int64_t> > listSent;
            wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount);
            if (wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
            {
                for (const std::pair<CTxDestination,int64_t>& r : listReceived)
                    nBalance += r.second;
            }
            for (const std::pair<CTxDestination,int64_t>& r : listSent)
                nBalance -= r.second;
            nBalance -= allFee;
        }
        return  ValueFromAmount(nBalance);
    }

    accountingDeprecationCheck();

    string strAccount = AccountFromValue(params[0]);

    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);

    return ValueFromAmount(nBalance);
}

// Move funds from one account to another
Value movecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
             "move \"fromaccount\" \"toaccount\" amount ( minconf \"comment\" )\n"
             "\nMove a specified amount from one account in your wallet to another.\n"
             "\nArguments:\n"
             "1. \"fromaccount\"   (string, required) The name of the account to move funds from. May be the default account using \"\".\n"
             "2. \"toaccount\"     (string, required) The name of the account to move funds to. May be the default account using \"\".\n"
             "3. minconf           (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
             "4. \"comment\"       (string, optional) An optional comment, stored in the wallet only.\n"
             "\nResult:\n"
             "true|false           (boolean) true if successfull.\n"
             "\nExamples:\n"
             "\nMove 1000000 bitb from the default account to the account named teambean\n"
             "  \nBeancashd move  \"\" teambean 1000000\n"
             "\nMove 0.01 bitb from nokat to adzuki with a comment and funds have 6 confirmations\n"
             "  \nBeancashd move nokat adzuki 0.01 6 \"Bean Cash to the Moon then Mars!\"\n"
         );

    accountingDeprecationCheck();

    string strFrom = AccountFromValue(params[0]);
    string strTo = AccountFromValue(params[1]);
    int64_t nAmount = AmountFromValue(params[2]);

    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    walletdb.WriteAccountingEntry(debit);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    walletdb.WriteAccountingEntry(credit);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}

// Send BITB from an account to a Bean Cash address
Value sendfrom(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
             "sendfrom \"fromaccount\" \"tobeancashaddress\" amount ( minconf \"comment\" \"comment-to\" )\n"
             "\nSend an amount from an account to a Bean Cash address.\n"
             "The amount is a real and is rounded to the nearest 0.00000001."
             + HelpRequiringPassphrase() + "\n"
             "\nArguments:\n"
             "1. \"fromaccount\"       (string, required) The name of the account to send funds from. May be the default account using \"\".\n"
             "2. \"tobeancashaddress\" (string, required) The Bean Cash address to send funds to.\n"
             "3. amount                (numeric, required) The amount in bitb (transaction fee is added on top).\n"
             "4. minconf               (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
             "5. \"comment\"           (string, optional) A comment used to store what the transaction is for. \n"
             "                                     This is not part of the transaction, just kept in your wallet.\n"
             "6. \"comment-to\"        (string, optional) An optional comment to store the name of the person or organization \n"
             "                                     to which you're sending the transaction. This is not part of the transaction, \n"
             "                                     it is just kept in your wallet.\n"
             "\nResult:\n"
             "\"transactionid\"        (string) The transaction id. (view at https://chainz.cryptoid.info/bean/api.dws?t=[transactionID])\n"
             "\nExamples:\n"
             "\nSend 1000000 bitb from the default account to the address, must have at least 1 confirmation\n"
             "  \nBeancashd sendfrom \"\" 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 1000000\n"
             "\nSend 0.01 from the teambean account to the given address, funds must have at least 6 confirmations\n"
             "  \nBeancashd sendfrom teambean 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 0.01 6 \"donation\" \"Bean Core team development address\"\n"
         );

    string strAccount = AccountFromValue(params[0]);
    CBitbeanAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");
    int64_t nAmount = AmountFromValue(params[2]);

    int nMinDepth = 1;
    if (params.size() > 3)
        nMinDepth = params[3].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();

    EnsureWalletIsUnlocked();

    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

// Send BITB from account to many addresses
Value sendmany(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
             "sendmany \"fromaccount\" {\"address\":amount,...} ( minconf \"comment\" )\n"
             "\nSend multiple times. Amounts are double-precision floating point numbers."
             + HelpRequiringPassphrase() + "\n"
             "\nArguments:\n"
             "1. \"fromaccount\"         (string, required) The account to send the funds from, can be \"\" for the default account\n"
             "2. \"amounts\"             (string, required) A json object with addresses and amounts\n"
             "    {\n"
             "      \"address\":amount   (numeric) The Bean Cash address is the key, the numeric amount in bitb is the value\n"
             "      ,...\n"
             "    }\n"
             "3. minconf                 (numeric, optional, default=1) Only use the balance confirmed at least this many times.\n"
             "4. \"comment\"             (string, optional) A comment\n"
             "\nResult:\n"
             "\"transactionid\"          (string) The transaction id for the send. Only 1 transaction is created regardless of \n"
             "                                    the number of addresses. See https://chainz.cryptoid.info/bean/api.dws?t=[transactionID]\n"
             "\nExamples:\n"
             "\nSend two amounts to two different addresses:\n"
             "  \nBeancashd sendmany teambean {\\\"2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\\\":0.01,\\\"2ZkipiuqPaKwgMVxth5skFep6pRdeTf3BK\\\":0.02}\n"
             "\nSend two amounts to two different addresses setting the confirmation and comment:\n"
             "  \nBeancashd sendmany teambean {\\\"2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\\\":0.01,\\\"2ZkipiuqPaKwgMVxth5skFep6pRdeTf3BK\\\":0.02}\" 6 \"testing\"\m"
         );

    string strAccount = AccountFromValue(params[0]);
    Object sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

    set<CBitbeanAddress> setAddress;
    vector<pair<CScript, int64_t> > vecSend;

    int64_t totalAmount = 0;
    for (const Pair& s : sendTo)
    {
        CBitbeanAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Bean Cash address: ")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64_t nAmount = AmountFromValue(s.value_);

        totalAmount += nAmount;

        vecSend.push_back(make_pair(scriptPubKey, nAmount));
    }

    EnsureWalletIsUnlocked();

    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);
    if (!fCreated)
    {
        if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

// Used by both addmultisigaddress & createmultisig
static CScript _createmultisig(const Array& params)
{
    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();

        // Case 1: Bitbean address and we have full public key:
        CBitbeanAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks.c_str()));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks.c_str()));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result;
    result.SetMultisig(nRequired, pubkeys);
    return result;
}

// Add a multi-signature address
Value addmultisigaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
    {
        string msg = "addmultisigaddress nrequired [\"key\",...] ( \"account\" )\n"
            "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
            "Each key is a Bean Cash address or hex-encoded public key.\n"
            "If 'account' is specified, assign address to that account.\n"

            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of Bean Cash addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"address\"  (string) Bean Cash address or hex-encoded public key\n"
            "       ...,\n"
            "     ]\n"
            "3. \"account\"      (string, optional) An account to assign the addresses to.\n"

            "\nResult:\n"
            "\"nbeancashaddress\"  (string) A Bean Cash address associated with the keys.\n"

            "\nExamples:\n"
            "\nAdd a multisig address from 2 addresses\n"
            "  \nBeancashd addmultisigaddress 2 [\\\"2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\\\",\\\"2ZkipiuqPaKwgMVxth5skFep6pRdeTf3BK\\\"]\n"
        ;
        throw runtime_error(msg);
    }

    string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig(params);
    CScriptID innerID = inner.GetID();
    if (!pwalletMain->AddCScript(inner))
        throw runtime_error("AddCScript() failed");

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return CBitbeanAddress(innerID).ToString();
}

// Create a multi-signature address with a given number of keys required
Value createmultisig(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 2)
    {
        string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are Bean Cash addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) Bean Cash address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            "  \nBeancashd createmultisig 2 [\\\"2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\\\",\\\"2ZkipiuqPaKwgMVxth5skFep6pRdeTf3BK\\\"]\n"
        ;
        throw runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig(params);
    CScriptID innerID = inner.GetID();
    CBitbeanAddress address(innerID);

    Object result;
    result.push_back(Pair("address", address.ToString()));
    result.push_back(Pair("redeemScript", HexStr(inner.begin(), inner.end())));
    return result;
}

Value addredeemscript(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
    {
        string msg = "addredeemscript <redeemScript> [account]\n"
            "Add a P2SH address with a specified redeemScript to the wallet.\n"
            "If [account] is specified, assign address to [account].";
        throw runtime_error(msg);
    }

    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Construct using pay-to-script-hash:
    vector<unsigned char> innerData = ParseHexV(params[0], "redeemScript");
    CScript inner(innerData.begin(), innerData.end());
    CScriptID innerID = inner.GetID();
    if (!pwalletMain->AddCScript(inner))
        throw runtime_error("AddCScript() failed");

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return CBitbeanAddress(innerID).ToString();
}

struct tallyitem
{
    int64_t nAmount;
    int nConf;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
    }
};

Value ListReceived(const Array& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    // Tally
    map<CBitbeanAddress, tallyitem> mapTally;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;

        if (wtx.IsBeanBase() || wtx.IsBeanStake() || !wtx.IsFinal())
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < nMinDepth)
            continue;

        for (const CTxOut& txout : wtx.vout)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address) || !IsMine(*pwalletMain, address))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = min(item.nConf, nDepth);
        }
    }

    // Reply
    Array ret;
    map<string, tallyitem> mapAccountTally;
    for (const std::pair<CBitbeanAddress, CAddressBookData>& item : pwalletMain->mapAddressBook)
    {
        const CBitbeanAddress& address = item.first;
        const string& strAccount = item.second.name;
        map<CBitbeanAddress, tallyitem>::iterator it = mapTally.find(address);
        if (it == mapTally.end() && !fIncludeEmpty)
            continue;

        int64_t nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        if (it != mapTally.end())
        {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
        }

        if (fByAccounts)
        {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = min(item.nConf, nConf);
        }
        else
        {
            Object obj;
            obj.push_back(Pair("address",       address.ToString()));
            obj.push_back(Pair("account",       strAccount));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    if (fByAccounts)
    {
        for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
        {
            int64_t nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            Object obj;
            obj.push_back(Pair("account",       (*it).first));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}

// List balances by receiving address
Value listreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
             "listreceivedbyaddress ( minconf include empty )\n"
             "\nList balances by receiving address.\n"
             "\nArguments:\n"
             "1. minconf       (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
             "2. includeempty  (numeric, optional, dafault=false) Whether to include addresses that haven't received any payments.\n"

             "\nResult:\n"
             "[\n"
             "  {\n"
             "    \"address\" : \"receivingaddress\",  (string) The receiving address\n"
             "    \"account\" : \"accountname\",       (string) The account of the receiving address. The default account is \"\".\n"
             "    \"amount\" : x.xxx,                  (numeric) The total amount in bitb received by the address\n"
             "    \"confirmations\" : n                (numeric) The number of confirmations of the most recent transaction included\n"
             "  }\n"
             "  ,...\n"
             "]\n"

             "\nExamples:\n"
             "  \nBeancashd listreceivedbyaddress 6 true\n"
         );

    return ListReceived(params, false);
}

// List balances by account
Value listreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
             "listreceivedbyaccount ( minconf include empty )\n"
             "\nList balances by account.\n"
             "\nArguments:\n"
             "1. minconf      (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
             "2. includeempty (boolean, optional, default=false) Whether to include accounts that haven't received any payments.\n"

             "\nResult:\n"
             "[\n"
             "  {\n"
             "    \"account\" : \"accountname\",  (string) The account name of the receiving account\n"
             "    \"amount\" : x.xxx,             (numeric) The total amount received by addresses with this account\n"
             "    \"confirmations\" : n           (numeric) The number of confirmations of the most recent transaction included\n"
             "  }\n"
             "  ,...\n"
             "]\n"

             "\nExamples:\n"
             "  \nBeancashd listreceivedbyaccount 6 true\n"
        );

    accountingDeprecationCheck();

    return ListReceived(params, true);
}

static void MaybePushAddress(Object & entry, const CTxDestination &dest)
{
    CBitbeanAddress addr;
    if (addr.Set(dest))
        entry.push_back(Pair("address", addr.ToString()));
}

void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
{
    int64_t nFee;
    string strSentAccount;
    list<pair<CTxDestination, int64_t> > listReceived;
    list<pair<CTxDestination, int64_t> > listSent;

    wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);

    bool fAllAccounts = (strAccount == string("*"));

    // Sent
    if ((!wtx.IsBeanStake()) && (!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    {
        for (const std::pair<CTxDestination, int64_t>& s : listSent)
        {
            Object entry;
            entry.push_back(Pair("account", strSentAccount));
            MaybePushAddress(entry, s.first);
            entry.push_back(Pair("category", "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }
    }

    // Received
    if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    {
        bool stop = false;
        for (const std::pair<CTxDestination, int64_t>& r : listReceived)
        {
            string account;
            if (pwalletMain->mapAddressBook.count(r.first))
                account = pwalletMain->mapAddressBook[r.first].name;
            if (fAllAccounts || (account == strAccount))
            {
                Object entry;
                entry.push_back(Pair("account", account));
                MaybePushAddress(entry, r.first);
                if (wtx.IsBeanBase() || wtx.IsBeanStake())
                {
                    if (wtx.GetDepthInMainChain() < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                }
                else
                {
                    entry.push_back(Pair("category", "receive"));
                }
                if (!wtx.IsBeanStake())
                    entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                else
                {
                    entry.push_back(Pair("amount", ValueFromAmount(-nFee)));
                    stop = true; // only one beansprout output
                }
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
            if (stop)
                break;
        }
    }
}

void AcentryToJSON(const CAccountingEntry& acentry, const string& strAccount, Array& ret)
{
    bool fAllAccounts = (strAccount == string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount)
    {
        Object entry;
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", (int64_t)acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

// List transactions
Value listtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
             "listtransactions ( \"account\" count from )\n"
             "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
             "\nArguments:\n"
             "1. \"account\"    (string, optional) The account name. If not included, it will list all transactions for all accounts.\n"
             "                                     If \"\" is set, it will list transactions for the default account.\n"
             "2. count          (numeric, optional, default=10) The number of transactions to return\n"
             "3. from           (numeric, optional, default=0) The number of transactions to skip\n"

             "\nResult:\n"
             "[\n"
             "  {\n"
             "    \"account\":\"accountname\",       (string) The account name associated with the transaction. \n"
             "                                                It will be \"\" for the default account.\n"
             "    \"address\":\"beancashaddress\",    (string) The beancash address of the transaction. Not present for \n"
             "                                                move transactions (category = move).\n"
             "    \"category\":\"send|receive|move\", (string) The transaction category. 'move' is a local (off blockchain)\n"
             "                                                transaction between accounts, and not associated with an address,\n"
             "                                                transaction id or block. 'send' and 'receive' transactions are \n"
             "                                                associated with an address, transaction id and block details\n"
             "    \"amount\": x.xxx,          (numeric) The amount in btc. This is negative for the 'send' category, and for the\n"
             "                                         'move' category for moves outbound. It is positive for the 'receive' category,\n"
             "                                         and for the 'move' category for inbound funds.\n"
             "    \"fee\": x.xxx,             (numeric) The amount of the fee in bitb. This is negative and only available for the \n"
             "                                         'send' category of transactions.\n"
             "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and \n"
             "                                         'receive' category of transactions.\n"
             "    \"blockhash\": \"hashvalue\", (string) The block hash containing the transaction. Available for 'send' and 'receive'\n"
             "                                          category of transactions.\n"
             "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive'\n"
             "                                          category of transactions.\n"
             "    \"txid\": \"transactionid\", (string) The transaction id (see https://chainz.cryptoid.info/bean/api.dws?t=[transactionID]. Available \n"
             "                                          for 'send' and 'receive' category of transactions.\n"
             "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (midnight Feb 13 2015 GMT).\n"
             "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (midnight Feb 13 2015 GMT). Available \n"
             "                                          for 'send' and 'receive' category of transactions.\n"
             "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
             "    \"otheraccount\": \"accountname\",  (string) For the 'move' category of transactions, the account the funds came \n"
             "                                          from (for receiving funds, positive amounts), or went to (for sending funds,\n"
             "                                          negative amounts).\n"
             "  }\n"
             "]\n"

             "\nExamples:\n"
             "\nList the most recent 10 transactions in the systems\n"
             "  \nBeancashd listtransactions\n"
             "\nList the most recent 10 transactions for the teambean account\n"
             "  \nBeancashd listtransactions teambean\n"
             "\nList transactions 100 to 120 from the teambean account\n"
             "  /nBeancashd listtransactions teambean 20 100\n"
        );

    string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    Array ret;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, strAccount);

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret);
        CAccountingEntry *const pacentry = (*it).second.second;
        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount+nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;
    Array::iterator first = ret.begin();
    std::advance(first, nFrom);
    Array::iterator last = ret.begin();
    std::advance(last, nFrom+nCount);

    if (last != ret.end()) ret.erase(last, ret.end());
    if (first != ret.begin()) ret.erase(ret.begin(), first);

    std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    return ret;
}

// List accounts
Value listaccounts(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
             "listaccounts ( minconf )\n"
             "\nReturns Object that has account names as keys, account balances as values.\n"
             "\nArguments:\n"
             "1. minconf     (numeric, optional, default=1) Only onclude transactions with at least this many confirmations\n"
             "\nResult:\n"
             "{                      (json object where keys are account names, and values are numeric balances\n"
             "  \"account\": x.xxx,  (numeric) The property name is the account name, and the value is the total balance for the account.\n"
             "  ...\n"
             "}\n"
             "\nExamples:\n"
             "\nList account balances where there at least 1 confirmation\n"
             "  \nBeancashd listaccounts\n"
             "\nList account balances including zero confirmation transactions\n"
             "  \nBeancashd listaccounts 0\n"
             "\nList account balances for 6 or more confirmations\n"
             "  \nBeancashd listaccounts 6\n"
         );

    accountingDeprecationCheck();

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    map<string, int64_t> mapAccountBalances;
    for (const std::pair<CTxDestination, CAddressBookData>& entry : pwalletMain->mapAddressBook) {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
            mapAccountBalances[entry.second.name] = 0;
    }

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        int64_t nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64_t> > listReceived;
        list<pair<CTxDestination, int64_t> > listSent;
        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0)
            continue;
        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);
        mapAccountBalances[strSentAccount] -= nFee;
        for (const std::pair<CTxDestination, int64_t>& s : listSent)
            mapAccountBalances[strSentAccount] -= s.second;
        if (nDepth >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
        {
            for (const std::pair<CTxDestination, int64_t>& r : listReceived)
                if (pwalletMain->mapAddressBook.count(r.first))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.first].name] += r.second;
                else
                    mapAccountBalances[""] += r.second;
        }
    }

    list<CAccountingEntry> acentries;
    CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
    for (const CAccountingEntry& entry : acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    Object ret;
    for (const std::pair<string, int64_t>& accountBalance : mapAccountBalances) {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}

// List of all transactions since a given block
Value listsinceblock(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
             "listsinceblock ( \"blockhash\" target-confirmations )\n"
             "\nGet all transactions in blocks since block [blockhash], or all transactions if omitted\n"
             "\nArguments:\n"
             "1. \"blockhash\"   (string, optional) The block hash to list transactions since\n"
             "2. target-confirmations:    (numeric, optional) The confirmations required, must be 1 or more\n"
             "\nResult:\n"
             "{\n"
             "  \"transactions\": [\n"
             "    \"account\":\"accountname\",       (string) The account name associated with the transaction. Will be \"\" for the default account.\n"
             "    \"address\":\"beancashaddress\",    (string) The Bean Cash address of the transaction. Not present for move transactions (category = move).\n"
             "    \"category\":\"send|receive\",     (string) The transaction category. 'send' has negative amounts, 'receive' has positive amounts.\n"
             "    \"amount\": x.xxx,          (numeric) The amount in bitb. This is negative for the 'send' category, and for the 'move' category for moves \n"
             "                                          outbound. It is positive for the 'receive' category, and for the 'move' category for inbound funds.\n"
             "    \"fee\": x.xxx,             (numeric) The amount of the fee in bitb. This is negative and only available for the 'send' category of transactions.\n"
             "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and 'receive' category of transactions.\n"
             "    \"blockhash\": \"hashvalue\",     (string) The block hash containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
             "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
             "    \"blocktime\": xxx,         (numeric) The block time in seconds since epoch (Feb 13 2015 GMT).\n"
             "    \"txid\": \"transactionid\",  (string) The transaction id (see https://chainz.cryptoid.info/bean/api.dws?t=[transactionID]. Available for 'send' and 'receive' category of transactions.\n"
             "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (Feb 13 2015 GMT).\n"
             "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (Feb 13 2015 GMT). Available for 'send' and 'receive' category of transactions.\n"
             "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
             "    \"to\": \"...\",            (string) If a comment to is associated with the transaction.\n"
             "  ],\n"
             "  \"lastblock\": \"lastblockhash\"     (string) The hash of the last block\n"
             "}\n"
             "\nExamples:\n"
             "  \nBeancashd listsinceblock\n"
             "  \nBeancashd listsinceblock 0000000000000042c0c267858f0064f213c2e447fbce1675baad75d0ade1da13 6\n"
         );

    CBlockIndex *pindex = NULL;
    int target_confirms = 1;

    if (params.size() > 0)
    {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        pindex = CBlockLocator(blockId).GetBlockIndex();
    }

    if (params.size() > 1)
    {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    }

    int depth = pindex ? (1 + nBestHeight - pindex->nHeight) : -1;

    Array transactions;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain() < depth)
            ListTransactions(tx, "*", 0, true, transactions);
    }

    uint256 lastblock;

    if (target_confirms == 1)
    {
        lastblock = hashBestChain;
    }
    else
    {
        int target_height = pindexBest->nHeight + 1 - target_confirms;

        CBlockIndex *block;
        for (block = pindexBest;
             block && block->nHeight > target_height;
             block = block->pprev)  { }

        lastblock = block ? block->GetBlockHash() : 0;
    }

    Object ret;
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}

// Get detailed information about an in-wallet transaction
Value gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "gettransaction \"txid\"\n"
             "\nGet detailed information about in-wallet transaction <txid>\n"
             "\nArguments:\n"
             "1. \"txid\"    (string, required) The transaction id\n"
             "\nResult:\n"
             "{\n"
             "  \"amount\" : x.xxx,        (numeric) The transaction amount in bitb\n"
             "  \"confirmations\" : n,     (numeric) The number of confirmations\n"
             "  \"blockhash\" : \"hash\",  (string) The block hash\n"
             "  \"blockindex\" : xx,       (numeric) The block index\n"
             "  \"blocktime\" : ttt,       (numeric) The time in seconds since epoch (1 Jan 1970 GMT)\n"
             "  \"txid\" : \"transactionid\",   (string) The transaction id, see also https://chainz.cryptoid.info/bean/api.dws?t=[transactionID]\n"
             "  \"time\" : ttt,            (numeric) The transaction time in seconds since epoch (1 Jan 1970 GMT)\n"
             "  \"timereceived\" : ttt,    (numeric) The time received in seconds since epoch (1 Jan 1970 GMT)\n"
             "  \"details\" : [\n"
             "    {\n"
             "      \"account\" : \"accountname\",  (string) The account name involved in the transaction, can be \"\" for the default account.\n"
             "      \"address\" : \"beancashaddress\",   (string) The Bean Cash address involved in the transaction\n"
             "      \"category\" : \"send|receive\",    (string) The category, either 'send' or 'receive'\n"
             "      \"amount\" : x.xxx                  (numeric) The amount in bitb\n"
             "    }\n"
             "    ,...\n"
             "  ]\n"
             "}\n"

             "\nExamples\n"
             "  \nBeancashd gettransaction 5b13c95aff35384afe26691e31701b77df8c650b6d794e3885adb71afc43b787\n"
         );

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;

    if (pwalletMain->mapWallet.count(hash))
    {
        const CWalletTx& wtx = pwalletMain->mapWallet[hash];

        TxToJSON(wtx, 0, entry);

        int64_t nCredit = wtx.GetCredit();
        int64_t nDebit = wtx.GetDebit();
        int64_t nNet = nCredit - nDebit;
        int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

        entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
        if (wtx.IsFromMe())
            entry.push_back(Pair("fee", ValueFromAmount(nFee)));

        WalletTxToJSON(wtx, entry);

        Array details;
        ListTransactions(pwalletMain->mapWallet[hash], "*", 0, false, details);
        entry.push_back(Pair("details", details));
    }
    else
    {
        CTransaction tx;
        uint256 hashBlock = 0;
        if (GetTransaction(hash, tx, hashBlock))
        {
            TxToJSON(tx, 0, entry);
            if (hashBlock == 0)
                entry.push_back(Pair("confirmations", 0));
            else
            {
                entry.push_back(Pair("blockhash", hashBlock.GetHex()));
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
                if (mi != mapBlockIndex.end() && (*mi).second)
                {
                    CBlockIndex* pindex = (*mi).second;
                    if (pindex->IsInMainChain())
                        entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
                    else
                        entry.push_back(Pair("confirmations", 0));
                }
            }
        }
        else
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
    }

    return entry;
}

// Backup wallet
Value backupwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "backupwallet \"destination\"\n"
             "\nSafely copies wallet.dat to destination, which can be a directory or a path with filename.\n"
             "\nArguments:\n"
             "1. \"destination\"   (string) The destination directory or file\n"
             "\nExamples:\n"
             "  \nBeancashd backupwallet ~/.BitBean/backup.dat\n"
        );

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}

// Refil the keypool
Value keypoolrefill(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
             "keypoolrefill ( newsize )\n"
             "\nFills the keypool."
             + HelpRequiringPassphrase() + "\n"
             "\nArguments\n"
             "1. newsize     (numeric, optional, default=100) The new keypool size\n"
             "\nExamples:\n"
             "  \nBeancashd keypoolrefill\n"
        );

    unsigned int nSize = max(GetArg("-keypool", 100), (int64_t)0);
    if (params.size() > 0) {
        if (params[0].get_int() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size");
        nSize = (unsigned int) params[0].get_int();
    }

    EnsureWalletIsUnlocked();

    pwalletMain->TopUpKeyPool(nSize);

    if (pwalletMain->GetKeyPoolSize() < nSize)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");

    return Value::null;
}


void ThreadTopUpKeyPool(void* parg)
{
    // Make this thread recognisable as the key-topping-up thread
    RenameThread("Beancash-key-top");

    pwalletMain->TopUpKeyPool();
}

void ThreadCleanWalletPassphrase(void* parg)
{
    // Make this thread recognisable as the wallet relocking thread
    RenameThread("Beancash-lock-wa");

    int64_t nMyWakeTime = GetTimeMillis() + *((int64_t*)parg) * 1000;

    ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

    if (nWalletUnlockTime == 0)
    {
        nWalletUnlockTime = nMyWakeTime;

        do
        {
            if (nWalletUnlockTime==0)
                break;
            int64_t nToSleep = nWalletUnlockTime - GetTimeMillis();
            if (nToSleep <= 0)
                break;

            LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);
            MilliSleep(nToSleep);
            ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

        } while(1);

        if (nWalletUnlockTime)
        {
            nWalletUnlockTime = 0;
            pwalletMain->Lock();
        }
    }
    else
    {
        if (nWalletUnlockTime < nMyWakeTime)
            nWalletUnlockTime = nMyWakeTime;
    }

    LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);

    delete (int64_t*)parg;
}

// Unlock wallet for sprouting or sending Bean Cash
Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 3))
        throw runtime_error(
                "walletpassphrase \"passphrase\" timeout\n"
                "\nStores the wallet decryption key in memory for 'timeout' seconds.\n"
                "This is needed prior to performing transactions related to private keys such as sending Bean Cash\n"
                "\nArguments:\n"
                "1. \"passphrase\"     (string, required) The wallet passphrase\n"
                "2. \"timeout\"        (numeric, required) The time to keep the decryption key in seconds.\n"
                "3. \"sproutingonly\"  (boolean) If true enable sprouting and disable sending functions (default false).\n"
                "\nExamples:\n"
                "\nunlock the wallet for 60 seconds\n"
                "  \nBeancashd walletpassphrase <your passphrase> 60\n"
                "\nLock the wallet again (before 60 seconds)\n"
                "  \nBeancashd walletlock\n"
         );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    if (!pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_ALREADY_UNLOCKED, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");
    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() > 0)
    {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    }
    else
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");

    NewThread(ThreadTopUpKeyPool, NULL);
    int64_t* pnSleepTime = new int64_t(params[1].get_int64());
    NewThread(ThreadCleanWalletPassphrase, pnSleepTime);

    // ppbean: if user OS account compromised prevent trivial sendmoney commands
    if (params.size() > 2)
        fWalletUnlockStakingOnly = params[2].get_bool();
    else
        fWalletUnlockStakingOnly = false;

    return Value::null;
}

// Change wallet passphrase
Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
             "walletpassphrasechange \"oldpassphrase\" \"newpassphrase\"\n"
             "\nChanges the wallet passphrase from 'oldpassphrase' to 'newpassphrase'.\n"
             "\nArguments:\n"
             "1. \"oldpassphrase\"      (string) The current passphrase\n"
             "2. \"newpassphrase\"      (string) The new passphrase\n"
             "\nExamples:\n"
             "  \nBeancashd walletpassphrasechange <your old passphrase> <your new passphrase>\n"
         );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    return Value::null;
}

// Lock Wallet
Value walletlock(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
        throw runtime_error(
            "walletlock\n"
            "\nRemoves the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.\n"
            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            "  \nBeancashd walletpassphrase <your passphrase> 120\n"
            "\nPerform a send (requires passphrase set)\n"
            "  \nBeancashd sendtoaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 1000000\n"
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            "  \nBeancashd walletlock\n"
        );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return Value::null;
}

// Encrypt wallet
Value encryptwallet(const Array& params, bool fHelp)
{
    if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
        throw runtime_error(
             "encryptwallet \"passphrase\"\n"
             "\nEncrypts the wallet with 'passphrase'. This is for first time encryption.\n"
             "After this, any calls that interact with private keys such as sending or signing \n"
             "will require the passphrase to be set prior the making these calls.\n"
             "Use the walletpassphrase call for this, and then walletlock call.\n"
             "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
             "Note that this will shutdown the server.\n"
             "\nArguments:\n"
             "1. \"passphrase\"    (string) The pass phrase to encrypt the wallet with. It must be at least 1 character, but should be long.\n"
             "\nExamples:\n"
             "\nEncrypt you wallet\n"
             "  \nBeancashd encryptwallet <your passphrase>\n"
             "\nNow set the passphrase to use the wallet, such as for signing or sending bitcoin\n"
             "  \nBeancashd walletpassphrase <your passphrase>\n"
             "\nNow we can so something like sign\n"
             "  \nBeancashd signmessage 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4 \"Bean Cash to the Moon then Mars!\"\n"
             "\nNow lock the wallet again by removing the passphrase\n"
             "  \nBeancashd walletlock\n"
        );

    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; Bean Cash server stopping, restart to run with encrypted wallet. The keypool has been flushed, you need to make a new backup.";
}

class DescribeAddressVisitor : public boost::static_visitor<Object>
{
public:
    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        pwalletMain->GetPubKey(keyID, vchPubKey);
        obj.push_back(Pair("isscript", false));
        obj.push_back(Pair("pubkey", HexStr(vchPubKey)));
        obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        CScript subscript;
        pwalletMain->GetCScript(scriptID, subscript);
        std::vector<CTxDestination> addresses;
        txnouttype whichType;
        int nRequired;
        ExtractDestinations(subscript, whichType, addresses, nRequired);
        obj.push_back(Pair("script", GetTxnOutputType(whichType)));
        obj.push_back(Pair("hex", HexStr(subscript.begin(), subscript.end())));
        Array a;
        for (const CTxDestination& addr : addresses)
            a.push_back(CBitbeanAddress(addr).ToString());
        obj.push_back(Pair("addresses", a));
        if (whichType == TX_MULTISIG)
            obj.push_back(Pair("sigsrequired", nRequired));
        return obj;
    }
};

// Get information about a Bean Cash address
Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "validateaddress \"beancashaddress\"\n"
             "\nReturn information about the given Bean Cash address.\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"     (string, required) The Bean Cash address to validate\n"
             "\nResult:\n"
             "{\n"
             "  \"isvalid\" : true|false,         (boolean) If the address is valid or not. If not, this is the only property returned.\n"
             "  \"address\" : \"beancashaddress\", (string) The Bean Cash address validated\n"
             "  \"ismine\" : true|false,          (boolean) If the address is yours or not\n"
             "  \"isscript\" : true|false,        (boolean) If the key is a script\n"
             "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the raw public key\n"
             "  \"iscompressed\" : true|false,    (boolean) If the address is compressed\n"
             "  \"account\" : \"account\"         (string) The account associated with the address, \"\" is the default account\n"
             "}\n"
             "\nExamples:\n"
             "  \nBeancashd validateaddress 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\n"
        );

    CBitbeanAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = pwalletMain ? IsMine(*pwalletMain, dest) : false;
        ret.push_back(Pair("ismine", fMine));
        if (fMine) {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest].name));
    }
    return ret;
}

// Lock unspent outputs
Value lockunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
             "lockunspent unlock [{\"txid\":\"txid\",\"vout\":n},...]\n"
             "\nUpdates list of temporarily unspendable outputs.\n"
             "Temporarily lock (lock=true) or unlock (lock=false) specified transaction outputs.\n"
             "A locked transaction output will not be chosen by automatic bean selection, when spending Bean Cash.\n"
             "Locks are stored in memory only. Nodes start with zero locked outputs, and the locked output list\n"
             "is always cleared (by virtue of process exit) when a node stops or fails.\n"
             "Also see the listunspent call\n"
             "\nArguments:\n"
             "1. unlock            (boolean, required) Whether to unlock (true) or lock (false) the specified transactions\n"
             "2. \"transactions\"  (string, required) A json array of objects. Each object the txid (string) vout (numeric)\n"
             "     [           (json array of json objects)\n"
             "       {\n"
             "         \"txid\":\"id\",    (string) The transaction id\n"
             "         \"vout\": n         (numeric) The output number\n"
             "       }\n"
             "       ,...\n"
             "     ]\n"
             "\nResult:\n"
             "true|false    (boolean) Whether the command was successful or not\n"

             "\nExamples:\n"
             "\nList the unspent transactions\n"
             "  \nBeancashd listunspent\n"
             "\nLock an unspent transaction\n"
             "  \nBeancashd lockunspent false [{\\\"txid\\\":\\\"c48a0f58437c6e5ab5ca9443a2c0dec1dc2285e9d0e7739f18c02f937fcd9bf5\\\",\\\"vout\\\":1}]\n"
             "\nList the locked transactions\n"
             "  \nBeancashd listlockunspent\n"
             "\nUnlock the transaction again\n"
             "  \nBeancashd lockunspent true [{\\\"txid\\\":\\\"c48a0f58437c6e5ab5ca9443a2c0dec1dc2285e9d0e7739f18c02f937fcd9bf5\\\",\\\"vout\\\":1}]\n"
        );
    
    if (params.size() == 1)
        RPCTypeCheck(params, list_of(bool_type));
    else
        RPCTypeCheck(params, list_of(bool_type)(array_type));
    
    bool fUnlock = params[0].get_bool();
    
    if (params.size() == 1) {
        if (fUnlock)
            pwalletMain->UnlockAllBeans();
        return true;
    }
    
    Array outputs = params[1].get_array();
    for (Value& output : outputs)
    {
        if (output.type() != obj_type)
            throw JSONRPCError(-8, "Invalid parameter, expected object");
        const Object& o = output.get_obj();
        
        RPCTypeCheck(o, map_list_of("txid", str_type)("vout", int_type));
        
        string txid = find_value(o, "txid").get_str();
        if (!IsHex(txid))
            throw JSONRPCError(-8, "Invalid parameter, expected hex txid");

        int nOutput = find_value(o, "vout").get_int();
        if (nOutput < 0)
            throw JSONRPCError(-8, "Invalid parameter, vout must be positive");
        
        COutPoint outpt(uint256(txid), nOutput);
        
        if (fUnlock)
        		pwalletMain->UnlockBean(outpt);
        else
        		pwalletMain->LockBean(outpt);

    }
    
    return true;
}

// List temporarily unspendable outputs
Value listlockunspent(const Array& params, bool fHelp)
{
	if (fHelp || params.size() > 0)
		throw runtime_error(
			"listlockunspent\n"
            "\nReturns list of temporarily unspendable outputs.\n"
            "See the lockunspent call to lock and unlock transactions for spending.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"transactionid\",     (string) The transaction id locked\n"
            "    \"vout\" : n                      (numeric) The vout value\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            "\nList the unspent transactions\n"
            "  \nBeancashd listunspent\n"
            "\nLock an unspent transaction\n"
            "  \nBeancashd lockunspent false [{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\n"
            "\nList the locked transactions\n"
            "  \nBeancashd listlockunspent\n"
            "\nUnlock the transaction again\n"
            "  \nBeancashd lockunspent true [{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\n"
        );
			
	vector<COutPoint> vOutpts;
	pwalletMain->ListLockedBeans(vOutpts);
	
	Array ret;
	
	for (COutPoint &outpt : vOutpts) {
		Object o;
		
		o.push_back(Pair("txid", outpt.hash.GetHex()));
		o.push_back(Pair("vout", (int)outpt.n));
		ret.push_back(o);	
	}
	
	return ret;			
}

// Validate a pubkey
Value validatepubkey(const Array& params, bool fHelp)
{
    if (fHelp || !params.size() || params.size() > 2)
        throw runtime_error(
            "validatepubkey <Bean Cash pubkey>\n"
            "Return information about <Bean Cash pubkey>.");

    std::vector<unsigned char> vchPubKey = ParseHex(params[0].get_str());
    CPubKey pubKey(vchPubKey);

    bool isValid = pubKey.IsValid();
    bool isCompressed = pubKey.IsCompressed();
    CKeyID keyID = pubKey.GetID();

    CBitbeanAddress address;
    address.Set(keyID);

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = IsMine(*pwalletMain, dest);
        ret.push_back(Pair("ismine", fMine));
        ret.push_back(Pair("iscompressed", isCompressed));
        if (fMine) {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest].name));
    }
    return ret;
}

// Reserve balance from being sprouted for network protection
Value reservebalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "reservebalance [<reserve> [amount]]\n"
            "<reserve> is true or false to turn balance reserve on or off.\n"
            "<amount> is a real and rounded to cent.\n"
            "Set reserve amount not participating in network protection.\n"
            "If no parameters provided current setting is printed.\n");

    if (params.size() > 0)
    {
        bool fReserve = params[0].get_bool();
        if (fReserve)
        {
            if (params.size() == 1)
                throw runtime_error("must provide amount to reserve balance.\n");
            int64_t nAmount = AmountFromValue(params[1]);
            nAmount = (nAmount / CENT) * CENT;  // round to cent
            if (nAmount < 0)
                throw runtime_error("amount cannot be negative.\n");
            mapArgs["-reservebalance"] = FormatMoney(nAmount);
        }
        else
        {
            if (params.size() > 1)
                throw runtime_error("cannot specify amount to turn off reserve.\n");
            mapArgs["-reservebalance"] = "0";
        }
    }

    Object result;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        throw runtime_error("Invalid reserve balance amount\n");
    result.push_back(Pair("reserve", (nReserveBalance > 0)));
    result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
    return result;
}


// Check wallet integrity
Value checkwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "checkwallet\n"
            "Check wallet for integrity.\n");

    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    pwalletMain->FixSpentBeans(nMismatchSpent, nBalanceInQuestion, true);
    Object result;
    if (nMismatchSpent == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent beans", nMismatchSpent));
        result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
    }
    return result;
}


// Repair wallet
Value repairwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "repairwallet\n"
            "Repair wallet if checkwallet reports any problem.\n");

    int nMismatchSpent;
    int64_t nBalanceInQuestion;
    pwalletMain->FixSpentBeans(nMismatchSpent, nBalanceInQuestion);
    Object result;
    if (nMismatchSpent == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent beans", nMismatchSpent));
        result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
    }
    return result;
}

// Bitbean: resend unconfirmed wallet transactions
Value resendtx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "resendtx\n"
            "Re-send unconfirmed transactions.\n"
        );

    ResendWalletTransactions(true);

    return Value::null;
}

// Make a public-private key pair
Value makekeypair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "makekeypair [prefix]\n"
            "Make a public/private key pair.\n"
            "[prefix] is optional preferred prefix for the public key.\n");

    string strPrefix = "";
    if (params.size() > 0)
        strPrefix = params[0].get_str();
 
    CKey key;
    key.MakeNewKey(false);

    CPrivKey vchPrivKey = key.GetPrivKey();
    Object result;
    result.push_back(Pair("PrivateKey", HexStr<CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
    result.push_back(Pair("PublicKey", HexStr(key.GetPubKey())));
    return result;
}

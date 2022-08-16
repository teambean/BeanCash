// Copyright (c) 2009-2012 Bitcoin Developers
// Copyright (c) 2015-2022 Bean Core www.beancash.org
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h" // for pwalletMain
#include "bitbeanrpc.h"
#include "ui_interface.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

void EnsureWalletIsUnlocked();

class CTxDump
{
public:
    CBlockIndex *pindex;
    int64_t nValue;
    bool fSpent;
    CWalletTx* ptx;
    int nOut;
    CTxDump(CWalletTx* ptx = NULL, int nOut = -1)
    {
        pindex = NULL;
        nValue = 0;
        fSpent = false;
        this->ptx = ptx;
        this->nOut = nOut;
    }
};

// Import a private key
Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "importprivkey \"beancashprivkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"beancashprivkey\"  (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional) an optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            "  \nBitbeand dumpprivkey 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\n"
            "\nImport the private key\n"
            "  \nBeancashd importprivkey <replacewithyourkey>"
            "\nImport using a label\n"
            "  \nBeancashd importprivkey <replacewithyourkey> testing false\n"
         );


    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();
    CBitbeanSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    if (fWalletUnlockStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Wallet is unlocked for Sprouting only.");

    CKey key = vchSecret.GetKey();
    CPubKey pubkey = key.GetPubKey();
    CKeyID vchAddress = pubkey.GetID();
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBookName(vchAddress, strLabel);

        // Don't throw error in case a key is already there
        if (pwalletMain->HaveKey(vchAddress))
            return Value::null;

        pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

        if (!pwalletMain->AddKeyPubKey(key, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

        // whenever a key is imported, we need to scan the whole chain
        pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'

        pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
        pwalletMain->ReacceptWalletTransactions();
    }

    return Value::null;
}

// Import wallet from a file
Value importwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "importwallet \"filename\"\n"
             "\nImports keys from a wallet dump file (see dumpwallet).\n"
             "\nArguments:\n"
             "1. \"filename\"    (string, required) The wallet file\n"
             "\nExamples:\n"
             "\nDump the wallet\n"
             "  \nBeancashd dumpwallet ~/.BitBean/wallet_backup_08152022\n"
             "\nImport the wallet\n"
             "  \nBeancashd importwallet ~/.BitBean/wallet_backup_08152022\n"
             + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();
    if(!ImportWallet(pwalletMain, params[0].get_str()))
       throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");
    return Value::null;
}


Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "dumpprivkey \"beancashaddress\"\n"
             "\nReveals the private key corresponding to 'beancashaddress'.\n"
             "Then the importprivkey can be used with this output\n"
             "\nArguments:\n"
             "1. \"beancashaddress\"   (string, required) The beancash address for the private key\n"
             "\nResult:\n"
             "\"key\"                (string) The private key\n"
             "\nExamples:\n"
             "  \nBeancashd dumpprivkey 2VmJUEhuYQZA491FGdN1q6nJLTzahDG3o4\n"
             "  \nBeancashd importprivkey <yourprivatekey>\n"
        );

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CBitbeanAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bean Cash address");
    if (fWalletUnlockStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Wallet is unlocked for Sprouting only.");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CKey vchSecret;
    if (!pwalletMain->GetKey(keyID, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitbeanSecret(vchSecret).ToString();
}

Value dumpwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
             "dumpwallet \"filename\"\n"
             "\nDumps all wallet keys in a human-readable format.\n"
             "\nArguments:\n"
             "1. \"filename\"    (string, required) The filename\n"
             "\nExamples:\n"
             "  \nBeancashd dumpwallet ~/.BitBean/InsecurefileWithMyPrivatekeys.txt (DONT DO THIS ON M$ WINDOZ!)\n"
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    if(!DumpWallet(pwalletMain, params[0].get_str().c_str() ))
       throw JSONRPCError(RPC_WALLET_ERROR, "Error dumping wallet keys to file");
    return Value::null;
}

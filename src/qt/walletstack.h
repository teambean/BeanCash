/*
 * Qt4 Bean Cash GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2013
 * Bean Core www.beancash.org 2021
 */
#ifndef WALLETSTACK_H
#define WALLETSTACK_H

#include <QStackedWidget>
#include <QMap>
#include <boost/shared_ptr.hpp>

class BitbeanGUI;
class TransactionTableModel;
class ClientModel;
class WalletModel;
class WalletView;
class TransactionView;
class OverviewPage;
class AddressBookPage;
class SendBeansDialog;
class SignVerifyMessageDialog;
class Notificator;
class RPCConsole;

class CWalletManager;

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
QT_END_NAMESPACE

/*
  WalletStack class. This class is a container for WalletView instances. It takes the place of centralWidget.
  It was added to support multiple wallet functionality. It communicates with both the client and the
  wallet models to give the user an up-to-date view of the current core state. It manages all the wallet views
  it contains and updates them accordingly.
 */
class WalletStack : public QStackedWidget
{
    Q_OBJECT
public:
    explicit WalletStack(QWidget *parent = 0);
    ~WalletStack();

    void setBitbeanGUI(BitbeanGUI *gui) { this->gui = gui; }
    
    void setClientModel(ClientModel *clientModel) { this->clientModel = clientModel; }
    
    bool addWallet(const QString& name, WalletModel *walletModel);
    bool removeWallet(const QString& name);
    
    void removeAllWallets();

    bool handleURI(const QString &uri);
    
    void showOutOfSyncWarning(bool fShow);

private:
    BitbeanGUI *gui;
    ClientModel *clientModel;
    QMap<QString, WalletView*> mapWalletViews;

    bool bOutOfSync;

public slots:
    void setCurrentWallet(const QString& name);

    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to address book page */
    void gotoAddressBookPage();
    /** Switch to receive beans page */
    void gotoReceiveBeansPage();
    /** Switch to send beans page */
    void gotoSendBeansPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Export key pairs */
    void dumpWallet();
    /** Import key pairs */
    void importWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();
    /** Lock Wallet */
    void lockWallet();

    /** Set the encryption status as shown in the UI.
     @param[in] status            current encryption status
     @see WalletModel::EncryptionStatus
     */
    void setEncryptionStatus();
};

#endif // WALLETSTACK_H

#ifndef BITBEANGUI_H
#define BITBEANGUI_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMap>

#include <stdint.h>

class TransactionTableModel;
class WalletFrame;
class WalletView;
class ClientModel;
class WalletModel;
class WalletStack;
class TransactionView;
class OverviewPage;
class AddressBookPage;
class SendBeansDialog;
class SignVerifyMessageDialog;
class Notificator;
class RPCConsole;

class CWallet;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTableView;
class QAbstractItemModel;
class QModelIndex;
class QProgressBar;
class QStackedWidget;
class QUrl;
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

/**
  Bitbean GUI main class. This class represents the main window of the Bitbean UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class BitbeanGUI : public QMainWindow
{
    Q_OBJECT
public:
    static const QString DEFAULT_WALLET;
    
    explicit BitbeanGUI(QWidget *parent = 0);
    ~BitbeanGUI();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a beancash wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */

    bool addWallet(const QString& name, WalletModel *walletModel);
    bool setCurrentWallet(const QString& name);
    
    void removeAllWallets();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private:
    ClientModel *clientModel;
    WalletFrame *walletFrame;

    QLabel *labelEncryptionIcon;
    QLabel *labelStakingIcon;
    QLabel *labelConnectionsIcon;
    QLabel *labelBlocksIcon;
    QLabel *progressBarLabel;
    QProgressBar *progressBar;

    QMenuBar *appMenuBar;
    QAction *overviewAction;
    QAction *historyAction;
    QAction *quitAction;
    QAction *sendBeansAction;
    QAction *addressBookAction;
    QAction *signMessageAction;
    QAction *verifyMessageAction;
    QAction *aboutAction;
    QAction *receiveBeansAction;
    QAction *optionsAction;
    QAction *toggleHideAction;
    QAction *exportAction;
    QAction *encryptWalletAction;
    QAction *backupWalletAction;
    QAction *dumpWalletAction;
    QAction *importWalletAction;
    QAction *changePassphraseAction;
    QAction *unlockWalletAction;
    QAction *lockWalletAction;
    QAction *aboutQtAction;
    QAction *openRPCConsoleAction;
    
    QSystemTrayIcon *trayIcon;
    Notificator *notificator;
    TransactionView *transactionView;
    RPCConsole *rpcConsole;

    /** Keep track of previous number of blocks, to detect progress */
    int prevBlocks;
    int spinnerFrame;

    uint64_t nWeight;

    /** Create the main UI actions. */
    void createActions();
    /** Create the menu bar and sub-menus. */
    void createMenuBar();
    /** Create the toolbars */
    void createToolBars();
    /** Create system tray icon and notification */
    void createTrayIcon();
    /** Create system tray menu (or setup the dock menu) */
    void createTrayIconMenu();

public slots:
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count, int nTotalBlocks);
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionStatus(int status);

    /** Notify the user of an event from the core network or transaction handling code.
            @param[in] title     the message box / notification title
            @param[in] message   the displayed text
            @param[in] style     modality and style definitions (icon and used buttons - buttons only for message boxes)
            @see CClientUIInterface::MessageBoxFlags
        */

    void message(const QString &title, const QString &message, unsigned int style);

    /** Asks the user whether to pay the transaction fee or to cancel the transaction.
       It is currently not possible to pass a return value to another thread through
       BlockingQueuedConnection, so an indirected pointer is used.
       https://bugreports.qt-project.org/browse/QTBUG-10440

      @param[in] nFeeRequired       the required fee
      @param[out] payFee            true to pay the fee, false to not pay the fee
    */
    void askFee(qint64 nFeeRequired, bool *payFee);
    void handleURI(QString strURI);

    /** Show incoming transaction notification for new transactions. */
    void incomingTransaction(const QString& date, int unit, qint64 amount, const QString& type, const QString& address);

private slots:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to address book page */
    void gotoAddressBookPage();
    /** Switch to receive beans page */
    void gotoReceiveBeansPage();
    /** Switch to send beans page */
    void gotoSendBeansPage();
    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show configuration dialog */
    void optionsClicked();
    /** Show about dialog */
    void aboutClicked();
#ifndef Q_OS_MAC
    /** Handle tray icon clicked */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
#endif
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Export Key Pair */
    void dumpWallet();
    /** Import Key Pair */
    void importWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();
    /** Lock Wallet */
    void lockWallet();

    /** Show window if hidden, unminimize when minimized, rise when obscured or show if hidden and fToggleHidden is true */
    void showNormalIfMinimized(bool fToggleHidden = false);
    /** simply calls showNormalIfMinimized(true) for use in SLOT() macro */
    void toggleHidden();
    /** Called by a timer to check if fRequestShutdown has been set **/
    void detectShutdown();
    void updateWeight();
    void updateStakingIcon();
};

#endif

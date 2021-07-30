/*
 * Qt4 Bean Cash GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2012
 * Bean Core www.beancash.org 2021
 *
 */

#include <QApplication>

#include "bitbeangui.h"

#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendbeansdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "bitbeanunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "ui_interface.h"
#include "rpcconsole.h"
#include "ui_interface.h"
#include "wallet.h"
#include "init.h"
#include "blockbrowser.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>

#include <QPushButton>
#include <QStyle>

// #include <QLineEdit>
// #include <QPushButton>
// #include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QDragEnterEvent>

#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#include <QMimeData>
#include <QStyle>
#include <QFontDatabase>

#include <QSettings>
#include <iostream>

extern CWallet* pwalletMain;
extern int64_t nLastBeanStakeSearchInterval;
double GetPoSKernelPS();

BitbeanGUI::BitbeanGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    unlockWalletAction(0),
    lockWalletAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    nWeight(0),
    prevBlocks(0),
    spinnerFrame(0)
{
    GUIUtil::restoreWindowGeometry("nWindow", QSize(850, 550), this);
    setWindowTitle(tr("Bean Cash") + " - " + tr("Core v1.3.2"));
    qApp->setStyleSheet("QMainWindow { border-image: url(:images/bkg);border:none; } QProgressBar { background: transparent; border: 1px solid gray; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #d3eeaf, stop: 1 #9fd555); border-radius: 7px; margin: 0px; } QMenu { background-color: #d3eeaf; color: black; } QMenu::item { color: black; background: transparent; } QMenu::item:selected { background-color: #9fd555; } QMenuBar { background-color: #d3eeaf; color: black; } QPushButton {background-color: #d3eeaf; } QLineEdit { background-color: white; } QToolTip { color: #000000; background-color: #d3eeaf; border-radius: 7px; border: 1px solid black; } QTabWidget::pane {background-color: white; color: black } QTabWidget::tab-bar {left: 5px;} QTabBar::tab {background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #d3eeaf, stop: 1 #9fd555); border: 2px solid #C4C4C3; border-bottom-color: #C2C7CB; border-top-left-radius: 7px; border-top-right-radius: 7px; min-width: 8ex; padding: 2px; color: black; } QTabBar::tab:selected {border-color: #9fd555; border-bottom-color: #d3eeaf; background-color: white; color: black; } QTabBar:tab:!selected {margin-top: 3px; } ");
    QFontDatabase::addApplicationFont("::/res/fonts/helvetica");
    QFont font("Helvetica");
    font.setPointSize(11);
    QApplication::setFont(font);

#ifndef Q_OS_MAC
    qApp->setWindowIcon(QIcon(":icons/beancash"));
    setWindowIcon(QIcon(":icons/beancash"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon();

    // Create tabs
    overviewPage = new OverviewPage();
    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    transactionsPage->setLayout(vbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveBeansPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendBeansPage = new SendBeansDialog(this);

    blockBrowser = new BlockBrowser(this);

    signVerifyMessageDialog = new SignVerifyMessageDialog(this);

    centralWidget = new QStackedWidget(this);
    centralWidget->addWidget(overviewPage);
    // centralWidget->addWidget(blockBrowser);
    centralWidget->addWidget(transactionsPage);
    centralWidget->addWidget(addressBookPage);
    centralWidget->addWidget(receiveBeansPage);
    centralWidget->addWidget(sendBeansPage);
    setCentralWidget(centralWidget);

    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelEncryptionIcon = new QLabel();
    labelStakingIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelEncryptionIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelStakingIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    if (GetBoolArg("-sprout", true))
    {
        QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        timerStakingIcon->start(30 * 1000);
        updateStakingIcon();
    }

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = qApp->style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
      //  progressBar->setStyleSheet("QProgressBar { background-color: #ffffff; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #d3eeaf, stop: 1 #9fd555); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    // prevent an open debug window from getting stuck during shutdown
    // connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // Clicking on the "Send Beans" in the address book sends you to the Send Beans tab
    connect(addressBookPage, SIGNAL(sendBeans(QString)), this, SLOT(gotoSendBeansPage(QString)));
    // Clicking on "Verify Message" in the address book opens the verify message tab in the Sign/Verigy Message dialog
    connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive beans page opens the sign message tab in the Sign/Verify Message dialog
    connect(receiveBeansPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));

    gotoOverviewPage();
}

BitbeanGUI::~BitbeanGUI()
{
    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
#endif
}

void BitbeanGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setToolTip(tr("Overview of general information"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendBeansAction = new QAction(QIcon(":/icons/left_arrow"), tr("&Send Beans"), this);
    sendBeansAction->setToolTip(tr("Send Beans to a Bean Cash address"));
    sendBeansAction->setCheckable(true);
    sendBeansAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendBeansAction);

    receiveBeansAction = new QAction(QIcon(":/icons/right_arrow"), tr("&Receive Beans"), this);
    receiveBeansAction->setToolTip(tr("Show the list of addresses for receiving payments"));
    receiveBeansAction->setCheckable(true);
    receiveBeansAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveBeansAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Address Book"), this);
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    // blockAction = new QAction(QIcon(":/icons/blockexplorer"), tr("&Block Explorer"), this);
    // blockAction->setToolTip(tr("Goto Block Explorer"));
    // blockAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    // blockAction->setCheckable(true);
    // tabGroup->addAction(blockAction);

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendBeansAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendBeansAction, SIGNAL(triggered()), this, SLOT(gotoSendBeansPage()));
    connect(receiveBeansAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveBeansAction, SIGNAL(triggered()), this, SLOT(gotoReceiveBeansPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));

    // Dialog items
    blockAction = new QAction(QIcon(":/icons/blockexplorer"), tr("&Block Explorer"), this);
    blockAction->setStatusTip(tr("Explore the Bean Cash Blockchain and Transactions"));
    blockAction->setToolTip(blockAction->statusTip());

    connect(blockAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(blockAction, SIGNAL(triggered()), this, SLOT(gotoBlockBrowser()));


    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/about"), tr("&About Bean Cash"), this);
    aboutAction->setToolTip(tr("Show information about Bean Cash"));
    aboutAction->setMenuRole(QAction::AboutRole);
#if QT_VERSION < 0x050000
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#else
    aboutQtAction = new QAction(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#endif
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setToolTip(tr("Modify configuration options for Bean Cash"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/beancash"), tr("&Show / Hide"), this);
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Keys..."), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt vault"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Keys ..."), this);
    backupWalletAction->setToolTip(tr("Backup keys to another location"));
    dumpWalletAction = new QAction(QIcon(":/icons/exportkey"), tr("&Export Keys..."), this);
    dumpWalletAction->setStatusTip(tr("Export Key Pairs to a text file"));
    importWalletAction = new QAction(QIcon(":/icons/importkey"), tr("&Import Keys..."), this);
    importWalletAction->setStatusTip(tr("Import a text file containing a Key Pair"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for encrypting keys"));
    unlockWalletAction = new QAction(QIcon(":/icons/lock_open"), tr("&Unlock Vault..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Lock Vault"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));
    signMessageAction = new QAction(QIcon(":/icons/signmessage"), tr("Sign &message..."), this);
    verifyMessageAction = new QAction(QIcon(":/icons/verifymessage"), tr("&Verify message..."), this);

    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug Window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));

    blockAction = new QAction(QIcon(":/icons/blockexplorer"), tr("&Block Explorer"), this);
    blockAction->setToolTip(tr("Explore the Bean Cash BlockChain"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(dumpWalletAction, SIGNAL(triggered()), this, SLOT(dumpWallet()));
    connect(importWalletAction, SIGNAL(triggered()), this, SLOT(importWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(unlockWalletAction, SIGNAL(triggered()), this, SLOT(unlockWallet()));
    connect(lockWalletAction, SIGNAL(triggered()), this, SLOT(lockWallet()));
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
}

void BitbeanGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Handle Unity's global menu
    if (qgetenv("QT_QPA_PLATFORMTHEME") == "appmenu-qt5")
        appMenuBar = new QMenuBar();
    else
        appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    file->addSeparator();
    file->addAction(dumpWalletAction);
    file->addAction(importWalletAction);
    file->addAction(exportAction);
    file->addAction(signMessageAction);
    file->addAction(verifyMessageAction);
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(encryptWalletAction);
    settings->addAction(changePassphraseAction);
    settings->addAction(unlockWalletAction);
    settings->addAction(lockWalletAction);
    settings->addSeparator();
    settings->addAction(optionsAction);

    // QMenu *tools = appMenuBar->addMenu(tr("&Tools"));
    // tools->addAction(blockAction);


    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitbeanGUI::createToolBars()
{
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(overviewAction);
    toolbar->addAction(sendBeansAction);
    toolbar->addAction(receiveBeansAction);
    toolbar->addAction(historyAction);
    toolbar->addAction(addressBookAction);
    // toolbar->addAction(blockAction);
    toolbar->setStyleSheet("#toolbar {background: transparent}");

    QToolBar *toolbar2 = addToolBar(tr("Actions toolbar"));
    toolbar2->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar2->addAction(exportAction);
}

void BitbeanGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Replace some strings and icons, when using the testnet
        if(clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            qApp->setWindowIcon(QIcon(":icons/bitbean_testnet"));
            setWindowIcon(QIcon(":icons/bitbean_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitbean_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(tr("Bean Cash client") + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
            }
            toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }
        // Create system tray menu (or setup the dock menu) later to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();


        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));

        // Receive and report messages from network/worker thread
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        rpcConsole->setClientModel(clientModel);
        addressBookPage->setOptionsModel(clientModel->getOptionsModel());
        receiveBeansPage->setOptionsModel(clientModel->getOptionsModel());
        blockBrowser->setClientModel(clientModel);
    }
}

void BitbeanGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // Receive and report messages from wallet thread
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveBeansPage->setModel(walletModel->getAddressTableModel());
        sendBeansPage->setModel(walletModel);
        signVerifyMessageDialog->setModel(walletModel);


        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
    }
}

void BitbeanGUI::createTrayIcon()
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip(tr("Bean Cash Core"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    trayIcon->show();

#endif

    notificator = new Notificator(qApp->applicationName(), trayIcon);
}

void BitbeanGUI::createTrayIconMenu()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    // Only for non-Macs, return if trayIcon is unset
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif


    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(backupWalletAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendBeansAction);
    trayIconMenu->addAction(receiveBeansAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(encryptWalletAction);
    trayIconMenu->addAction(changePassphraseAction);
    trayIconMenu->addAction(lockWalletAction);
    trayIconMenu->addAction(unlockWalletAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
    trayIconMenu->addAction(aboutAction);
    trayIconMenu->addSeparator();
    // trayIconMenu->addAction(blockAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

}

#ifndef Q_OS_MAC
void BitbeanGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitbeanGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitbeanGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitbeanGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to the Bean Cash network", "", count));
}

void BitbeanGUI::setNumBlocks(int count, int nTotalBlocks)
{
    QString tooltip;
    QString strStatusBarWarnings = clientModel->getStatusBarWarnings();

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // don't show / hide progress bar and its label if we have no connection to the network
    if (!clientModel || (clientModel->getNumConnections() == 0 && !clientModel->isImporting()))
    {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        return;
    }
    
    // Override progressBarLabel text and hide progress bar if we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
    {
			progressBarLabel->setText(strStatusBarWarnings);
			progressBarLabel->setVisible(true);
			progressBar->setVisible(false);    
    }

    if(count < nTotalBlocks)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (strStatusBarWarnings.isEmpty())
        {
        		progressBarLabel->setText(tr(clientModel->isImporting() ? "Importing blocks..." : "Synchronizing with network..."));
        		progressBarLabel->setVisible(true);
        		progressBar->setFormat(tr("~%n block(s) remaining", "", nRemainingBlocks));
        		progressBar->setMaximum(nTotalBlocks);
        		progressBar->setValue(count);
        		progressBar->setVisible(true);
        }
        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    }
    else
    {
        if (strStatusBarWarnings.isEmpty())
            progressBarLabel->setVisible(false);

        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    // Override progressBarLabel text and hide progress bar, when we have warnings to display
    if (!strStatusBarWarnings.isEmpty())
    {
        progressBarLabel->setText(strStatusBarWarnings);
        progressBarLabel->setVisible(true);
        progressBar->setVisible(false);
    }

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(QDateTime::currentDateTime());
    QString text;

    // Represent time from last generated block in human readable text
    if(secs <= 0)
    {
        // Fully up to date. Leave text empty.
    }
    else if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 5*60 && count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        overviewPage->showOutOfSyncWarning(false);
    }
    else
    {
        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(QIcon(QString(":/movies/spinnerbean-%1").arg(spinnerFrame, 3, 10, QChar('0'))).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame +1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

        overviewPage->showOutOfSyncWarning(true);
    }

    if(!text.isEmpty())
    {
        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitbeanGUI::message(const QString &title, const QString &message, unsigned int style)
{
    QString strTitle = tr("Bean Cash") + " - ";
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    // Check for usage of predefined title
    switch (style) {
        case CClientUIInterface::MSG_ERROR:
            strTitle += tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            strTitle += tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            strTitle += tr("Information");
            break;
        default:
            strTitle += title; // Use supplied title
        }

        // Check for error/warning icon
        if (style & CClientUIInterface::ICON_ERROR) {
            nMBoxIcon = QMessageBox::Critical;
            nNotifyIcon = Notificator::Critical;
        }
        else if (style & CClientUIInterface::ICON_WARNING) {
            nMBoxIcon = QMessageBox::Warning;
            nNotifyIcon = Notificator::Warning;
        }

        // Display message
               if (style & CClientUIInterface::MODAL) {
                   // Check for buttons, use OK as default, if none was supplied
                   QMessageBox::StandardButton buttons;
                   if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
                       buttons = QMessageBox::Ok;
                   // Only show messages after initilization is complete
                   if(!(style & CClientUIInterface::NOSHOWGUI))
                       showNormalIfMinimized();
                   QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons);
                   mBox.exec();
               }
               else
                       notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
           }

void BitbeanGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitbeanGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void BitbeanGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    QString strMessage = tr("In order to process your transaction(s) on the Bean Cash Network, a fee of %1 is required. "
           "This fee is paid to the node that processes your transaction. Select <YES> to continue and send your transaction(s).").arg(
                BitbeanUnits::formatWithUnit(clientModel->getOptionsModel()->getDisplayUnit(), nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitbeanGUI::incomingTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
       if(!walletModel || !clientModel || clientModel->inInitialBlockDownload())
           return;

       TransactionTableModel *ttm = walletModel->getTransactionTableModel();

       QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
       qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                         .data(Qt::EditRole).toULongLong();
       QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
       QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                           .data().toString();

       // On new transaction, make an info balloon
       message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
                tr("Date: %1\n"
                   "Amount: %2\n"
                   "Type: %3\n"
                   "Address: %4\n")
                     .arg(date)
                     .arg(BitbeanUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                     .arg(type)
                     .arg(address), CClientUIInterface::MSG_INFORMATION);
}

void BitbeanGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(overviewPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitbeanGUI::gotoBlockBrowser()
{
    /*
    blockAction->setChecked(true);
    centralWidget->setCurrentWidget(blockBrowser);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    */
    blockBrowser->show();
    blockBrowser->setFocus();
}

void BitbeanGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(transactionsPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
}

void BitbeanGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(addressBookPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
}

void BitbeanGUI::gotoReceiveBeansPage()
{
    receiveBeansAction->setChecked(true);
    centralWidget->setCurrentWidget(receiveBeansPage);

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveBeansPage, SLOT(exportClicked()));
}

void BitbeanGUI::gotoSendBeansPage(QString addr)
{
    sendBeansAction->setChecked(true);
    centralWidget->setCurrentWidget(sendBeansPage);

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);

    if(!addr.isEmpty())
        sendBeansPage->setAddress(addr);
}

void BitbeanGUI::gotoSignMessageTab(QString addr)
{
    // call show() in showTab_SM()
    signVerifyMessageDialog->showTab_SM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void BitbeanGUI::gotoVerifyMessageTab(QString addr)
{
    // call show() in showTab_VM()
    signVerifyMessageDialog->showTab_VM(true);

    if(!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

void BitbeanGUI::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(backupWalletAction);
    menu.addSeparator();
    menu.addAction(signMessageAction);
    menu.addAction(verifyMessageAction);
    menu.addSeparator();
    menu.addAction(encryptWalletAction);
    menu.addAction(changePassphraseAction);
    menu.addAction(lockWalletAction);
    menu.addAction(unlockWalletAction);
    menu.addSeparator();
    menu.addAction(optionsAction);
    menu.addAction(openRPCConsoleAction);
    menu.addAction(blockAction);
    menu.addAction(aboutAction);
    menu.addSeparator();
    menu.addAction(quitAction);
    menu.exec(event->globalPos());
}

void BitbeanGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitbeanGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            if (sendBeansPage->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            gotoSendBeansPage();
        else
            message(tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Bean Cash address or malformed URI parameters."),

                    CClientUIInterface::ICON_WARNING);
    }

    event->acceptProposedAction();
}

void BitbeanGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (sendBeansPage->handleURI(strURI))
    {
        showNormalIfMinimized();
        gotoSendBeansPage();
    }
    else
        message(tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid Bean Cash address or malformed URI parameters."),
                CClientUIInterface::ICON_WARNING);
}

void BitbeanGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void BitbeanGUI::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void BitbeanGUI::backupWallet()
{
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Wallet"), saveDir, tr("Wallet Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            message(tr("Backup Failed"), tr("There was an error trying to save the keypair data to the new location."),
                    CClientUIInterface::MSG_ERROR);
        }
        else
            message(tr("Backup Successful"), tr("Keypair data was successfully saved to the new location."),
                    CClientUIInterface::MSG_INFORMATION);
    }
}

void BitbeanGUI::dumpWallet()
{
   if(!walletModel)
      return;

   WalletModel::UnlockContext ctx(walletModel->requestUnlock());
   if(!ctx.isValid())
   {
       // Unlock vault failed or was cancelled
       return;
   }

#if QT_VERSION < 0x050000
    QString saveDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    QString saveDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Key Pair"), saveDir, tr("KeyPair(*.txt)"));
    if(!filename.isEmpty()) {
        if(!walletModel->dumpWallet(filename)) {
            message(tr("Export Key Pair Failed"),
                         tr("An error happened while trying to save the keys to your location.\n"
                            "Keys were not saved")
                      ,CClientUIInterface::MSG_ERROR);
        }
        else
          message(tr("Export of Key Pair was Successful"),
                       tr("Keys were saved to this file:\n%2")
                       .arg(filename)
                      ,CClientUIInterface::MSG_INFORMATION);
    }
}

void BitbeanGUI::importWallet()
{
   if(!walletModel)
      return;

   WalletModel::UnlockContext ctx(walletModel->requestUnlock());
   if(!ctx.isValid())
   {
       // Unlock vault failed or was cancelled
       return;
   }

#if QT_VERSION < 0x050000
    QString openDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    QString openDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Key Pair"), openDir, tr("KeyPair(*.txt)"));
    if(!filename.isEmpty()) {
        if(!walletModel->importWallet(filename)) {
            message(tr("Import of Key Pair Failed"),
                         tr("An error happened while trying to import the keys.\n"
                            "Some or all keys from:\n %1,\n were not imported into your vault.")
                         .arg(filename)
                      ,CClientUIInterface::MSG_ERROR);
        }
        else
          message(tr("Import of Key Pair was Successful"),
                       tr("All keys from:\n %1,\n were imported into your vault.")
                       .arg(filename)
                      ,CClientUIInterface::MSG_INFORMATION);
    }
}

void BitbeanGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitbeanGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog::Mode mode = sender() == unlockWalletAction ?
              AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
        AskPassphraseDialog dlg(mode, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void BitbeanGUI::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void BitbeanGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitbeanGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitbeanGUI::detectShutdown()
{
    if (ShutdownRequested())
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

void BitbeanGUI::updateWeight()
{
    if (!pwalletMain)
        return;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;

    uint64_t nMinWeight = 0, nMaxWeight = 0;
    pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);
}

void BitbeanGUI::updateStakingIcon()
{
    // Mitigate freezing of inital block downloads, as updateWeight requires mutex lock
    if (nLastBeanStakeSearchInterval && pwalletMain && !IsInitialBlockDownload())
    {
        updateWeight();

        if (nLastBeanStakeSearchInterval && nWeight)
        {
            uint64_t nNetworkWeight = GetPoSKernelPS();
            unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

            QString text;
            if (nEstimateTime < 60)
            {
                text = tr("%n second(s)", "", nEstimateTime);
            }
            else if (nEstimateTime < 60*60)
            {
                text = tr("%n minute(s)", "", nEstimateTime/60);
            }
            else if (nEstimateTime < 24*60*60)
            {
                text = tr("%n hour(s)", "", nEstimateTime/(60*60));
            }
            else
            {
                text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
            }

            labelStakingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelStakingIcon->setToolTip(tr("Sprouting.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
        }
        else
        {
            labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            if (pwalletMain && pwalletMain->IsLocked())
                labelStakingIcon->setToolTip(tr("Not Sprouting because vault is locked"));
            else if (vNodes.empty())
                labelStakingIcon->setToolTip(tr("Not Sprouting because vault is offline"));
            else if (IsInitialBlockDownload())
                labelStakingIcon->setToolTip(tr("Not Sprouting because vault is syncing"));
            else if (!nWeight)
                labelStakingIcon->setToolTip(tr("Not Sprouting because you don't have mature Beans"));
            else
                labelStakingIcon->setToolTip(tr("Not Sprouting"));
        }
    }
}

/*
 * Qt4 Bean Cash GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2013
 * Bean Core www.beancash.org 2021
 */

#include "walletframe.h"
#include "walletview.h"
#include "bitbeangui.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QStackedWidget>

WalletFrame::WalletFrame(BitbeanGUI *_gui) :
    QFrame(_gui),
    gui(_gui)
{
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new QStackedWidget(this);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->addWidget(walletStack);
}

WalletFrame::~WalletFrame()
{
}

void WalletFrame::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
}

bool WalletFrame::addWallet(const QString& name, WalletModel *walletModel)
{
    if (!gui || !clientModel || !walletModel || mapWalletViews.count(name) > 0)
        return false;

    WalletView *walletView = new WalletView(this);
    walletView->setBitbeanGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    walletView->showOutOfSyncWarning(bOutOfSync);

    walletView->gotoOverviewPage(); /* XXX we should go to the currently selected page */
    walletStack->addWidget(walletView);
    mapWalletViews[name] = walletView;

    // Ensure a walletView is able to show the main window
    connect(walletView, SIGNAL(showNormalIfMinimized()), gui, SLOT(showNormalIfMinimized()));

    return true;
}

bool WalletFrame::setCurrentWallet(const QString& name)
{
    if (mapWalletViews.count(name) == 0)
        return false;

    WalletView *walletView = mapWalletViews.value(name);
    walletStack->setCurrentWidget(walletView);
    walletView->setEncryptionStatus();
    return true;
}

void WalletFrame::removeAllWallets()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        walletStack->removeWidget(i.value());
    mapWalletViews.clear();
}

bool WalletFrame::handleURI(const QString &uri)
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (!walletView)
        return false;

    return walletView->handleURI(uri);
}

void WalletFrame::showOutOfSyncWarning(bool fShow)
{
    bOutOfSync = fShow;
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();
}

void WalletFrame::gotoHistoryPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
}

void WalletFrame::gotoAddressBookPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoAddressBookPage();
}

void WalletFrame::gotoReceiveBeansPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoReceiveBeansPage();
}

void WalletFrame::gotoSendBeansPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendBeansPage(addr);
}

void WalletFrame::gotoBlockBrowser()
{
    /*
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoBlockBrowser();
    */
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    walletView->gotoBlockBrowser();
}

void WalletFrame::gotoSignMessageTab(QString addr)
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr)
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->gotoVerifyMessageTab(addr);
}

void WalletFrame::encryptWallet(bool status)
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->encryptWallet(status);
}

void WalletFrame::backupWallet()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->backupWallet();
}

void WalletFrame::importWallet()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->importWallet();
}

void WalletFrame::dumpWallet()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->dumpWallet();
}

void WalletFrame::changePassphrase()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->changePassphrase();
}

void WalletFrame::unlockWallet()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->unlockWallet();
}

void WalletFrame::lockWallet()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->lockWallet();
}

void WalletFrame::setEncryptionStatus()
{
    WalletView *walletView = (WalletView*)walletStack->currentWidget();
    if (walletView)
        walletView->setEncryptionStatus();
}

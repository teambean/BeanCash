/*
 * Qt4 Bean Cash GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2013
 * Bean Core www.beancash.org 2021
 */

#include "walletframe.h"
#include "bitbeangui.h"
#include "walletstack.h"

#include <QHBoxLayout>
#include <QMessageBox>

WalletFrame::WalletFrame(BitbeanGUI *_gui) :
    QFrame(_gui),
    gui(_gui),
    clientModel(0)
{
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new WalletStack(this);
    walletStack->setBitbeanGUI(gui);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->addWidget(walletStack);
}

WalletFrame::~WalletFrame()
{
}

void WalletFrame::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    walletStack->setClientModel(clientModel);
}

bool WalletFrame::addWallet(const QString& name, WalletModel *walletModel)
{
    return walletStack->addWallet(name, walletModel);
}

bool WalletFrame::setCurrentWallet(const QString& name)
{
    // TODO: Check if valid name
    walletStack->setCurrentWallet(name);
    return true;
}

void WalletFrame::removeAllWallets()
{
    walletStack->removeAllWallets();
}

bool WalletFrame::handleURI(const QString &uri)
{
    return walletStack->handleURI(uri);
}

void WalletFrame::showOutOfSyncWarning(bool fShow)
{
    if (!walletStack)
        return;

    walletStack->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage()
{
    walletStack->gotoOverviewPage();
}

void WalletFrame::gotoHistoryPage()
{
    walletStack->gotoHistoryPage();
}

void WalletFrame::gotoAddressBookPage()
{
    walletStack->gotoAddressBookPage();
}

void WalletFrame::gotoReceiveBeansPage()
{
    walletStack->gotoReceiveBeansPage();
}

void WalletFrame::gotoSendBeansPage(QString addr)
{
    walletStack->gotoSendBeansPage(addr);
}

void WalletFrame::gotoSignMessageTab(QString addr)
{
    walletStack->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr)
{
    walletStack->gotoSignMessageTab(addr);
}

void WalletFrame::encryptWallet(bool status)
{
    walletStack->encryptWallet(status);
}

void WalletFrame::backupWallet()
{
    walletStack->backupWallet();
}

void WalletFrame::importWallet()
{
    walletStack->importWallet();
}

void WalletFrame::dumpWallet()
{
    walletStack->dumpWallet();
}

void WalletFrame::changePassphrase()
{
    walletStack->changePassphrase();
}

void WalletFrame::unlockWallet()
{
    walletStack->unlockWallet();
}

void WalletFrame::lockWallet()
{
    walletStack->lockWallet();
}

void WalletFrame::setEncryptionStatus()
{
    walletStack->setEncryptionStatus();
}

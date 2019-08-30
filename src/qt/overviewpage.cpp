#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitbeanunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "bitbeanrpc.h"
#include "init.h"
#include "base58.h"
#include "main.h"
#include "wallet.h"
#include "bitbeanrpc.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QIcon>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QFrame>
#include <sstream>
#include <string>
#include <QFontDatabase>

#define DECORATION_SIZE 64
#define NUM_ITEMS 6

extern CWallet* pwalletMain;
double GetPoSKernelPS();

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitbeanUnits::BitB)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitbeanUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    QFontDatabase::addApplicationFont("::/res/fonts/helvetica");

    QFont font("Helvetica");
    font.setPointSize(11);
    QApplication::setFont(font);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelSpendable->setText(BitbeanUnits::formatWithUnit(unit, balance));
    ui->labelSprouting->setText(BitbeanUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitbeanUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitbeanUnits::formatWithUnit(unit, immatureBalance));
    ui->labelTotal->setText(BitbeanUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BTC")
    // - no longer needed -
    updateDisplayUnit();

    // update Beancash Network Information

    updateNetworkinfo();

    // Beancash Network Information Update Timer (every minute)
    QTimer *timerNetworkStats = new QTimer();
    connect (timerNetworkStats, SIGNAL(timeout()), this, SLOT(updateNetworkinfo()));
            timerNetworkStats->start(60 * 1000);
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, model->getStake(), currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateNetworkinfo()
{
    // StalkHeight
    int stalkHeight = pindexBest->nHeight;

    // Bean Network Weight
    int beanWeight = GetPoSKernelPS();

    // Sprouting Difficulty
    double sproutingDifficulty = GetDifficulty(GetLastBlockIndex(pindexBest, true));

    // Connections
    int otherStalks = this->modelNetworkinfo->getNumConnections();

    // Total Beans
    int64_t Beans = ((pindexBest->nMoneySupply)/100000000);


    QString height = QString::number(stalkHeight);
    QString weight = QString::number(beanWeight);
    QString difficulty = QString::number(sproutingDifficulty, 'f', 6);
    QString connections = QString::number(otherStalks);
    QString totalbeans = QLocale::system().toString((qlonglong)Beans);

    // Display StalkHeight
    if(stalkHeight > heightPrevious)
    {
        ui->stalkBox->setText("<b><font color=\"black\">" + height + "</font></b>");
    }
    else
    {
        ui->stalkBox->setText(height);
    }

    // Display Bean Weight on the Network
    if(beanWeight > weightPrevious)
    {
        ui->weightBox->setText("<b><font color=\"black\">" + weight + "</font></b>");
    }
    else if (beanWeight < weightPrevious)
    {
        ui->weightBox->setText("<b><font color=\"grey\">" + weight + "</font></b>");
    }
    else
    {
        ui->weightBox->setText(weight);
    }

    // Display Sprouting Difficulty
    if(sproutingDifficulty > difficultyPrevious)
    {
        ui->sproutBox->setText("<b><font color=\"black\">" + difficulty + "</font></b>");
    }
    else if(sproutingDifficulty < difficultyPrevious)
    {
        ui->sproutBox->setText("<b><font color=\"grey\">" + difficulty + "</font></b>");
    }
    else
    {
        ui->sproutBox->setText(difficulty);
    }

    // Display Connections
    if(otherStalks > connectionsPrevious)
    {
        ui->connectionBox->setText("<b><font color=\"black\">" + connections + "</font></b>");
    }
    else if(otherStalks < connectionsPrevious)
    {
        ui->connectionBox->setText("<b><font color=\"grey\">" + connections + "</font></b>");
    }
    else
    {
        ui->connectionBox->setText(connections);
    }

    // Display Total Beans
    if(Beans > totalbeansPrevious)
    {
        ui->beanBox->setText("<b><font color=\"black\">" + totalbeans + "</font></b>");
    }
    else if(Beans < totalbeansPrevious)
    {
        ui->beanBox->setText("<b><font color=\"gray\">" + totalbeans + "</font></b>");
    }
    else
    {
        ui->beanBox->setText(totalbeans);
    }


    updatePrevious(stalkHeight, beanWeight, sproutingDifficulty, otherStalks, Beans);

}

void OverviewPage::updatePrevious(int stalkHeight, int beanWeight, double sproutingDifficulty, int otherStalks, double Beans)
{
    heightPrevious = stalkHeight;
    weightPrevious = beanWeight;
    difficultyPrevious = sproutingDifficulty;
    connectionsPrevious = otherStalks;
    totalbeansPrevious = Beans;
}

void OverviewPage::setNetworkinfo(ClientModel *modelNetworkinfo)
{
    updateNetworkinfo();
    this->modelNetworkinfo = modelNetworkinfo;
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

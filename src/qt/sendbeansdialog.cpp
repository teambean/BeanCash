#include "sendbeansdialog.h"
#include "ui_sendbeansdialog.h"

#include "init.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"

#include "bitbeanunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "sendbeansentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"

#include "beancontrol.h"
#include "beancontroldialog.h"

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>

SendBeansDialog::SendBeansDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendBeansDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->lineEditBeanControlChange->setPlaceholderText(tr("Enter a Beancash address (e.g. 2JhbfkAFvXqYkreSgJfrRLS9DepUcxbQci)"));
#endif

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Bean Control
    ui->lineEditBeanControlChange->setFont(GUIUtil::bitbeanAddressFont());
    connect(ui->pushButtonBeanControl, SIGNAL(clicked()), this, SLOT(beanControlButtonClicked()));
    connect(ui->checkBoxBeanControlChange, SIGNAL(stateChanged(int)), this, SLOT(beanControlChangeChecked(int)));
    connect(ui->lineEditBeanControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(beanControlChangeEdited(const QString &)));

    // Bean Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(beanControlClipboardChange()));
    ui->labelBeanControlQuantity->addAction(clipboardQuantityAction);
    ui->labelBeanControlAmount->addAction(clipboardAmountAction);
    ui->labelBeanControlFee->addAction(clipboardFeeAction);
    ui->labelBeanControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelBeanControlBytes->addAction(clipboardBytesAction);
    ui->labelBeanControlPriority->addAction(clipboardPriorityAction);
    ui->labelBeanControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelBeanControlChange->addAction(clipboardChangeAction);

    fNewRecipientAllowed = true;
}

void SendBeansDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBeansEntry *entry = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setModel(model);
        }
    }
    if(model && model->getOptionsModel())
    {
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        // Bean Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(beanControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(beanControlFeaturesChanged(bool)), this, SLOT(beanControlFeatureChanged(bool)));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(beanControlUpdateLabels()));
        ui->frameBeanControl->setVisible(model->getOptionsModel()->getBeanControlFeatures());
        beanControlUpdateLabels();
    }
}

SendBeansDialog::~SendBeansDialog()
{
    delete ui;
}

void SendBeansDialog::on_sendButton_clicked()
{
    QList<SendBeansRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBeansEntry *entry = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    // Format confirmation message
    QStringList formatted;
    foreach(const SendBeansRecipient &rcp, recipients)
    {
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(BitbeanUnits::formatWithUnit(BitbeanUnits::BitB, rcp.amount), Qt::escape(rcp.label), rcp.address));
    }

    fNewRecipientAllowed = false;

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send beans"),
                          tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
          QMessageBox::Yes|QMessageBox::Cancel,
          QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    WalletModel::SendBeansReturn sendstatus;

    if (!model->getOptionsModel() || !model->getOptionsModel()->getBeanControlFeatures())
        sendstatus = model->sendBeans(recipients);
    else
        sendstatus = model->sendBeans(recipients, BeanControlDialog::beanControl);

    switch(sendstatus.status)
    {
    case WalletModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::InvalidAmount:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountExceedsBalance:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("The amount exceeds your balance."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("The total exceeds your balance when the %1 transaction fee is included.").
            arg(BitbeanUnits::formatWithUnit(BitbeanUnits::BitB, sendstatus.fee)),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::DuplicateAddress:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCreationFailed:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("Error: Transaction creation failed!"),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCommitFailed:
        QMessageBox::warning(this, tr("Send Beans"),
            tr("Error: The transaction was rejected. This might happen if some of the beans in your wallet were already spent, such as if you used a copy of wallet.dat and beans were spent in the copy but not marked as spent here."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::Aborted: // User aborted, nothing to do
        break;
    case WalletModel::OK:
        accept();
        BeanControlDialog::beanControl->UnSelectAll();
        beanControlUpdateLabels();
        break;
    }
    fNewRecipientAllowed = true;
}

void SendBeansDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        delete ui->entries->takeAt(0)->widget();
    }
    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

void SendBeansDialog::reject()
{
    clear();
}

void SendBeansDialog::accept()
{
    clear();
}

SendBeansEntry *SendBeansDialog::addEntry()
{
    SendBeansEntry *entry = new SendBeansEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendBeansEntry*)), this, SLOT(removeEntry(SendBeansEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(beanControlUpdateLabels()));

    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendBeansDialog::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBeansEntry *entry = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setRemoveEnabled(enabled);
        }
    }
    setupTabChain(0);
    beanControlUpdateLabels();
}

void SendBeansDialog::removeEntry(SendBeansEntry* entry)
{
    delete entry;
    updateRemoveEnabled();
}

QWidget *SendBeansDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBeansEntry *entry = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sendButton);
    return ui->sendButton;
}

void SendBeansDialog::pasteEntry(const SendBeansRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendBeansEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendBeansEntry *first = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
}

bool SendBeansDialog::handleURI(const QString &uri)
{
    SendBeansRecipient rv;
    // URI has to be valid
    if (GUIUtil::parseBitbeanURI(uri, &rv))
    {
        CBitbeanAddress address(rv.address.toStdString());
        if (!address.IsValid())
            return false;
        pasteEntry(rv);
        return true;
    }

    return false;
}

void SendBeansDialog::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(stake);
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    if(!model || !model->getOptionsModel())
        return;

    int unit = model->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(BitbeanUnits::formatWithUnit(unit, balance));
}

void SendBeansDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelBalance->setText(BitbeanUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}

// Bean Control: copy label "Quantity" to clipboard
void SendBeansDialog::beanControlClipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelBeanControlQuantity->text());
}

// Bean Control: copy label "Amount" to clipboard
void SendBeansDialog::beanControlClipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelBeanControlAmount->text().left(ui->labelBeanControlAmount->text().indexOf(" ")));
}

// Bean Control: copy label "Fee" to clipboard
void SendBeansDialog::beanControlClipboardFee()
{
    QApplication::clipboard()->setText(ui->labelBeanControlFee->text().left(ui->labelBeanControlFee->text().indexOf(" ")));
}

// Bean Control: copy label "After fee" to clipboard
void SendBeansDialog::beanControlClipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelBeanControlAfterFee->text().left(ui->labelBeanControlAfterFee->text().indexOf(" ")));
}

// Bean Control: copy label "Bytes" to clipboard
void SendBeansDialog::beanControlClipboardBytes()
{
    QApplication::clipboard()->setText(ui->labelBeanControlBytes->text());
}

// Bean Control: copy label "Priority" to clipboard
void SendBeansDialog::beanControlClipboardPriority()
{
    QApplication::clipboard()->setText(ui->labelBeanControlPriority->text());
}

// Bean Control: copy label "Low output" to clipboard
void SendBeansDialog::beanControlClipboardLowOutput()
{
    QApplication::clipboard()->setText(ui->labelBeanControlLowOutput->text());
}

// Bean Control: copy label "Change" to clipboard
void SendBeansDialog::beanControlClipboardChange()
{
    QApplication::clipboard()->setText(ui->labelBeanControlChange->text().left(ui->labelBeanControlChange->text().indexOf(" ")));
}

// Bean Control: settings menu - bean control enabled/disabled by user
void SendBeansDialog::beanControlFeatureChanged(bool checked)
{
    ui->frameBeanControl->setVisible(checked);

    if (!checked && model) // bean control features disabled
        BeanControlDialog::beanControl->SetNull();
}

// Bean Control: button inputs -> show actual bean control dialog
void SendBeansDialog::beanControlButtonClicked()
{
    BeanControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    beanControlUpdateLabels();
}

// Bean Control: checkbox custom change address
void SendBeansDialog::beanControlChangeChecked(int state)
{
    if (model)
    {
        if (state == Qt::Checked)
            BeanControlDialog::beanControl->destChange = CBitbeanAddress(ui->lineEditBeanControlChange->text().toStdString()).Get();
        else
            BeanControlDialog::beanControl->destChange = CNoDestination();
    }

    ui->lineEditBeanControlChange->setEnabled((state == Qt::Checked));
    ui->labelBeanControlChangeLabel->setEnabled((state == Qt::Checked));
}

// Bean Control: custom change address changed
void SendBeansDialog::beanControlChangeEdited(const QString & text)
{
    if (model)
    {
        BeanControlDialog::beanControl->destChange = CBitbeanAddress(text.toStdString()).Get();

        // label for the change address
        ui->labelBeanControlChangeLabel->setStyleSheet("QLabel{color:Test;}");
        if (text.isEmpty())
            ui->labelBeanControlChangeLabel->setText("");
        else if (!CBitbeanAddress(text.toStdString()).IsValid())
        {
            ui->labelBeanControlChangeLabel->setStyleSheet("QLabel{color:red;}");
            ui->labelBeanControlChangeLabel->setText(tr("WARNING: Invalid Beancash address"));
        }
        else
        {
            QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
            if (!associatedLabel.isEmpty())
                ui->labelBeanControlChangeLabel->setText(associatedLabel);
            else
            {
                CPubKey pubkey;
                CKeyID keyid;
                CBitbeanAddress(text.toStdString()).GetKeyID(keyid);
                if (model->getPubKey(keyid, pubkey))
                    ui->labelBeanControlChangeLabel->setText(tr("(no label)"));
                else
                {
                    ui->labelBeanControlChangeLabel->setStyleSheet("QLabel{color:red;}");
                    ui->labelBeanControlChangeLabel->setText(tr("WARNING: unknown change address"));
                }
            }
        }
    }
}

// Bean Control: update labels
void SendBeansDialog::beanControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getBeanControlFeatures())
        return;

    // set pay amounts
    BeanControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendBeansEntry *entry = qobject_cast<SendBeansEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            BeanControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (BeanControlDialog::beanControl->HasSelected())
    {
        // actual bean control calculation
        BeanControlDialog::updateLabels(model, this);

        // show bean control stats
        ui->labelBeanControlAutomaticallySelected->hide();
        ui->widgetBeanControl->show();
    }
    else
    {
        // hide bean control stats
        ui->labelBeanControlAutomaticallySelected->show();
        ui->widgetBeanControl->hide();
        ui->labelBeanControlInsuffFunds->hide();
    }
}

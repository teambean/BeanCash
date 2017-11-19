#ifndef SENDbeanSDIALOG_H
#define SENDbeanSDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
    class SendBeansDialog;
}
class WalletModel;
class SendBeansEntry;
class SendBeansRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitbeans */
class SendBeansDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendBeansDialog(QWidget *parent = 0);
    ~SendBeansDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void pasteEntry(const SendBeansRecipient &rv);
    bool handleURI(const QString &uri);

public slots:
    void clear();
    void reject();
    void accept();
    SendBeansEntry *addEntry();
    void updateRemoveEnabled();
    void setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance);

private:
    Ui::SendBeansDialog *ui;
    WalletModel *model;
    bool fNewRecipientAllowed;

private slots:
    void on_sendButton_clicked();
    void removeEntry(SendBeansEntry* entry);
    void updateDisplayUnit();
    void beanControlFeatureChanged(bool);
    void beanControlButtonClicked();
    void beanControlChangeChecked(int);
    void beanControlChangeEdited(const QString &);
    void beanControlUpdateLabels();
    void beanControlClipboardQuantity();
    void beanControlClipboardAmount();
    void beanControlClipboardFee();
    void beanControlClipboardAfterFee();
    void beanControlClipboardBytes();
    void beanControlClipboardPriority();
    void beanControlClipboardLowOutput();
    void beanControlClipboardChange();
};

#endif // SENDbeanSDIALOG_H

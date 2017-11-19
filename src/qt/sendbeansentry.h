#ifndef SENDbeanSENTRY_H
#define SENDbeanSENTRY_H

#include <QFrame>

namespace Ui {
    class SendBeansEntry;
}
class WalletModel;
class SendBeansRecipient;

/** A single entry in the dialog for sending bitbeans. */
class SendBeansEntry : public QFrame
{
    Q_OBJECT

public:
    explicit SendBeansEntry(QWidget *parent = 0);
    ~SendBeansEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendBeansRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendBeansRecipient &value);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void setRemoveEnabled(bool enabled);
    void clear();

signals:
    void removeEntry(SendBeansEntry *entry);
    void payAmountChanged();

private slots:
    void on_deleteButton_clicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Ui::SendBeansEntry *ui;
    WalletModel *model;
};

#endif // SENDbeanSENTRY_H

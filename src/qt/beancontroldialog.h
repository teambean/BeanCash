#ifndef BEANCONTROLDIALOG_H
#define BEANCONTROLDIALOG_H

#include <QAbstractButton>
#include <QAction>
#include <QDialog>
#include <QList>
#include <QMenu>
#include <QPoint>
#include <QString>
#include <QTreeWidgetItem>

namespace Ui {
    class BeanControlDialog;
}
class WalletModel;
class CBeanControl;

class BeanControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BeanControlDialog(QWidget *parent = 0);
    ~BeanControlDialog();

    void setModel(WalletModel *model);

    // static because also called from sendbeansdialog
    static void updateLabels(WalletModel*, QDialog*);
    static QString getPriorityLabel(double);

    static QList<qint64> payAmounts;
    static CBeanControl *beanControl;

private:
    Ui::BeanControlDialog *ui;
    WalletModel *model;
    int sortColumn;
    Qt::SortOrder sortOrder;

    QMenu *contextMenu;
    QTreeWidgetItem *contextMenuItem;
    QAction *copyTransactionHashAction;
    //QAction *lockAction;
    //QAction *unlockAction;

    QString strPad(QString, int, QString);
    void sortView(int, Qt::SortOrder);
    void updateView();

    enum
    {
        COLUMN_CHECKBOX,
        COLUMN_AMOUNT,
        COLUMN_LABEL,
        COLUMN_ADDRESS,
        COLUMN_DATE,
        COLUMN_CONFIRMATIONS,
        COLUMN_PRIORITY,
        COLUMN_TXHASH,
        COLUMN_VOUT_INDEX,
        COLUMN_AMOUNT_INT64,
        COLUMN_PRIORITY_INT64
    };

private slots:
    void showMenu(const QPoint &);
    void copyAmount();
    void copyLabel();
    void copyAddress();
    void copyTransactionHash();
    //void lockBean();
    //void unlockBean();
    void clipboardQuantity();
    void clipboardAmount();
    void clipboardFee();
    void clipboardAfterFee();
    void clipboardBytes();
    void clipboardPriority();
    void clipboardLowOutput();
    void clipboardChange();
    void radioTreeMode(bool);
    void radioListMode(bool);
    void viewItemChanged(QTreeWidgetItem*, int);
    void headerSectionClicked(int);
    void buttonBoxClicked(QAbstractButton*);
    void buttonSelectAllClicked();
    //void updateLabelLocked();
};

#endif // BEANCONTROLDIALOG_H

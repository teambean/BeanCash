#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "clientmodel.h"

// Copyright year (2015-this)
// Todo: update this to change copyright comments in the source
const int ABOUTDIALOG_COPYRIGHT_YEAR = 2022;

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
        // Set current copyright year
        ui->copyrightLabel->setText(tr("Copyright ") + tr("2015-%1 www.beancash.org").arg(ABOUTDIALOG_COPYRIGHT_YEAR));
}

void AboutDialog::setModel(ClientModel *model)
{
    if(model)
    {
        ui->versionLabel->setText(model->formatFullVersion());
    }
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_closeButton_clicked()
{
    close();
}

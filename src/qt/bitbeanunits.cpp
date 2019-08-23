#include "bitbeanunits.h"

#include <QStringList>

BitbeanUnits::BitbeanUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitbeanUnits::Unit> BitbeanUnits::availableUnits()
{
    QList<BitbeanUnits::Unit> unitlist;
    unitlist.append(Sprout);
    unitlist.append(BitB);
    unitlist.append(mBitB);
    unitlist.append(uBitB);
    unitlist.append(nokat);
    unitlist.append(Bean);
    return unitlist;
}

bool BitbeanUnits::valid(int unit)
{
    switch(unit)
    {
    case Sprout:
    case BitB:
    case mBitB:
    case uBitB:
    case nokat:
    case Bean:
        return true;
    default:
        return false;
    }
}

QString BitbeanUnits::name(int unit)
{
    switch(unit)
    {
    case Sprout: return QString("SPROUT");
    case BitB: return QString("BITB");
    case mBitB: return QString("mBITB");
    case uBitB: return QString::fromUtf8("μBITB");
    case nokat: return QString("ADZUKI");
    case Bean: return QString("BEAN");
    default: return QString("???");
    }
}

QString BitbeanUnits::description(int unit)
{
    switch(unit)
    {
    case Sprout: return QString("SPROUT (1,000 BITB)");
    case BitB: return QString("BITB");
    case mBitB: return QString("Milli-BITB (1 / 1,000)");
    case uBitB: return QString("Micro-BITB (1 / 1,000,000)");
    case nokat: return QString("Adzuki (1 / 100,000,000)");
    case Bean: return QString("New Bean Class (Reserved Project Aurora)");
    default: return QString("???");
    }
}

qint64 BitbeanUnits::factor(int unit)
{
    switch(unit)
    {
    case Sprout: return 100000000000;
    case BitB:  return 100000000;
    case mBitB: return 100000;
    case uBitB: return 100;
    case nokat: return 1;
    case Bean: return 100000000;  // New Class of Bean, Value Not Defined Yet
    default:   return 100000000;
    }
}

int BitbeanUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case Sprout: return 5;
    case BitB: return 8; // 21,000,000 (# digits, without commas)
    case mBitB: return 11; // 21,000,000,000
    case uBitB: return 14; // 21,000,000,000,000
    case nokat: return 16;
    case Bean: return 8;
    default: return 0;
    }
}

int BitbeanUnits::decimals(int unit)
{
    switch(unit)
    {
    case Sprout: return 11;
    case BitB: return 8;
    case mBitB: return 5;
    case uBitB: return 2;
    case nokat: return 0;
    case Bean: return 8;
    default: return 0;
    }
}

QString BitbeanUnits::format(int unit, qint64 n, bool fPlus)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 bean = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / bean;
    qint64 remainder = n_abs % bean;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess zeros after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

QString BitbeanUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitbeanUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);
    QStringList parts = value.split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

int BitbeanUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitbeanUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

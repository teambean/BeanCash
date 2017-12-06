#include "uritests.h"
#include "../guiutil.h"
#include "../walletmodel.h"

#include <QUrl>

/*
struct SendBeansRecipient
{
    QString address;
    QString label;
    qint64 amount;
};
*/

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?req-dontexist="));
    QVERIFY(!GUIUtil::parsebeancashURI(uri, &rv));

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parsebeancashURI(uri, &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.label == QString("Team Bean Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?amount=1.001"));
    QVERIFY(GUIUtil::parsebeancashURI(uri, &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?amount=100&label=Team Bean Example"));
    QVERIFY(GUIUtil::parsebeancashURI(uri, &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Team Bean Address"));

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?message=Team Bean Address"));
    QVERIFY(GUIUtil::parsebeancashURI(uri, &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parsebeancashURI("beancash://2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?message=Team Bean Example Address", &rv));
    QVERIFY(rv.address == QString("2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw"));
    QVERIFY(rv.label == QString());

    // We currently don't implement the message parameter (ok, yea, we break spec...)
    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?req-message=Team Bean Example Address"));
    QVERIFY(!GUIUtil::parseBibeanURI(uri, &rv));

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?amount=1,000&label=Team Bean Example"));
    QVERIFY(!GUIUtil::parseBitbeanURI(uri, &rv));

    uri.setUrl(QString("beancash:2WcY6PfocpVHi23rGceWCn4F4b4UrWSPhw?amount=1,000.0&label=Team Bean Example"));
    QVERIFY(!GUIUtil::parseBitbeanURI(uri, &rv));
}

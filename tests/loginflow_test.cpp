#include "loginflow.h"
#include <QSignalSpy>
#include <QtTest>
#include <deque>
class LoginTest final : public QObject {
  Q_OBJECT
  struct Pending {
    QString path;
    QJsonObject args;
    LoginFlow::Callback done;
  };
  std::deque<Pending> pending;
  LoginFlow::Request requester() {
    return [this](QString path, QJsonObject args, QString,
                  LoginFlow::Callback done) {
      pending.push_back({path, args, std::move(done)});
    };
  }
  void respond(QJsonObject data, QString error = {}) {
    auto call = std::move(pending.front());
    pending.pop_front();
    call.done(data, error);
  }
private slots:
  void init() { pending.clear(); }
  void confirmationPersistsBeforeUi() {
    bool saved = false;
    LoginFlow flow(
        requester(),
        [&](QJsonObject p) {
          QCOMPARE(p.value("userId").toInt(), 42);
          saved = true;
        },
        nullptr, 10);
    QSignalSpy ready(&flow, &LoginFlow::accountReady);
    connect(&flow, &LoginFlow::accountReady, this, [&] { QVERIFY(saved); });
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 802}});
    QCOMPARE(flow.phase(), QString("scanned"));
    QTRY_VERIFY(!pending.empty());
    respond({{"code", "803"}});
    QCOMPARE(flow.phase(), QString("authorizing"));
    QVERIFY(!pending.empty());
    QCOMPARE(pending.front().path, QString("/api/nuser/account/get"));
    respond(
        {{"code", 200},
         {"profile", QJsonObject{{"userId", 42}, {"nickname", "fixture"}}}});
    QCOMPARE(ready.count(), 1);
    QCOMPARE(flow.phase(), QString("authenticated"));
    QTest::qWait(30);
    QVERIFY(pending.empty());
  }
  void staleQrAndCancelledPollAreIgnored() {
    LoginFlow flow(requester(), [](QJsonObject) {}, nullptr, 10);
    QSignalSpy qr(&flow, &LoginFlow::challenge),
        ready(&flow, &LoginFlow::accountReady);
    flow.start();
    auto old = std::move(pending.front());
    pending.pop_front();
    flow.start();
    respond({{"code", 200}, {"unikey", "new"}});
    old.done({{"code", 200}, {"unikey", "old"}}, {});
    QVERIFY(qr.last().first().toString().contains("codekey=new"));
    QTRY_VERIFY(!pending.empty());
    flow.cancel();
    respond({{"code", 803}});
    QVERIFY(pending.empty());
    QCOMPARE(ready.count(), 0);
  }
  void expirationAndNetworkRetry() {
    LoginFlow flow(requester(), [](QJsonObject) {}, nullptr, 10);
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({}, "temporary failure");
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 800}});
    QCOMPARE(flow.phase(), QString("expired"));
    QTest::qWait(40);
    QVERIFY(pending.empty());
  }
  void transientServiceStatusRetainsQr() {
    bool saved = false;
    LoginFlow flow(
        requester(), [&](QJsonObject) { saved = true; }, nullptr, 10);
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 802}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 502}});
    QTRY_VERIFY(!pending.empty());
    QCOMPARE(pending.front().args.value("key").toString(), QString("fixture"));
    respond({{"code", 803}});
    respond({{"code", 200}, {"profile", QJsonObject{{"userId", 42}}}});
    QVERIFY(saved);
  }
  void delayedProfileUsesExistingGrant() {
    bool saved = false;
    LoginFlow flow(
        requester(), [&](QJsonObject) { saved = true; }, nullptr, 10);
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 803}});
    respond({{"code", 200}, {"profile", QJsonValue::Null}});
    QTRY_VERIFY_WITH_TIMEOUT(!pending.empty(), 2000);
    QCOMPARE(pending.front().path, QString("/api/nuser/account/get"));
    respond({{"code", 200}, {"profile", QJsonObject{{"userId", 42}}}});
    QVERIFY(saved);
  }
  void logoutInvalidatesVerifiedSession() {
    LoginFlow flow(requester(), [](QJsonObject) {}, nullptr, 10);
    flow.restore();
    respond({{"code", 200}, {"profile", QJsonObject{{"userId", 42}}}});
    flow.forgetAccount();
    flow.start();
    QCOMPARE(pending.front().path, QString("/api/login/qrcode/unikey"));
  }
  void profileFailureCanRetryWithoutAnotherQr() {
    LoginFlow flow(requester(), [](QJsonObject) {}, nullptr, 10);
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 803}});
    for (int attempt = 0; attempt < 3; ++attempt) {
      QTRY_VERIFY_WITH_TIMEOUT(!pending.empty(), 2000);
      respond({}, "temporary network failure");
    }
    QCOMPARE(flow.phase(), QString("account-error"));
    flow.retryAccount();
    QCOMPARE(pending.front().path, QString("/api/nuser/account/get"));
    respond({{"code", 200}, {"profile", QJsonObject{{"userId", 42}}}});
    QCOMPARE(flow.phase(), QString("authenticated"));
  }
  void rejectedSessionCanGetNewQr() {
    LoginFlow flow(requester(), [](QJsonObject) {}, nullptr, 10);
    flow.start();
    respond({{"code", 200}, {"unikey", "fixture"}});
    QTRY_VERIFY(!pending.empty());
    respond({{"code", 803}});
    for (int attempt = 0; attempt < 3; ++attempt) {
      QTRY_VERIFY_WITH_TIMEOUT(!pending.empty(), 2000);
      respond({{"code", 200}, {"profile", QJsonValue::Null}});
    }
    QCOMPARE(flow.phase(), QString("expired"));
    flow.start();
    QCOMPARE(pending.front().path, QString("/api/login/qrcode/unikey"));
  }
};
QTEST_GUILESS_MAIN(LoginTest)
#include "loginflow_test.moc"

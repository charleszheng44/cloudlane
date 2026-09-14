#include "loginflow.h"
#include <QLoggingCategory>
#include <QUrl>
#include <QUrlQuery>
Q_LOGGING_CATEGORY(loginLog, "yunjian.login")
LoginFlow::LoginFlow(Request request, Persist persist, QObject *parent,
                     int pollInterval)
    : QObject(parent), request(std::move(request)), persist(std::move(persist)),
      pollInterval(pollInterval) {
  timer.setParent(this);
  timer.setInterval(pollInterval);
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &LoginFlow::poll);
}
void LoginFlow::transition(QString phase, QString message) {
  if (state != phase)
    qCInfo(loginLog).noquote() << phase;
  state = phase;
  emit changed(phase, message);
}
void LoginFlow::start() {
  cancel();
  const auto epoch = generation;
  if (authorized) {
    verifyAccount(true, epoch);
    return;
  }
  transition("loading", "Getting a QR code…");
  emit challenge({});
  request("/api/login/qrcode/unikey", {{"type", 3}}, "eapi",
          [this, epoch](QJsonObject data, QString error) {
            if (epoch != generation)
              return;
            key = data.value("unikey").toString();
            if (key.isEmpty())
              key = data.value("data").toObject().value("unikey").toString();
            if (!error.isEmpty() || key.isEmpty()) {
              transition("error",
                         error.isEmpty() ? "Could not get a QR code. Please refresh." : error);
              return;
            }
            QUrl url("https://music.163.com/login");
            QUrlQuery query;
            query.addQueryItem("codekey", key);
            url.setQuery(query);
            emit challenge(url.toString());
            transition("waiting", "Scan with the NetEase Cloud Music mobile app and confirm sign-in.");
            timer.start();
          });
}
void LoginFlow::cancel() {
  ++generation;
  timer.stop();
  inFlight = false;
  key.clear();
  statusRetries = 0;
  timer.setInterval(pollInterval);
  transition("idle", {});
}
void LoginFlow::forgetAccount() {
  cancel();
  authorized = false;
}
void LoginFlow::restore() { verifyAccount(false, generation); }
void LoginFlow::retryAccount() {
  timer.stop();
  inFlight = false;
  verifyAccount(true, ++generation);
}
void LoginFlow::poll() {
  if (inFlight || key.isEmpty())
    return;
  inFlight = true;
  const auto epoch = generation;
  request("/api/login/qrcode/client/login", {{"key", key}, {"type", 3}}, "eapi",
          [this, epoch](QJsonObject data, QString error) {
            if (epoch != generation)
              return;
            inFlight = false;
            const int code = data.value("code").toVariant().toInt();
            if (!error.isEmpty()) {
              transition("waiting", error + " · Retrying");
              timer.start();
              return;
            }
            if (code == 803) {
              authorized = true;
              key.clear();
              verifyAccount(true, epoch);
              return;
            }
            if (code == 800) {
              transition("expired", "QR code expired. Please refresh.");
              return;
            }
            if (code == 802)
              transition("scanned", "Scanned. Waiting for confirmation on your phone…");
            else if (code == 801)
              transition("waiting", "Scan with the NetEase Cloud Music mobile app and confirm sign-in.");
            else {
              qCWarning(loginLog) << "Unexpected QR status" << code;
              if (++statusRetries <= 5) {
                transition(
                    "waiting",
                    QString("Sign-in is not confirmed yet (code %1). Retrying…").arg(code));
                timer.start(pollInterval * 2);
              } else {
                transition(
                    "error",
                    QString("Sign-in returned code %1. Please refresh and try again.").arg(code));
              }
              return;
            }
            statusRetries = 0;
            timer.setInterval(pollInterval);
            timer.start();
          });
}
void LoginFlow::verifyAccount(bool afterLogin, quint64 epoch, int attempt) {
  if (afterLogin)
    transition("authorizing", "Approved. Syncing your account…");
  request(
      "/api/nuser/account/get", {}, "weapi",
      [this, afterLogin, epoch, attempt](QJsonObject data, QString error) {
        if (epoch != generation)
          return;
        const auto profile = data.value("profile").toObject();
        if (error.isEmpty() && data.value("code").toVariant().toInt() == 200 &&
            !profile.value("userId").toVariant().toString().isEmpty()) {
          // Persist before notifying the UI: a consumed QR grant must survive a
          // closed/minimized window or a restart immediately after
          // authorization.
          persist(profile);
          authorized = true;
          emit challenge({});
          transition("authenticated", "Signed in");
          emit accountReady(profile, afterLogin);
          return;
        }
        if (afterLogin && attempt < 2) {
          QTimer::singleShot(1200, this, [this, afterLogin, epoch, attempt] {
            if (epoch == generation)
              verifyAccount(afterLogin, epoch, attempt + 1);
          });
          return;
        }
        if (error.isEmpty() &&
            (data.value("code").toVariant().toInt() == 200 ||
             data.value("code").toVariant().toInt() == 301)) {
          authorized = false;
          if (afterLogin)
            transition("expired", "Your session expired. Please refresh the QR code.");
        } else if (afterLogin)
          transition("account-error",
                     error.isEmpty()
                         ? "Sign-in approved, but account sync is incomplete. Please retry sync."
                         : error);
      });
}

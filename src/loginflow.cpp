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
  transition("loading", "正在获取二维码…");
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
                         error.isEmpty() ? "二维码获取失败，请刷新" : error);
              return;
            }
            QUrl url("https://music.163.com/login");
            QUrlQuery query;
            query.addQueryItem("codekey", key);
            url.setQuery(query);
            emit challenge(url.toString());
            transition("waiting", "使用网易云音乐手机 App 扫码确认");
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
              transition("waiting", error + " · 正在重试");
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
              transition("expired", "二维码已过期，请刷新");
              return;
            }
            if (code == 802)
              transition("scanned", "已扫码，等待手机确认…");
            else if (code == 801)
              transition("waiting", "使用网易云音乐手机 App 扫码确认");
            else {
              qCWarning(loginLog) << "Unexpected QR status" << code;
              if (++statusRetries <= 5) {
                transition(
                    "waiting",
                    QString("登录服务暂未完成确认（%1），正在重试…").arg(code));
                timer.start(pollInterval * 2);
              } else {
                transition(
                    "error",
                    QString("登录服务返回状态 %1，请刷新重试").arg(code));
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
    transition("authorizing", "已授权，正在同步账户…");
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
          transition("authenticated", "已登录");
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
            transition("expired", "登录会话已失效，请刷新二维码");
        } else if (afterLogin)
          transition("account-error",
                     error.isEmpty()
                         ? "授权已确认，账户同步暂未完成；请重试同步"
                         : error);
      });
}

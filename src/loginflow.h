#pragma once
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <functional>

// Network login is driven by a real event-loop timer, independent of QML's
// animation clock and whether the window is occluded or minimized.
class LoginFlow final : public QObject {
  Q_OBJECT
public:
  using Callback = std::function<void(QJsonObject, QString)>;
  using Request = std::function<void(QString, QJsonObject, QString, Callback)>;
  using Persist = std::function<void(QJsonObject)>;
  explicit LoginFlow(Request request, Persist persist,
                     QObject *parent = nullptr, int pollInterval = 2000);
  void start();
  void cancel();
  void forgetAccount();
  void restore();
  void retryAccount();
  QString phase() const { return state; }
signals:
  void changed(QString phase, QString message);
  void challenge(QString url);
  void accountReady(QJsonObject profile, bool afterLogin);

private:
  void poll();
  void verifyAccount(bool afterLogin, quint64 generation, int attempt = 0);
  void transition(QString phase, QString message);
  Request request;
  Persist persist;
  QTimer timer;
  QString key, state = "idle";
  quint64 generation = 0;
  bool inFlight = false, authorized = false;
  int statusRetries = 0;
  int pollInterval;
};

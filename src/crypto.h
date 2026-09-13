#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
namespace TransportCrypto {
QByteArray randomBytes(int size);
QByteArray form(const QJsonObject &data);
QByteArray aes(const QByteArray &data, const QByteArray &key, bool encrypt,
               const QByteArray &iv = {});
QJsonObject weapi(const QByteArray &json, QByteArray secret = {});
QJsonObject eapi(const QString &path, const QByteArray &json);
QByteArray decodeResponse(const QByteArray &data);
QByteArray keySignature(const QString &timestamp, const QString &nonce);
QJsonObject decodePublicKey(const QString &encrypted);
QJsonObject xeapi(const QString &path, QJsonObject data,
                  const QJsonObject &keyState,
                  const QByteArray &sessionKey = {},
                  const QString &sessionId = {});
} // namespace TransportCrypto

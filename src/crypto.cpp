// Consumer transport derived from API Enhanced, MIT (see
// third_party/API-Enhanced-LICENSE). Pinned reference:
// a8c781fd64faab17fedfd46e0615a2609307f163, util/crypto.js.
#include "crypto.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <algorithm>
#include <memory>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <stdexcept>
#include <zlib.h>
namespace {
const QByteArray ekey("e82ckenh8dichen8");
const QByteArray xkey = QByteArray::fromHex(
    "ab1d5a430f6bb04a3f01e81ddd72bd916d5ce591248ac128714806d7f8fb1b84");
const QByteArray signkey("mUHCwVNWJbunMqAHf5MImuirT6plvs6VSFW62MGHstFQxhBGdEoIh"
                         "LItH3djc4+FB/OKty3+lL2rGeoFBpVe5g==");
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
QByteArray hmac(const QByteArray &key, const QByteArray &data) {
  QByteArray out(32, 0);
  unsigned int n = 0;
  require(HMAC(EVP_sha256(), key.data(), key.size(),
               reinterpret_cast<const unsigned char *>(data.data()),
               data.size(), reinterpret_cast<unsigned char *>(out.data()),
               &n) != nullptr,
          "HMAC failed");
  out.resize(n);
  return out;
}
QJsonObject object(const QByteArray &bytes) {
  QJsonParseError error;
  auto doc = QJsonDocument::fromJson(bytes, &error);
  require(error.error == QJsonParseError::NoError && doc.isObject(),
          "Invalid encrypted JSON");
  return doc.object();
}
QByteArray encoded(const QString &value) {
  auto result = QUrl::toPercentEncoding(value, "*-._");
  result.replace("%20", "+");
  result.replace("~", "%7E");
  return result;
}
} // namespace
namespace TransportCrypto {
QByteArray randomBytes(int size) {
  require(size > 0 && size < 1024, "Invalid random size");
  QByteArray b(size, 0);
  require(RAND_bytes(reinterpret_cast<unsigned char *>(b.data()), size) == 1,
          "Random generation failed");
  return b;
}
QByteArray form(const QJsonObject &data) {
  QByteArray out;
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (!out.isEmpty())
      out += '&';
    QString value;
    if (it->isString())
      value = it->toString();
    else if (it->isBool())
      value = it->toBool() ? "true" : "false";
    else if (it->isNull())
      value = "null";
    else if (it->isObject())
      value = QString::fromUtf8(
          QJsonDocument(it->toObject()).toJson(QJsonDocument::Compact));
    else if (it->isArray())
      value = QString::fromUtf8(
          QJsonDocument(it->toArray()).toJson(QJsonDocument::Compact));
    else
      value = QString::number(it->toDouble(), 'g', 16);
    out += encoded(it.key()) + '=' + encoded(value);
  }
  return out;
}
QByteArray aes(const QByteArray &data, const QByteArray &key, bool encrypt,
               const QByteArray &iv) {
  require(key.size() == 16 || key.size() == 32, "Invalid AES key");
  require(iv.isEmpty() || iv.size() == 16, "Invalid IV");
  const EVP_CIPHER *cipher =
      iv.isEmpty() ? (key.size() == 16 ? EVP_aes_128_ecb() : EVP_aes_256_ecb())
                   : (key.size() == 16 ? EVP_aes_128_cbc() : EVP_aes_256_cbc());
  std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(
      EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  require(bool(ctx), "Cipher allocation failed");
  require(EVP_CipherInit_ex(
              ctx.get(), cipher, nullptr,
              reinterpret_cast<const unsigned char *>(key.data()),
              iv.isEmpty() ? nullptr
                           : reinterpret_cast<const unsigned char *>(iv.data()),
              encrypt) == 1,
          "Cipher init failed");
  QByteArray out(data.size() + 32, 0);
  int written = 0, tail = 0;
  require(EVP_CipherUpdate(
              ctx.get(), reinterpret_cast<unsigned char *>(out.data()),
              &written, reinterpret_cast<const unsigned char *>(data.data()),
              data.size()) == 1,
          "Cipher update failed");
  require(EVP_CipherFinal_ex(
              ctx.get(),
              reinterpret_cast<unsigned char *>(out.data() + written),
              &tail) == 1,
          "Cipher padding failed");
  out.resize(written + tail);
  return out;
}
QJsonObject weapi(const QByteArray &json, QByteArray secret) {
  if (secret.isEmpty()) {
    const QByteArray alphabet(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    auto rnd = randomBytes(16);
    for (unsigned char c : rnd)
      secret += alphabet[c % 62];
  }
  require(secret.size() == 16, "Invalid WEAPI key");
  auto encrypted =
      aes(aes(json, "0CoJUm6Qyw8W8jud", true, "0102030405060708").toBase64(),
          secret, true, "0102030405060708")
          .toBase64();
  const char pem[] =
      "-----BEGIN PUBLIC "
      "KEY-----"
      "\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDgtQn2JZ34ZC28NWYpAUd98iZ37BUrX/"
      "aKzmFbt7clFSs6sXqHauqKWqdtLkF2KexO40H1YTX8z2lSgBBOAxLsvaklV8k4cBFK9snQXE"
      "9/"
      "DDaFt6Rr7iVZMldczhC0JNgTz+"
      "SHXT6CBHuX3e9SdB1Ua44oncaTWz7OBGLbCiK45wIDAQAB\n-----END PUBLIC "
      "KEY-----";
  std::unique_ptr<BIO, decltype(&BIO_free)> bio(BIO_new_mem_buf(pem, -1),
                                                BIO_free);
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
      PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr), EVP_PKEY_free);
  require(bool(key), "RSA key invalid");
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
      EVP_PKEY_CTX_new(key.get(), nullptr), EVP_PKEY_CTX_free);
  require(ctx && EVP_PKEY_encrypt_init(ctx.get()) > 0 &&
              EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_NO_PADDING) > 0,
          "RSA init failed");
  QByteArray input(EVP_PKEY_get_size(key.get()), 0);
  std::reverse(secret.begin(), secret.end());
  input.replace(input.size() - 16, 16, secret);
  QByteArray output(input.size(), 0);
  size_t n = output.size();
  require(EVP_PKEY_encrypt(
              ctx.get(), reinterpret_cast<unsigned char *>(output.data()), &n,
              reinterpret_cast<const unsigned char *>(input.data()),
              input.size()) > 0,
          "RSA encrypt failed");
  return {{"params", QString::fromLatin1(encrypted)},
          {"encSecKey", QString::fromLatin1(output.toHex())}};
}
QJsonObject eapi(const QString &path, const QByteArray &json) {
  const auto uri = path.toUtf8();
  auto hash =
      QCryptographicHash::hash("nobody" + uri + "use" + json + "md5forencrypt",
                               QCryptographicHash::Md5)
          .toHex();
  return {{"params", QString::fromLatin1(aes(uri + "-36cd479b6b5-" + json +
                                                 "-36cd479b6b5-" + hash,
                                             ekey, true)
                                             .toHex()
                                             .toUpper())}};
}
QByteArray decodeResponse(const QByteArray &data) {
  if (data.trimmed().startsWith('{') || data.trimmed().startsWith('['))
    return data;
  auto plain = aes(data, ekey, false);
  if (plain.size() > 2 && static_cast<unsigned char>(plain[0]) == 0x1f &&
      static_cast<unsigned char>(plain[1]) == 0x8b) {
    z_stream z{};
    z.next_in = reinterpret_cast<Bytef *>(plain.data());
    z.avail_in = plain.size();
    require(inflateInit2(&z, 16 + MAX_WBITS) == Z_OK, "Gzip init failed");
    QByteArray result;
    int status;
    do {
      char chunk[8192];
      z.next_out = reinterpret_cast<Bytef *>(chunk);
      z.avail_out = sizeof(chunk);
      status = inflate(&z, Z_NO_FLUSH);
      result.append(chunk, sizeof(chunk) - z.avail_out);
      if (result.size() > 20 * 1024 * 1024) {
        inflateEnd(&z);
        throw std::runtime_error("Response too large");
      }
    } while (status == Z_OK);
    inflateEnd(&z);
    require(status == Z_STREAM_END, "Invalid gzip response");
    return result;
  }
  return plain;
}
QByteArray keySignature(const QString &timestamp, const QString &nonce) {
  return hmac(signkey, (timestamp + nonce).toUtf8()).toBase64();
}
QJsonObject decodePublicKey(const QString &encrypted) {
  return object(aes(QByteArray::fromBase64(encrypted.toLatin1()), xkey, false));
}
QJsonObject xeapi(const QString &path, QJsonObject data,
                  const QJsonObject &state, const QByteArray &sessionKey,
                  const QString &sessionId) {
  data.remove("e_r");
  QJsonObject fields{{"body", QString::fromLatin1(form(data).toBase64())},
                     {"queryString", QUrl(path).query().isEmpty()
                                         ? "e_r=true"
                                         : QUrl(path).query() + "&e_r=true"}};
  auto dynamic = sessionKey.isEmpty() ? randomBytes(16) : sessionKey;
  auto inner =
      aes(QJsonDocument(fields).toJson(QJsonDocument::Compact), xkey, true);
  auto mask = randomBytes(16);
  for (qsizetype i = 0; i < inner.size(); ++i)
    inner[i] = inner[i] ^ mask[i % 16];
  auto b64 = inner.toBase64();
  int rotation = (static_cast<unsigned char>(mask[0]) & 15) % b64.size();
  auto b = aes(mask + b64.mid(rotation) + b64.left(rotation), dynamic, true);
  auto raw =
      QByteArray::fromBase64(state.value("publicKey").toString().toLatin1());
  require(raw.size() == 32, "Invalid X25519 peer key");
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> peer(
      EVP_PKEY_new_raw_public_key(
          EVP_PKEY_X25519, nullptr,
          reinterpret_cast<const unsigned char *>(raw.data()), raw.size()),
      EVP_PKEY_free);
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> gen(
      EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr), EVP_PKEY_CTX_free);
  require(gen && EVP_PKEY_keygen_init(gen.get()) > 0, "X25519 init failed");
  EVP_PKEY *rawLocal = nullptr;
  require(EVP_PKEY_keygen(gen.get(), &rawLocal) > 0,
          "X25519 generation failed");
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> local(rawLocal,
                                                            EVP_PKEY_free);
  QByteArray pub(32, 0), shared(32, 0);
  size_t n = 32;
  require(EVP_PKEY_get_raw_public_key(
              local.get(), reinterpret_cast<unsigned char *>(pub.data()), &n) >
              0,
          "X25519 public key failed");
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> derive(
      EVP_PKEY_CTX_new(local.get(), nullptr), EVP_PKEY_CTX_free);
  require(derive && EVP_PKEY_derive_init(derive.get()) > 0 &&
              EVP_PKEY_derive_set_peer(derive.get(), peer.get()) > 0,
          "X25519 derive init failed");
  n = 32;
  require(EVP_PKEY_derive(derive.get(),
                          reinterpret_cast<unsigned char *>(shared.data()),
                          &n) > 0,
          "X25519 derive failed");
  auto derived =
      hmac(hmac(QByteArray(32, 0), shared), pub + QByteArray(1, 1)).left(16);
  auto iv = randomBytes(12);
  auto payload =
      dynamic.toBase64() + "|android|" + state.value("sk").toString().toUtf8();
  std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> cipher(
      EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  require(cipher &&
              EVP_EncryptInit_ex(
                  cipher.get(), EVP_aes_128_gcm(), nullptr,
                  reinterpret_cast<const unsigned char *>(derived.data()),
                  reinterpret_cast<const unsigned char *>(iv.data())) == 1,
          "GCM init failed");
  QByteArray ciphertext(payload.size() + 16, 0), tag(16, 0);
  int written = 0, tail = 0;
  require(EVP_EncryptUpdate(
              cipher.get(),
              reinterpret_cast<unsigned char *>(ciphertext.data()), &written,
              reinterpret_cast<const unsigned char *>(payload.data()),
              payload.size()) == 1,
          "GCM update failed");
  require(EVP_EncryptFinal_ex(
              cipher.get(),
              reinterpret_cast<unsigned char *>(ciphertext.data() + written),
              &tail) == 1 &&
              EVP_CIPHER_CTX_ctrl(cipher.get(), EVP_CTRL_GCM_GET_TAG, 16,
                                  tag.data()) == 1,
          "GCM finish failed");
  ciphertext.resize(written + tail);
  auto version = state.value("version").toVariant().toString().toUtf8();
  auto r = aes(version + "|" +
                   (sessionKey.isEmpty() ? QByteArray() : sessionId.toUtf8()),
               xkey, true);
  return {{"B", QString::fromLatin1(b.toBase64())},
          {"S", QString::fromLatin1((pub + iv + ciphertext + tag).toBase64())},
          {"R", QString::fromLatin1(r.toBase64())}};
}
} // namespace TransportCrypto

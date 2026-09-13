#include "crypto.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <cstdio>
#include <stdexcept>
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    QFile f(argc > 1 ? argv[1] : "tests/transport-vectors.json");
    if (!f.open(QIODevice::ReadOnly))
      throw std::runtime_error("Missing vectors");
    auto v = QJsonDocument::fromJson(f.readAll()).object();
    int count = 0;
    auto check = [&](bool ok, const char *name) {
      if (!ok)
        throw std::runtime_error(name);
      ++count;
      printf("PASS %s\n", name);
    };
    using namespace TransportCrypto;
    check(eapi(v["path"].toString(), v["json"].toString().toUtf8())["params"] ==
              v["eapi"],
          "EAPI matches independent Node crypto vector");
    check(decodeResponse(
              QByteArray::fromBase64(v["encrypted"].toString().toLatin1())) ==
              v["response"].toString().toUtf8(),
          "Encrypted response retains rights/trial/null fields");
    check(decodeResponse(
              QByteArray::fromBase64(v["compressed"].toString().toLatin1())) ==
              v["response"].toString().toUtf8(),
          "Gzip encrypted response decodes");
    check(form({{"a", "中文 & +"}, {"b", "a~!*()"}}) ==
              v["form"].toString().toUtf8(),
          "Form encoding matches URLSearchParams");
    auto we = weapi("{\"hello\":\"世界\"}", "abcdefghijklmnop");
    auto first = aes(QByteArray::fromBase64(we["params"].toString().toLatin1()),
                     "abcdefghijklmnop", false, "0102030405060708");
    check(aes(QByteArray::fromBase64(first), "0CoJUm6Qyw8W8jud", false,
              "0102030405060708") == "{\"hello\":\"世界\"}",
          "WEAPI double CBC round trip");
    check(we["encSecKey"].toString().size() == 256,
          "WEAPI RSA fixed-width padding");
    bool rejected = false;
    try {
      decodeResponse("broken ciphertext");
    } catch (...) {
      rejected = true;
    }
    check(rejected, "Malformed encrypted data rejected");
    printf("%d transport checks passed\n", count);
    return 0;
  } catch (const std::exception &e) {
    fprintf(stderr, "FAIL %s\n", e.what());
    return 1;
  }
}

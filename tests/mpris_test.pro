QT += core gui network sql dbus concurrent testlib
CONFIG += c++20 link_pkgconfig
TEMPLATE = app
TARGET = mpris-test
INCLUDEPATH += ../src
PKGCONFIG += mpv libsecret-1 openssl libqrencode taglib zlib
SOURCES += mpris_test.cpp ../src/backend.cpp ../src/mpris.cpp ../src/session.cpp ../src/crypto.cpp ../src/storage.cpp
HEADERS += ../src/backend.h ../src/mpris.h ../src/session.h ../src/crypto.h ../src/storage.h

SOURCES += ../src/loginflow.cpp
HEADERS += ../src/loginflow.h

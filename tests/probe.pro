QT += core network
CONFIG += console c++20 link_pkgconfig
CONFIG -= app_bundle
TEMPLATE = app
TARGET = protocol-probe
INCLUDEPATH += ../src
PKGCONFIG += libsecret-1 openssl zlib
SOURCES += probe.cpp ../src/session.cpp ../src/crypto.cpp
HEADERS += ../src/session.h ../src/crypto.h

SOURCES += ../src/storage.cpp
HEADERS += ../src/storage.h
QT += sql
PKGCONFIG += taglib

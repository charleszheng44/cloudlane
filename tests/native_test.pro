QT += core gui quick quickcontrols2 network sql dbus concurrent opengl testlib
CONFIG += c++20 link_pkgconfig
TARGET = native-test
TEMPLATE = app
INCLUDEPATH += ../src
PKGCONFIG += mpv libsecret-1 openssl libqrencode taglib zlib
SOURCES += native_test.cpp ../src/crypto.cpp ../src/session.cpp ../src/backend.cpp ../src/videoitem.cpp ../src/mpris.cpp
HEADERS += ../src/crypto.h ../src/session.h ../src/backend.h ../src/videoitem.h ../src/mpris.h
RESOURCES += ../resources.qrc

SOURCES += ../src/storage.cpp
HEADERS += ../src/storage.h

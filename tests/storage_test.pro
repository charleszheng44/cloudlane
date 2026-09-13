QT += core network sql testlib
CONFIG += console c++20 link_pkgconfig
TEMPLATE = app
TARGET = storage-test
INCLUDEPATH += ../src
PKGCONFIG += taglib
SOURCES += storage_test.cpp ../src/storage.cpp
HEADERS += ../src/storage.h

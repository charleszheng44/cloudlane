QT += core gui testlib
CONFIG += c++20
TEMPLATE = app
TARGET = theme-test
INCLUDEPATH += ../src
SOURCES += theme_test.cpp ../src/theme.cpp
HEADERS += ../src/theme.h

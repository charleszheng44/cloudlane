QT += core testlib
CONFIG += console c++20
TEMPLATE = app
TARGET = loginflow-test
INCLUDEPATH += ../src
SOURCES += loginflow_test.cpp ../src/loginflow.cpp
HEADERS += ../src/loginflow.h

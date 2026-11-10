QT += core gui widgets testlib
CONFIG += c++11 testcase console
# Qt 5.12 moc cannot resolve predefs through absolute paths containing Chinese characters.
win32:CONFIG += no_moc_predefs
CONFIG -= app_bundle
TARGET = editor_tests
TEMPLATE = app
INCLUDEPATH += ..
SOURCES += tst_editor.cpp ../mainwindow.cpp ../searchdialog.cpp ../codeeditor.cpp
HEADERS += ../mainwindow.h ../searchdialog.h ../codeeditor.h
FORMS += ../mainwindow.ui ../searchdialog.ui ../aboutdialog.ui

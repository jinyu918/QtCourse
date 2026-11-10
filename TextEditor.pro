QT += core gui widgets
CONFIG += c++11
# Qt 5.12 moc cannot resolve predefs through absolute paths containing Chinese characters.
win32:CONFIG += no_moc_predefs
TARGET = TextEditor
TEMPLATE = app
SOURCES += main.cpp mainwindow.cpp searchdialog.cpp codeeditor.cpp
HEADERS += mainwindow.h searchdialog.h codeeditor.h
FORMS += mainwindow.ui searchdialog.ui aboutdialog.ui

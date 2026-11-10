#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("简单文本编辑器"));
    QTranslator translator;
    if (translator.load(QStringLiteral("qtbase_zh_CN"),
                        QDir(QApplication::applicationDirPath()).filePath(QStringLiteral("translations")))
            || translator.load(QStringLiteral("qtbase_zh_CN"),
                               QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&translator);
    auto *window = new MainWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
    return app.exec();
}

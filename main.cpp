#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("简单文本编辑器"));
    auto *window = new MainWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
    return app.exec();
}

#include "mainwindow.h"

#include <QAction>
#include <QPlainTextEdit>
#include <QTest>

class EditorTest : public QObject
{
    Q_OBJECT

private slots:
    void designerInterface();
};

void EditorTest::designerInterface()
{
    MainWindow window;
    QVERIFY(window.findChild<QPlainTextEdit *>("editor"));
    QVERIFY(window.findChild<QAction *>("actionSave"));
    QVERIFY(window.findChild<QAction *>("actionFind"));
    QVERIFY(window.findChild<QAction *>("actionWrap")->isChecked());
}

QTEST_MAIN(EditorTest)
#include "tst_editor.moc"

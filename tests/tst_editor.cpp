#include "mainwindow.h"

#include <QAction>
#include <QPlainTextEdit>
#include <QTest>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>

class EditorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void designerInterface();
    void fileRoundTripAndSaveAs();
    void failedIoPreservesDocument();
    void closeCancelAndDiscard();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

void EditorTest::initTestCase()
{
    qApp->installEventFilter(this);
}

bool EditorTest::eventFilter(QObject *object, QEvent *event)
{
    // Qt 5.12 Windows message boxes dereference a missing native interface offscreen.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")
            && qobject_cast<QMessageBox *>(object) && event->type() == QEvent::Show)
        return true;
    return QObject::eventFilter(object, event);
}

void EditorTest::designerInterface()
{
    MainWindow window;
    QVERIFY(window.findChild<QPlainTextEdit *>("editor"));
    QVERIFY(window.findChild<QAction *>("actionSave"));
    QVERIFY(window.findChild<QAction *>("actionFind"));
    QVERIFY(window.findChild<QAction *>("actionWrap")->isChecked());
}

void EditorTest::fileRoundTripAndSaveAs()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    const QString text = QStringLiteral("中文文本\nsecond line\n末行");
    editor->insertPlainText(text);
    QVERIFY(window.isWindowModified());
    const QString first = directory.filePath(QStringLiteral("中文.txt"));
    QString error;
    QVERIFY2(window.saveFile(first, &error), qPrintable(error));
    QVERIFY(!window.isWindowModified());
    MainWindow reopened;
    QVERIFY2(reopened.loadFile(first, &error), qPrintable(error));
    QCOMPARE(reopened.findChild<QPlainTextEdit *>("editor")->toPlainText(), text);
    const QString second = directory.filePath("second.txt");
    QVERIFY(window.saveFile(second, &error));
    QCOMPARE(window.filePath(), second);
    editor->insertPlainText(QStringLiteral("修改"));
    window.findChild<QAction *>("actionSave")->trigger();
    QFile original(first);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(original.readAll()), text);
    QVERIFY(reopened.loadFile(second, &error));
    QCOMPARE(reopened.findChild<QPlainTextEdit *>("editor")->toPlainText(), editor->toPlainText());
}

void EditorTest::failedIoPreservesDocument()
{
    QTemporaryDir directory;
    MainWindow window;
    auto *editor = window.findChild<QPlainTextEdit *>("editor");
    editor->insertPlainText(QStringLiteral("必须保留的内容"));
    QString error;
    QVERIFY(!window.loadFile(directory.filePath("missing.txt"), &error));
    QVERIFY(!error.isEmpty());
    QFile binary(directory.filePath("binary.txt"));
    QVERIFY(binary.open(QIODevice::WriteOnly));
    binary.write(QByteArray::fromHex("fffe0000"));
    binary.close();
    QVERIFY(!window.loadFile(binary.fileName(), &error));
    QVERIFY(!window.saveFile(directory.filePath("missing/file.txt"), &error));
    QCOMPARE(editor->toPlainText(), QStringLiteral("必须保留的内容"));
    QVERIFY(window.filePath().isEmpty());
    QVERIFY(window.isWindowModified());
}

void EditorTest::closeCancelAndDiscard()
{
    MainWindow window;
    window.show();
    window.findChild<QPlainTextEdit *>("editor")->insertPlainText("unsaved");
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        box->button(QMessageBox::Cancel)->click();
    });
    QVERIFY(!window.close());
    QVERIFY(window.isVisible());
    QTimer::singleShot(50, [] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        box->button(QMessageBox::Discard)->click();
    });
    QVERIFY(window.close());
}

QTEST_MAIN(EditorTest)
#include "tst_editor.moc"

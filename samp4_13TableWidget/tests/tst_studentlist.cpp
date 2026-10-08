#include "mainwindow.h"

#include <QAction>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

class StudentListTest : public QObject
{
    Q_OBJECT

private slots:
    void toolbarButtonSetsFiveSourceRows()
    {
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("tableInfo");
        auto *action = window.findChild<QAction *>("actSetStudentList");
        auto *toolbar = window.findChild<QToolBar *>("toolBar");
        QVERIFY(table);
        QVERIFY(action);
        QVERIFY(toolbar);
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
        QVERIFY(button);
        QVERIFY(!button->icon().isNull());
        QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(button->text(), QStringLiteral("设置学生名单"));
        QCOMPARE(table->columnCount(), 5);
        button->click();

        QCOMPARE(table->rowCount(), 5);
        QCOMPARE(table->columnCount(), 7);
        const QStringList headers = {QStringLiteral("学号"), QStringLiteral("姓名"),
                                     QStringLiteral("性别"), QStringLiteral("行政班级"),
                                     QStringLiteral("院系"), QStringLiteral("专业"),
                                     QStringLiteral("修读性质")};
        for (int column = 0; column < headers.size(); ++column)
        {
            QCOMPARE(table->horizontalHeaderItem(column)->text(), headers.at(column));
            QVERIFY(table->horizontalHeaderItem(column)->font().bold());
            QCOMPARE(table->horizontalHeaderItem(column)->foreground().color(), QColor(Qt::red));
        }

        const QStringList ids = {"2024414300218", "2024414300219", "2024414300220",
                                 "2024414300223", "2024414300225"};
        const QStringList names = {QStringLiteral("刘晓辉"), QStringLiteral("罗名港"),
                                   QStringLiteral("马锦润"), QStringLiteral("邱振羽"),
                                   QStringLiteral("张杜锰")};
        for (int row = 0; row < ids.size(); ++row)
        {
            QCOMPARE(table->item(row, 0)->text(), ids.at(row));
            QCOMPARE(table->item(row, 1)->text(), names.at(row));
            QCOMPARE(table->item(row, 3)->text(), QStringLiteral("2024软件卓越2班"));
            for (int column = 0; column < table->columnCount(); ++column)
            {
                const auto *item = table->item(row, column);
                QVERIFY(item);
                const bool ownIdentity = row == 2 && column < 2;
                QCOMPARE(item->font().bold(), ownIdentity);
                QCOMPARE(item->foreground().color() == QColor(Qt::red), ownIdentity);
            }
        }
        QCOMPARE(table->selectionBehavior(), QAbstractItemView::SelectRows);
        QVERIFY(window.findChild<QRadioButton *>("rBtnSelectRow")->isChecked());
        QVERIFY(!window.findChild<QRadioButton *>("rBtnSelectItem")->isChecked());
        QVERIFY(table->selectedItems().isEmpty());
    }

    void nativePlaceFollowsRowAndResetsOnReload()
    {
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("tableInfo");
        auto *action = window.findChild<QAction *>("actSetStudentList");
        auto *label = window.findChild<QLabel *>("labNativePlace");
        QVERIFY(label);
        action->trigger();
        QCOMPARE(label->text(), QStringLiteral("籍贯："));

        const QStringList nativePlaces = {QStringLiteral("广东广州"), QStringLiteral("广东深圳"),
                                          QStringLiteral("广东东莞"), QStringLiteral("广东惠州"),
                                          QStringLiteral("广东佛山")};
        for (int row = 0; row < table->rowCount(); ++row)
        {
            QCOMPARE(table->item(row, 1)->data(Qt::UserRole).toString(), nativePlaces.at(row));
            table->setCurrentCell(row, 6);
            QCOMPARE(label->text(), QStringLiteral("籍贯：%1").arg(nativePlaces.at(row)));
        }
        table->setCurrentCell(2, 0);
        QCOMPARE(label->text(), QStringLiteral("籍贯：广东东莞"));
        bool fullStudentId = false;
        for (const auto *statusLabel : window.findChildren<QLabel *>())
            fullStudentId |= statusLabel->text() == QStringLiteral("学生ID：2024414300220");
        QVERIFY(fullStudentId);

        action->trigger();
        QCOMPARE(label->text(), QStringLiteral("籍贯："));
        QVERIFY(table->selectedItems().isEmpty());
        table->setCurrentCell(0, 0);
        QCOMPARE(label->text(), QStringLiteral("籍贯：广东广州"));
        table->setCurrentCell(-1, -1);
        QCOMPARE(label->text(), QStringLiteral("籍贯："));
    }

    void readingRosterIncludesAllSevenColumnsAndHandlesEmptyCells()
    {
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("tableInfo");
        window.findChild<QAction *>("actSetStudentList")->trigger();
        const QString studyType = table->item(2, 6)->text();
        window.findChild<QPushButton *>("btnReadToEdit")->click();
        auto *text = window.findChild<QPlainTextEdit *>("textEdit");
        const QString ownLine = text->toPlainText().split('\n').at(2);
        QVERIFY(ownLine.contains(QStringLiteral("2024414300220")));
        QVERIFY(ownLine.contains(QStringLiteral("马锦润")));
        QVERIFY(ownLine.endsWith(studyType + "   "));
        QVERIFY(!ownLine.contains(QStringLiteral("群众")));

        table->setRowCount(6);
        window.findChild<QPushButton *>("btnReadToEdit")->click();
        QCOMPARE(text->document()->blockCount(), 6);
        table->setCurrentCell(5, 6);
        QCOMPARE(window.findChild<QLabel *>("labNativePlace")->text(), QStringLiteral("籍贯：未提供"));
    }

    void existingRowButtonsKeepRosterColumnsAligned()
    {
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("tableInfo");
        window.findChild<QAction *>("actSetStudentList")->trigger();
        window.findChild<QPushButton *>("btnInsertRow")->click();
        QCOMPARE(table->rowCount(), 6);
        QCOMPARE(table->item(5, 1)->text(), QStringLiteral("新学生"));
        window.findChild<QAction *>("actSetStudentList")->trigger();
        window.findChild<QPushButton *>("btnAppendRow")->click();
        QCOMPARE(table->rowCount(), 6);
        QCOMPARE(table->item(5, 1)->text(), QStringLiteral("新生"));
        QVERIFY(table->item(5, 0)->text().isEmpty());
        table->setCurrentCell(2, 6);
        window.findChild<QPushButton *>("btnInsertRow")->click();
        QCOMPARE(table->item(2, 1)->text(), QStringLiteral("新学生"));
        QCOMPARE(table->item(3, 0)->text(), QStringLiteral("2024414300220"));
        window.findChild<QPushButton *>("btnIniData")->click();
        QCOMPARE(table->rowCount(), 5);
        QCOMPARE(table->item(2, 0)->text(), QStringLiteral("2024414300220"));
    }

    void originalDemoStillReadsCheckStates()
    {
        MainWindow window;
        window.findChild<QPushButton *>("btnSetHeader")->click();
        window.findChild<QPushButton *>("btnIniData")->click();
        window.findChild<QPushButton *>("btnReadToEdit")->click();
        const QStringList lines = window.findChild<QPlainTextEdit *>("textEdit")->toPlainText().split('\n');
        QVERIFY(lines.at(0).contains(QStringLiteral("学生0")));
        QVERIFY(lines.at(0).contains(QStringLiteral("群众")));
        QVERIFY(lines.at(1).contains(QStringLiteral("党员")));

        window.findChild<QAction *>("actSetStudentList")->trigger();
        window.findChild<QPushButton *>("btnSetHeader")->click();
        window.findChild<QPushButton *>("btnIniData")->click();
        auto *table = window.findChild<QTableWidget *>("tableInfo");
        QCOMPARE(table->columnCount(), 6);
        QCOMPARE(table->item(0, 0)->text(), QStringLiteral("学生0"));
        table->setCurrentCell(0, 5);
        QCOMPARE(window.findChild<QLabel *>("labNativePlace")->text(), QStringLiteral("籍贯："));
    }
};

QTEST_MAIN(StudentListTest)
#include "tst_studentlist.moc"

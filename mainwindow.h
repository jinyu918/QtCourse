#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui { class MainWindow; }
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool loadFile(const QString &path, QString *error = nullptr);
    bool saveFile(const QString &path, QString *error = nullptr);
    QString filePath() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    bool on_actionSave_triggered();
    bool on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionExit_triggered();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    QString currentPath;
    QLabel *statisticsLabel;
    QLabel *positionLabel;
    bool maybeSave();
    void updateTitle();
    void updateActions();
    void updateStatus();
    void setUpEditing();
};

#endif

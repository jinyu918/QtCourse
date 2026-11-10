#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui { class MainWindow; }
class QLabel;
class SearchDialog;

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
    void on_actionFind_triggered();
    void on_actionReplace_triggered();
    void on_actionAbout_triggered();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    QString currentPath;
    QLabel *statisticsLabel;
    QLabel *positionLabel;
    SearchDialog *findDialog = nullptr;
    SearchDialog *replaceDialog = nullptr;
    bool maybeSave();
    void updateTitle();
    void updateActions();
    void updateStatus();
    void setUpEditing();
    void showSearch(bool replaceMode);
};

#endif

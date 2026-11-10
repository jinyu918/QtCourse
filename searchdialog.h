#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QTextDocument>
#include <memory>

class QPlainTextEdit;
namespace Ui { class SearchDialog; }

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    SearchDialog(QPlainTextEdit *editor, bool replaceMode, QWidget *parent);
    ~SearchDialog() override;
    void setSearchText(const QString &text);

private slots:
    void findNext();
    void replaceOne();
    void replaceAll();
    void updateButtons();

private:
    std::unique_ptr<Ui::SearchDialog> ui;
    QPlainTextEdit *editor;
    QTextDocument::FindFlags findFlags() const;
};

#endif

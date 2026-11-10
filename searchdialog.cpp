#include "searchdialog.h"
#include "ui_searchdialog.h"

#include <QPlainTextEdit>
#include <QTextCursor>

SearchDialog::SearchDialog(QPlainTextEdit *editor, bool replaceMode, QWidget *parent)
    : QDialog(parent), ui(new Ui::SearchDialog), editor(editor)
{
    ui->setupUi(this);
    setWindowTitle(replaceMode ? tr("替换") : tr("查找"));
    ui->replaceLabel->setVisible(replaceMode);
    ui->replaceEdit->setVisible(replaceMode);
    ui->replaceButton->setVisible(replaceMode);
    ui->replaceAllButton->setVisible(replaceMode);
    connect(ui->findButton, &QPushButton::clicked, this, &SearchDialog::findNext);
    connect(ui->replaceButton, &QPushButton::clicked, this, &SearchDialog::replaceOne);
    connect(ui->replaceAllButton, &QPushButton::clicked, this, &SearchDialog::replaceAll);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &SearchDialog::updateButtons);
    updateButtons();
}

SearchDialog::~SearchDialog() = default;

void SearchDialog::setSearchText(const QString &text)
{
    ui->searchEdit->setText(text);
    ui->searchEdit->selectAll();
    ui->searchEdit->setFocus();
}

QTextDocument::FindFlags SearchDialog::findFlags() const
{
    QTextDocument::FindFlags flags;
    if (ui->caseCheck->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeWordCheck->isChecked()) flags |= QTextDocument::FindWholeWords;
    return flags;
}

void SearchDialog::updateButtons()
{
    const bool enabled = !ui->searchEdit->text().isEmpty();
    ui->findButton->setEnabled(enabled);
    ui->replaceButton->setEnabled(enabled);
    ui->replaceAllButton->setEnabled(enabled);
    ui->feedbackLabel->setText(enabled ? tr("就绪") : tr("请输入查找内容"));
}

void SearchDialog::findNext()
{
    const QString needle = ui->searchEdit->text();
    if (needle.isEmpty()) return;
    QTextCursor found = editor->document()->find(needle, editor->textCursor(), findFlags());
    const bool wrapped = found.isNull();
    if (wrapped) found = editor->document()->find(needle, 0, findFlags());
    if (found.isNull()) {
        ui->feedbackLabel->setText(tr("未找到“%1”").arg(needle));
        return;
    }
    editor->setTextCursor(found);
    editor->ensureCursorVisible();
    ui->feedbackLabel->setText(wrapped ? tr("已从文档开头继续查找") : tr("已找到下一处"));
}

void SearchDialog::replaceOne()
{
    const QString needle = ui->searchEdit->text();
    if (needle.isEmpty()) return;
    QTextCursor cursor = editor->textCursor();
    const QTextCursor match = editor->document()->find(needle, cursor.selectionStart(), findFlags());
    if (!cursor.hasSelection() || match.isNull()
            || cursor.selectionStart() != match.selectionStart()
            || cursor.selectionEnd() != match.selectionEnd()) {
        findNext();
        return;
    }
    cursor.insertText(ui->replaceEdit->text());
    editor->setTextCursor(cursor);
    findNext();
    ui->feedbackLabel->setText(tr("已替换 1 处；%1").arg(ui->feedbackLabel->text()));
}

void SearchDialog::replaceAll()
{
    const QString needle = ui->searchEdit->text();
    if (needle.isEmpty()) return;
    const QString replacement = ui->replaceEdit->text();
    QTextCursor cursor(editor->document());
    cursor.beginEditBlock();
    int count = 0;
    for (;;) {
        const QTextCursor found = editor->document()->find(needle, cursor, findFlags());
        if (found.isNull()) break;
        cursor.setPosition(found.selectionStart());
        cursor.setPosition(found.selectionEnd(), QTextCursor::KeepAnchor);
        cursor.insertText(replacement);
        ++count;
    }
    cursor.endEditBlock();
    editor->setTextCursor(cursor);
    ui->feedbackLabel->setText(count ? tr("已替换 %1 处").arg(count) : tr("未找到“%1”").arg(needle));
}

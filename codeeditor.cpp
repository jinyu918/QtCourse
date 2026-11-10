#include "codeeditor.h"

#include <QPainter>
#include <QTextBlock>

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), editor(editor)
    {
        setObjectName(QStringLiteral("lineNumberArea"));
    }

    QSize sizeHint() const override
    {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        editor->paintLineNumbers(event);
    }

private:
    CodeEditor *editor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this))
{
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    updateLineNumberAreaWidth();
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!showLineNumbers) return 0;
    int digits = 1;
    for (int count = blockCount(); count >= 10; count /= 10) ++digits;
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

bool CodeEditor::lineNumbersVisible() const
{
    return showLineNumbers;
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (showLineNumbers == visible) return;
    showLineNumbers = visible;
    lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth();
}

void CodeEditor::updateLineNumberAreaWidth()
{
    const int width = lineNumberAreaWidth();
    setViewportMargins(width, 0, 0, 0);
    const QRect rect = contentsRect();
    lineNumberArea->setGeometry(rect.left(), rect.top(), width, rect.height());
    lineNumberArea->update();
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) lineNumberArea->scroll(0, dy);
    else lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateLineNumberAreaWidth();
}

void CodeEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange)
        updateLineNumberAreaWidth();
}

void CodeEditor::paintLineNumbers(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));
    painter.setPen(palette().color(QPalette::Text));
    painter.setFont(font());
    QTextBlock block = firstVisibleBlock();
    int number = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top())
            painter.drawText(0, top, lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, QString::number(number + 1));
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++number;
    }
}

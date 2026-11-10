#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth() const;
    bool lineNumbersVisible() const;
    void setLineNumbersVisible(bool visible);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    friend class LineNumberArea;
    LineNumberArea *lineNumberArea;
    bool showLineNumbers = true;
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void paintLineNumbers(QPaintEvent *event);
};

#endif

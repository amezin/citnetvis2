#include "sparqltextedit.h"

#include <QKeyEvent>
#include <QTextBlock>
#include <QFontMetrics>
#include <QPainter>
#include <QTextDocumentFragment>

#include "sparqlhighlighter.h"

SparqlTextEdit::SparqlTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
    setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont monospace("monospace");
    monospace.setStyleHint(QFont::TypeWriter);
    document()->setDefaultFont(monospace);

    new SparqlHighlighter(document());

    connect(this, SIGNAL(blockCountChanged(int)), SLOT(repaintLineNumbers()));
    connect(this, SIGNAL(updateRequest(QRect,int)),
            SLOT(updateRequested(QRect,int)));

    connect(this, SIGNAL(cursorPositionChanged()), SLOT(highlightLine()));
    connect(this, SIGNAL(selectionChanged()), SLOT(highlightLine()));
    highlightLine();
}

void SparqlTextEdit::keyPressEvent(QKeyEvent *e)
{
    QPlainTextEdit::keyPressEvent(e);

    if (e->key() == Qt::Key_Return) {
        QString prev = textCursor().block().previous().text();

        int indentLength = 0;
        while (indentLength < prev.length() && prev[indentLength].isSpace())
        {
            indentLength++;
        }

        textCursor().insertText(prev.left(indentLength));
    }
}

void SparqlTextEdit::paintEvent(QPaintEvent *e)
{
    setTabStopWidth(4 * QFontMetrics(currentCharFormat().font()).width(' '));
    QPlainTextEdit::paintEvent(e);
}

void SparqlTextEdit::paintLineNumbers(QPaintEvent *e)
{
    QFontMetrics metrics(currentCharFormat().font());

    int digits = 0;
    int blocks = blockCount();
    do {
        blocks /= 10;
        ++digits;
    } while (blocks);

    int maxCharWidth = 0;
    for (char c = '0'; c <= '9'; c++) {
        maxCharWidth = qMax(maxCharWidth, metrics.width(c));
    }

    int newLeftMargin = frameWidth() + maxCharWidth * digits;
    if (leftMargin != newLeftMargin) {
        leftMargin = newLeftMargin;
        setViewportMargins(leftMargin, 0, 0, 0);
    }

    QPainter painter(this);
    painter.setClipRect(lineNumbersRect(), Qt::IntersectClip);
    painter.fillRect(e->rect(), palette().window());
    painter.setPen(palette().windowText().color());
    painter.setFont(currentCharFormat().font());

    QTextBlock block = firstVisibleBlock();

    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block)
            .translated(contentOffset()).top() + lineNumbersRect().top();
    int bottom = top + (int) blockBoundingRect(block).height();
    while (block.isValid() && top <= e->rect().bottom()) {
        if (block.isVisible() && bottom >= e->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, leftMargin, metrics.height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

bool SparqlTextEdit::event(QEvent *e)
{
    QPaintEvent *paint = dynamic_cast<QPaintEvent*>(e);
    bool result = QPlainTextEdit::event(e);
    if (paint) {
        paintLineNumbers(paint);
    }
    return result;
}

void SparqlTextEdit::repaintLineNumbers()
{
    repaint(lineNumbersRect());
}

void SparqlTextEdit::updateRequested(const QRect &, int dy)
{
    if (dy) {
        repaintLineNumbers();
    }
}

void SparqlTextEdit::highlightLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly() && textCursor().selection().isEmpty()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = palette().color(QPalette::Highlight);
        lineColor.setAlphaF(0.25);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

QRect SparqlTextEdit::lineNumbersRect() const
{
    QRect frame = frameRect();
    frame.adjust(frameWidth(), frameWidth(), -frameWidth(), -frameWidth());
    frame.setWidth(qMin(frame.width(), leftMargin));
    return frame;
}

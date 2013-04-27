#ifndef SPARQLTEXTEDIT_H
#define SPARQLTEXTEDIT_H

#include <QPlainTextEdit>

class SparqlTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit SparqlTextEdit(QWidget *parent = 0);

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void paintEvent(QPaintEvent *e);
    virtual bool event(QEvent *e);

private slots:
    void repaintLineNumbers();
    void updateRequested(const QRect &rect, int dy);
    void highlightLine();

private:
    void paintLineNumbers(QPaintEvent *e);
    QRect lineNumbersRect() const;

    int leftMargin;
};

#endif // SPARQLTEXTEDIT_H

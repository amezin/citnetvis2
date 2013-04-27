#ifndef QUERYEDITOR_H
#define QUERYEDITOR_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFontDialog>

#include "sparqltextedit.h"
#include "persistentwidget.h"

class QueryEditor : public QMainWindow, public PersistentWidget
{
    Q_OBJECT

public:
    explicit QueryEditor(QWidget *parent = 0);

    QString text() const { return textEdit->toPlainText(); }

    virtual void loadState(const QSettings *);
    virtual void saveState(QSettings *) const;

private slots:
    void load();
    void save();
    void changeFont();

private:
    QAction *addAction(const QString &text,
                       const QString &icon,
                       const QKeySequence &shortcut,
                       const char *slot = 0,
                       const char *availableSignal = 0);

    SparqlTextEdit *textEdit;
    QToolBar *toolBar;
    QFileDialog *fileDialog;
    QString fileName;
    QFontDialog *fontDialog;
};

#endif // QUERYEDITOR_H

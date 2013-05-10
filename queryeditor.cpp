#include "queryeditor.h"

#include <QAction>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QTextStream>
#include <QToolBar>

QAction *QueryEditor::addAction(const QString &text,
                                const QIcon &icon,
                                const QKeySequence &shortcut,
                                const char *slot,
                                const char *availableSignal)
{
    auto action = new QAction(icon, text, this);
    action->setShortcut(shortcut);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    if (slot) {
        textEdit->connect(action, SIGNAL(triggered()), slot);
    }

    if (availableSignal) {
        action->connect(textEdit, availableSignal, SLOT(setEnabled(bool)));
    }

    toolBar->addAction(action);
    return action;
}

QueryEditor::QueryEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("QueryEditor");

    textEdit = new SparqlTextEdit(this);
    setCentralWidget(textEdit);

    toolBar = new QToolBar("Query editor tool bar", this);
    toolBar->setObjectName(objectName() + "Toolbar");
    addToolBar(toolBar);

    auto loadAction = addAction("L&oad", QIcon::fromTheme("document-open"),
                                QKeySequence::Open);
    connect(loadAction, SIGNAL(triggered()), SLOT(load()));

    auto saveAction = addAction("&Save", QIcon::fromTheme("document-save"),
                                QKeySequence::Save);
    connect(saveAction, SIGNAL(triggered()), SLOT(save()));

    toolBar->addSeparator();

    addAction("&Undo", QIcon::fromTheme("edit-undo"), QKeySequence::Undo,
              SLOT(undo()), SIGNAL(undoAvailable(bool)))->setEnabled(false);
    addAction("&Redo", QIcon::fromTheme("edit-redo"), QKeySequence::Redo,
              SLOT(redo()), SIGNAL(redoAvailable(bool)))->setEnabled(false);

    toolBar->addSeparator();

    addAction("Cu&t", QIcon::fromTheme("edit-cut"), QKeySequence::Cut,
              SLOT(cut()), SIGNAL(copyAvailable(bool)))->setEnabled(false);
    addAction("&Copy", QIcon::fromTheme("edit-copy"), QKeySequence::Copy,
              SLOT(copy()), SIGNAL(copyAvailable(bool)))->setEnabled(false);
    addAction("&Paste", QIcon::fromTheme("edit-paste"), QKeySequence::Paste,
              SLOT(paste()));

    toolBar->addSeparator();

    addAction("Select &All", QIcon::fromTheme("edit-select-all"),
              QKeySequence::SelectAll, SLOT(selectAll()));

    toolBar->addSeparator();

    auto fontAction = addAction("Change &font",
                                QIcon::fromTheme("preferences-desktop-font"),
                                QKeySequence::Preferences);
    connect(fontAction, SIGNAL(triggered()), SLOT(changeFont()));

    textEdit->addActions(toolBar->actions());
    textEdit->setContextMenuPolicy(Qt::ActionsContextMenu);

    fileDialog = new QFileDialog(this);
    fontDialog = new QFontDialog(this);

    textEdit->setPlainText("PREFIX akt:\t<http://www.aktors.org/ontology/portal#>\n"
                           "SELECT DISTINCT ?publication\n"
                           "WHERE {\n"
                           "\t?publication akt:addresses-generic-area-of-interest ?i1.\n"
                           "\tFILTER (\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#G.2.2> ||\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#I.2.8.3> ||\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#G.2.2.0> ||\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#G.2.2.1> ||\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#G.2.2.2> ||\n"
                           "\t\t?i1 = <http://acm.rkbexplorer.com/ontologies/acm#G.2.2.5>\n"
                           "\t)\n"
                           "} LIMIT 5");
}

void QueryEditor::load()
{
    fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->selectFile(this->fileName);

    if (!fileDialog->exec() || fileDialog->selectedFiles().isEmpty()) {
        return;
    }

    QString fileName = fileDialog->selectedFiles()[0];
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    QString data;
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        data = stream.readAll();
    }
    file.close();
    if (file.error() == QFile::NoError) {
        textEdit->setPlainText(data);
        this->fileName = fileName;
    } else {
        QMessageBox::critical(this, fileName, file.errorString());
    }
}

void QueryEditor::save()
{
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->selectFile(this->fileName);

    if (!fileDialog->exec() || fileDialog->selectedFiles().isEmpty()) {
        return;
    }

    QString fileName = fileDialog->selectedFiles()[0];
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream stream(&file);
        stream << textEdit->toPlainText();
    }
    file.close();
    if (file.error() == QFile::NoError) {
        this->fileName = fileName;
    } else {
        QMessageBox::critical(this, fileName, file.errorString());
    }
}

void QueryEditor::changeFont()
{
    fontDialog->setCurrentFont(textEdit->currentCharFormat().font());

    if (!fontDialog->exec()) {
        return;
    }

    textEdit->document()->setDefaultFont(fontDialog->selectedFont());
}

void QueryEditor::loadState(const QSettings *settings)
{
    QString myName = objectName();
    restoreState(settings->value(myName + "State").toByteArray());

    if (settings->contains(myName + "Text")) {
        textEdit->setPlainText(settings->value(myName + "Text").toString());
    }

    if (settings->contains(myName + "Font")
            && settings->value(myName + "Font").canConvert<QFont>()) {
        textEdit->document()->setDefaultFont(
                    settings->value(myName + "Font").value<QFont>());
    }
}

void QueryEditor::saveState(QSettings *settings) const
{
    QString myName = objectName();
    settings->setValue(myName + "State", QMainWindow::saveState());
    settings->setValue(myName + "Text", textEdit->toPlainText());
    settings->setValue(myName + "Font", textEdit->currentCharFormat().font());
}

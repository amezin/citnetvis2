#include "logwidget.h"

#include <QSet>
#include <QScopedPointer>
#include <QVBoxLayout>

static QtMsgHandler oldMsgHandler = 0;
static bool installedHandler = false;

typedef QSet<LogWidget*> Loggers;
Q_GLOBAL_STATIC(Loggers, loggers)

static void messageHandler(QtMsgType type, const char *msg)
{
    foreach (auto i, *loggers()) {
        i->message(type, msg);
    }

    if (oldMsgHandler) {
        oldMsgHandler(type, msg);
    }
}

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent),
      debugIcon(QIcon::fromTheme("dialog-information")),
      warningIcon(QIcon::fromTheme("dialog-warning")),
      criticalIcon(QIcon::fromTheme("dialog-error"))
{
    model = new QStandardItemModel(this);
    view = new QListView(this);
    view->setModel(model);
    view->setWordWrap(true);

    auto layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->addWidget(view);

    if (!installedHandler) {
        oldMsgHandler = qInstallMsgHandler(messageHandler);
        installedHandler = true;
    }

    loggers()->insert(this);
}

LogWidget::~LogWidget()
{
    loggers()->remove(this);
}

void LogWidget::message(QtMsgType type, const char *msg)
{
    QIcon *icon;

    switch (type) {
    case QtDebugMsg:
        icon = &debugIcon;
        break;
    case QtWarningMsg:
        icon = &warningIcon;
        break;
    default:
        icon = &criticalIcon;
    }

    QScopedPointer<QStandardItem> item(new QStandardItem(
                                           *icon, QString::fromLocal8Bit(msg)));
    model->appendRow(item.data());
    item.take();

    emit message();
}

void LogWidget::clear()
{
    model->clear();
}

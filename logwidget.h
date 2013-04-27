#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QIcon>
#include <QListView>
#include <QStandardItemModel>

class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = 0);
    virtual ~LogWidget();

signals:
    void message();

public slots:
    void message(QtMsgType, const char *);
    void clear();
    
private:
    QIcon debugIcon, warningIcon, criticalIcon;
    QListView *view;
    QStandardItemModel *model;
};

#endif // LOGWIDGET_H

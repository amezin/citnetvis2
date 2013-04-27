#include <QApplication>
#include <QSettings>
#include <QNetworkProxyFactory>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    QSettings settings("Alexander Mezin", "citnetvis2");

    MainWindow w(&settings);
    w.show();
    
    return a.exec();
}

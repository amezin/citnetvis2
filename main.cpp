#include <QApplication>
#include <QSettings>
#include <QNetworkProxyFactory>
#include <QIcon>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    if (QIcon::themeName().isEmpty()) {
        QIcon::setThemeSearchPaths(QStringList()
                                   << ":\\oxygen-icons");
        QIcon::setThemeName("oxygen");
    }

    QSettings settings("Alexander Mezin", "citnetvis2");

    MainWindow w(&settings);
    w.show();
    
    return a.exec();
}

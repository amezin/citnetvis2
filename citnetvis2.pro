#-------------------------------------------------
#
# Project created by QtCreator 2013-04-17T21:06:55
#
#-------------------------------------------------

QT       += core gui network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 4): DEFINES += "QT_DISABLE_DEPRECATED_BEFORE=0"

RESOURCES += "icons.qrc"

TARGET = citnetvis2
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp\
        mainwindow.cpp \
    queryeditor.cpp \
    dockbutton.cpp \
    sparqltextedit.cpp \
    progressoverlay.cpp \
    sparqlquery.cpp \
    dataset.cpp \
    sparqltokenizer.cpp \
    sparqlhighlighter.cpp \
    persistentfield.cpp \
    logwidget.cpp \
    sparqlqueryinfo.cpp \
    scene.cpp \
    graphview.cpp \
    datasettingswidget.cpp \
    visualisationsettingswidget.cpp \
    lineanimation.cpp \
    nodeanimation.cpp \
    labelanimation.cpp \
    opacityanimation.cpp \
    disappearanimation.cpp \
    nodeinfowidget.cpp

HEADERS  += mainwindow.h \
    queryeditor.h \
    dockbutton.h \
    sparqltextedit.h \
    progressoverlay.h \
    sparqlquery.h \
    dataset.h \
    sparqltokenizer.h \
    sparqlhighlighter.h \
    persistentwidget.h \
    persistentfield.h \
    logwidget.h \
    sparqlqueryinfo.h \
    publication.h \
    scene.h \
    vnode.h \
    identifier.h \
    graphview.h \
    datasettingswidget.h \
    visualisationsettingswidget.h \
    lineanimation.h \
    nodeanimation.h \
    labelanimation.h \
    opacityanimation.h \
    disappearanimation.h \
    nodeinfowidget.h

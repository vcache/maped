/*
 * \file main.h
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 * \brief Main file.
 **/
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

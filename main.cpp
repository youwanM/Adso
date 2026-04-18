#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    // Check if an argument was provided
    if (argc > 1) {
        // Convert the C-style string to a QString
        QString initialPath = QString::fromLocal8Bit(argv[1]);
        w.loadVolume(initialPath);
    }

    w.show();
    return a.exec();
}
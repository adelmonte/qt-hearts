#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Hearts");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Hearts");
    app.setWindowIcon(QIcon::fromTheme("qt-hearts", QIcon(":/data/icons/qt-hearts.svg")));

    MainWindow window;
    window.show();

    return app.exec();
}


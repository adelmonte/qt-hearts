#include "mainwindow.h"
#include "smoothanimationdriver.h"
#include <QApplication>
#include <QIcon>
#include <QScreen>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Hearts");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Hearts");
    app.setWindowIcon(QIcon::fromTheme("qt-hearts", QIcon(":/data/icons/qt-hearts.svg")));

    // Install custom animation driver for smooth animations on high refresh rate displays
    // Default Qt animation driver is capped at ~60fps
    int targetFps = 120;  // Default to 120fps
    if (QScreen* screen = app.primaryScreen()) {
        qreal refreshRate = screen->refreshRate();
        if (refreshRate > 60) {
            // Target slightly above refresh rate to avoid frame drops
            targetFps = static_cast<int>(refreshRate * 1.2);
        }
    }
    SmoothAnimationDriver* animDriver = new SmoothAnimationDriver(targetFps);
    animDriver->install();

    MainWindow window;
    window.show();

    return app.exec();
}


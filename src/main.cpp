#include <QtWidgets/QApplication>
#include "render/GameWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName(QStringLiteral("HillClimbGame"));
    QApplication::setApplicationDisplayName(QStringLiteral("Hill Climb Racing 2D - Qt 6 MinGW"));
    QApplication::setOrganizationName(QStringLiteral("QtGameStudio"));

    Render::GameWidget game;
    game.show();

    return app.exec();
}

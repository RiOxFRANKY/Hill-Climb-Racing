#include "gamewidget.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFont>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>

int main(int argc, char *argv[])
{
    // This game intentionally uses an exact 1920x1080 render surface instead of
    // allowing Windows display scaling to turn it into (for example) 2400x1350.
    qputenv("QT_FONT_DPI", QByteArrayLiteral("96"));

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Hill Climb Qt"));
    app.setOrganizationName(QStringLiteral("HCR"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A basic hill-climb driving game made with Qt."));
    parser.addHelpOption();
    QCommandLineOption screenshotOption(
        QStringList{QStringLiteral("s"), QStringLiteral("screenshot")},
        QStringLiteral("Save a 1920x1080 preview frame and exit."),
        QStringLiteral("file"));
    parser.addOption(screenshotOption);
    parser.process(app);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(16, 30, 48));
    palette.setColor(QPalette::WindowText, Qt::white);
    app.setPalette(palette);

    QFont defaultFont(QStringLiteral("Segoe UI"));
    defaultFont.setStyleHint(QFont::SansSerif);
    app.setFont(defaultFont);

    GameWidget game;
    game.show();

    if (parser.isSet(screenshotOption)) {
        const QString filePath = QDir::current().absoluteFilePath(parser.value(screenshotOption));
        QTimer::singleShot(250, &game, [&app, &game, filePath] {
            const bool saved = game.grab().save(filePath);
            app.exit(saved ? 0 : 2);
        });
    }

    return app.exec();
}

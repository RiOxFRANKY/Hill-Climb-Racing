#include "quick/game_controller.h"
#include "quick/sprite_provider.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Hill Climb Qt"));
    app.setOrganizationName(QStringLiteral("HCR"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A basic hill-climb driving game made with Qt."));
    parser.addHelpOption();
    QCommandLineOption screenshotOption(
        QStringList{QStringLiteral("s"), QStringLiteral("screenshot")},
        QStringLiteral("Save a 1920x1080 preview frame and exit."),
        QStringLiteral("file"));
    parser.addOption(screenshotOption);
    QCommandLineOption playOption(
        QStringList{QStringLiteral("p"), QStringLiteral("play")},
        QStringLiteral("Skip the start menu and begin a race straight away."));
    parser.addOption(playOption);
    QCommandLineOption seedOption(
        QStringLiteral("seed"),
        QStringLiteral("Seed for the procedural course (default 1)."),
        QStringLiteral("number"), QStringLiteral("1"));
    parser.addOption(seedOption);
    QCommandLineOption courseOption(
        QStringLiteral("course"),
        QStringLiteral("Drive a course from a JSON file instead, e.g. :/assets/terrain_10km.json."),
        QStringLiteral("file"));
    parser.addOption(courseOption);
    parser.process(app);

    bool seedOk = false;
    const quint32 seed = parser.value(seedOption).toUInt(&seedOk);
    if (!seedOk)
        parser.showHelp(1);
    GameController::setCourse(parser.value(courseOption), seed);

#ifdef Q_OS_ANDROID
    // The game is designed for a 16:9 landscape screen.
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        constexpr jint SensorLandscape = 6; // ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
        activity.callMethod<void>("setRequestedOrientation", "(I)V", SensorLandscape);
    });
#endif

    // The scene is scaled to fit the screen; curve-rendered text keeps the
    // pixel fonts sharp at any scale.
    QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("sprites"), new SpriteProvider);
    engine.setInitialProperties({{QStringLiteral("startInGame"), parser.isSet(playOption)}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("HillClimb", "Main");

    QObject *root = engine.rootObjects().isEmpty() ? nullptr : engine.rootObjects().constFirst();
    auto *stage = root ? root->findChild<QQuickItem *>(QStringLiteral("stage")) : nullptr;
    if (stage && parser.isSet(screenshotOption)) {
        // Grab the design surface itself so the frame is 1920x1080 whatever
        // size the window ended up.
        const QString filePath = QDir::current().absoluteFilePath(parser.value(screenshotOption));
        QTimer::singleShot(1200, stage, [&app, stage, filePath] {
            // The grab size is in logical pixels; undo display scaling.
            const QSize frameSize(1920, 1080);
            const qreal dpr = stage->window()->effectiveDevicePixelRatio();
            const QSharedPointer<QQuickItemGrabResult> grab =
                stage->grabToImage(QSize(qRound(frameSize.width() / dpr), qRound(frameSize.height() / dpr)));
            if (!grab) {
                app.exit(2);
                return;
            }
            QObject::connect(grab.data(), &QQuickItemGrabResult::ready, &app, [&app, grab, filePath, frameSize] {
                QImage image = grab->image();
                if (image.size() != frameSize)
                    image = image.scaled(frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                app.exit(image.save(filePath) ? 0 : 2);
            });
        });
    }

    return app.exec();
}

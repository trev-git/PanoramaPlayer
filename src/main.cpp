#include <QGuiApplication>
#include <QScreen>
#include <QCommandLineParser>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDir>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("Stereo video Panorama Player for virtual reality glasses"));
    app.setApplicationName(QStringLiteral("PanoramaPlayer"));
    app.setOrganizationName(QStringLiteral("Rpi5VRproject"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    QString descrText = app.applicationDisplayName();
    const auto screenList = app.screens();
    if (!screenList.isEmpty()) {
        descrText += QStringLiteral("\n    Available output screens (see --output option):");
        for (int i = 0; i < screenList.size(); i++) {
            const auto scr = screenList.at(i);
            descrText += QString("\n\t%1%2: %3 (%4 %5x%6/%7)")
                    .arg(scr == app.primaryScreen() ? '*' : ' ').arg(i).arg(scr->name()).arg(scr->model())
                    .arg(scr->size().width()).arg(scr->size().height()).arg(scr->refreshRate());
        }
    }
    QCommandLineParser parser;
    parser.setApplicationDescription(descrText);
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption debugOption({ "d", "debug" }, QStringLiteral("Enable OpenGL debugging output"));
    parser.addOption(debugOption);
#if !defined(Q_OS_MACOS) && !defined(Q_PROCESSOR_ARM)
    QCommandLineOption glesOption({ "e", "gles" }, QStringLiteral("Use OpenGLES instead of OpenGL"));
    parser.addOption(glesOption);
#endif
    QCommandLineOption fullOption({{ "f", "fullscreen" }, QStringLiteral("Full-screen mode, on an ARM processor by default") });
    parser.addOption(fullOption);
    QCommandLineOption outputOption({ "o", "output" }, QStringLiteral("The screen <index> to play video"), QStringLiteral("index"));
    parser.addOption(outputOption);
    parser.addPositionalArgument(QStringLiteral("source"), QStringLiteral("The URL of the video source to open (video360 format)"));
    parser.process(app);

    QUrl sourceUrl;
    if (!parser.positionalArguments().isEmpty())
        sourceUrl = QUrl::fromUserInput(parser.positionalArguments().at(0), QDir::currentPath());

    bool fullScreen = parser.isSet(fullOption);
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
#if defined(Q_OS_MACOS) // On macOS, request a core profile context in the unlikely case of using OpenGL
    format.setVersion(4, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);
#elif defined(Q_PROCESSOR_ARM)
    fullScreen = true;
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setVersion(3, 1);
#else
    if (parser.isSet(glesOption))
        format.setRenderableType(QSurfaceFormat::OpenGLES);
    if (format.renderableType() == QSurfaceFormat::OpenGLES ||
            QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
        format.setVersion(3, 1);
    } else {
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
    }
#endif
    QSurfaceFormat::setDefaultFormat(format);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QQuickView view;
    auto engine = view.engine();
    auto context = engine->rootContext();
    context->setContextProperty(QStringLiteral("appDebugOpenGL"), parser.isSet(debugOption));
    context->setContextProperty(QStringLiteral("appSourceUrl"), sourceUrl);
    QObject::connect(engine, &QQmlEngine::quit, &view, &QQuickView::close);

    view.setSurfaceType(QSurface::OpenGLSurface);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setMinimumSize(QSize(640, 360));
    view.setVisibility(QWindow::AutomaticVisibility);
    int scrIdx = parser.value(outputOption).toUInt();
    if (scrIdx >= 0 && scrIdx < screenList.size())
        view.setScreen(screenList.at(scrIdx));
    if (fullScreen) {
        auto geometry = view.screen()->geometry();
        view.setX(geometry.x());
        view.setY(geometry.y());
        view.resize(geometry.width(), geometry.height());
    } else if (parser.isSet(debugOption)) {
        view.resize(QSize(560, 996)); // imitation of portrait orientation (vertical screen)
    } else {
        view.resize(QSize(1280, 720)); // HD-ready resolution by default
    }

    QObject::connect(&view, &QQuickView::statusChanged, &app, [&view,fullScreen](QQuickView::Status st) {
        if (st == QQuickView::Error) QCoreApplication::exit(-1);
        else if (st == QQuickView::Ready) {
            if (!fullScreen) view.show();
            else view.showFullScreen();
        }
    }, Qt::QueuedConnection);

    view.setSource(QUrl("qrc:///PanoramaPlayer/Main.qml"));
    return app.exec();
}

#include <QtGui/QApplication>
#include <QtDeclarative>
#include <QtWebKit/QWebSettings>
#include "qmlapplicationviewer.h"
#include "src/utility.h"
#include "src/tbnetworkaccessmanagerfactory.h"
#include "src/downloader.h"
#include "src/httpuploader.h"
#include "src/audiorecorder.h"
#include "src/scribblearea.h"
#include "src/qwebviewitem.h"
#include "src/imageuploader.h"

#ifdef Q_WS_SIMULATOR
#include <QtNetwork/QNetworkProxy>
#endif

#ifdef QVIBRA
#include "QVibra/qvibra.h"
#endif

#ifdef Q_OS_HARMATTAN
#include <QtDBus/QDBusConnection>
#include "src/tbclientif.h"
#include "src/harmattanbackgroundprovider.h"
#endif

#ifdef Q_OS_SYMBIAN
#include <QSymbianEvent>
#include <w32std.h>
#include <avkon.hrh>

class SymbianApplication : public QApplication
{
public:
    SymbianApplication(int &argc, char** argv) : QApplication(argc, argv){}

protected:
    bool symbianEventFilter(const QSymbianEvent *event)
    {
        if (event->type() == QSymbianEvent::WindowServerEvent
                && event->windowServerEvent()->Type() == KAknUidValueEndKeyCloseEvent){
            return true;
        }
        return QApplication::symbianEventFilter(event);
    }
};

#endif  // Q_OS_SYMBIAN

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    // Symbian specific
#ifdef Q_OS_SYMBIAN
    QApplication::setAttribute(Qt::AA_CaptureMultimediaKeys);
    QScopedPointer<QApplication> app(new SymbianApplication(argc, argv));
#else
    QScopedPointer<QApplication> app(createApplication(argc, argv));
#endif

#if defined(Q_OS_SYMBIAN)||defined(Q_WS_SIMULATOR)
    QSplashScreen *splash = new QSplashScreen(QPixmap("qml/tbclient/gfx/splash.jpg"));
    splash->show();
    splash->raise();
#endif

    app->setApplicationName("tbclient");
    app->setOrganizationName("Yeatse");
    app->setApplicationVersion(VER);

    // Install translator for qt
    QString locale = QLocale::system().name();
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_"+locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app->installTranslator(&qtTranslator);
    QTranslator translator;
    if (translator.load(app->applicationName()+"_"+locale, ":/i18n/"))
        app->installTranslator(&translator);

    qmlRegisterUncreatableType<HttpPostField>("com.yeatse.tbclient", 1, 0, "HttpPostField", "Can't touch this");
    qmlRegisterType<HttpPostFieldValue>("com.yeatse.tbclient", 1, 0, "HttpPostFieldValue");
    qmlRegisterType<HttpPostFieldFile>("com.yeatse.tbclient", 1, 0, "HttpPostFieldFile");
    qmlRegisterType<HttpUploader>("com.yeatse.tbclient", 1, 0, "HttpUploader");

    qmlRegisterType<Downloader>("com.yeatse.tbclient", 1, 0, "Downloader");
    qmlRegisterType<AudioRecorder>("com.yeatse.tbclient", 1, 0, "AudioRecorder");
    qmlRegisterType<ScribbleArea>("com.yeatse.tbclient", 1, 0, "ScribbleArea");
    qmlRegisterType<QWebViewItem>("com.yeatse.tbclient", 1, 0, "WebView");
    qmlRegisterType<ImageUploader>("com.yeatse.tbclient", 1, 0, "ImageUploader");

#ifdef QVIBRA
    qmlRegisterType<QVibra>("com.yeatse.tbclient", 1, 0, "Vibra");
#elif defined(Q_WS_SIMULATOR)
    qmlRegisterType<QObject>("com.yeatse.tbclient", 1, 0, "Vibra");
#endif

#ifdef Q_OS_HARMATTAN
    QWebSettings::globalSettings()->setUserStyleSheetUrl(QUrl::fromLocalFile("/opt/tbclient/qml/js/default_theme.css"));
#else
    QWebSettings::globalSettings()->setUserStyleSheetUrl(QUrl::fromLocalFile("qml/js/default_theme.css"));
#endif

    QmlApplicationViewer viewer;
    viewer.setAttribute(Qt::WA_OpaquePaintEvent);
    viewer.setAttribute(Qt::WA_NoSystemBackground);
    viewer.viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewer.viewport()->setAttribute(Qt::WA_NoSystemBackground);

#ifdef Q_OS_HARMATTAN
    new TBClientIf(app.data(), &viewer);
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerService("com.tbclient");
    bus.registerObject("/com/tbclient", app.data());
#endif

    // For fiddler network debugging
#ifdef Q_WS_SIMULATOR
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName("localhost");
    proxy.setPort(8888);
    QNetworkProxy::setApplicationProxy(proxy);
#endif

    TBNetworkAccessManagerFactory factory;
    viewer.engine()->setNetworkAccessManagerFactory(&factory);

    Utility* utility = Utility::Instance();
    utility->setEngine(viewer.engine());
    viewer.rootContext()->setContextProperty("utility", utility);

#ifdef Q_OS_HARMATTAN
    HarmattanBackgroundProvider provider;
    viewer.engine()->addImageProvider("bgProvider", &provider);
#endif

    // For smoother flicking (only supported by Belle FP2)
    if (utility->qtVersion() > 0x040800)
        QApplication::setStartDragDistance(2);

    // Initialize settings
    if (!utility->getValue("AppVersion","").toString().startsWith("2.1")){
        utility->clearSettings();
        utility->setValue("AppVersion", VER);
    }

#ifdef Q_OS_SYMBIAN
    viewer.setMainQmlFile(QLatin1String("qml/tbclient/main.qml"));
#elif defined(Q_OS_HARMATTAN)
    viewer.setMainQmlFile(QLatin1String("qml/harmattan/main.qml"));
#else
    viewer.setMainQmlFile(QLatin1String("qml/tbclient/main.qml"));
#endif
    viewer.showExpanded();

#if defined(Q_OS_SYMBIAN)||defined(Q_WS_SIMULATOR)
    splash->finish(&viewer);
    splash->deleteLater();
#endif

    return app->exec();
}

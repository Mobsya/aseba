#include "launcher.h"
#include <QStandardPaths>
#include <QGuiApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QTimer>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QStyle>
#include <QScreen>
#include <QFileDialog>
#include <QPointer>
namespace mobsya {

Launcher::Launcher(QObject* parent) : QObject(parent) {}


class QuickView: public QQuickView {
public:
	using QQuickView::QQuickView;
public:
	virtual ~QuickView() override;
	bool event(QEvent *event) override {
		if (event->type() == QEvent::Close) {
			QTimer::singleShot(1000, [ptr = QPointer(this)] () {
				if(!ptr.isNull())
					ptr->deleteLater();
			});
		}
		return QQuickView::event(event);
	}
};

QuickView::~QuickView() {}

bool Launcher::platformIsOsX() const {
#ifdef Q_OS_MACOS
	return true;
#else
	return false;

#endif
}

#ifdef Q_OS_MACOS
bool Launcher::launchOsXBundle(const QString& name, const QStringList & args) const {
	return doLaunchOsXBundle(name, args);
}
#endif

QString Launcher::search_program(const QString& name) const {
	qDebug() << "Searching for " << name;
	auto path = QStandardPaths::findExecutable(name, applicationsSearchPaths());
	if(path.isEmpty()) {
		qDebug() << "Not found, search in standard locations";
		path = QStandardPaths::findExecutable(name);
	}
	if(path.isEmpty()) {
		qDebug() << name << " not found";
		return {};
	}
	qDebug() << name << "found: " << path;
	return path;
}

QStringList Launcher::applicationsSearchPaths() const {
	return {QCoreApplication::applicationDirPath()};
}

QStringList Launcher::webappsFolderSearchPaths() const {
	QStringList files{QFileInfo(QCoreApplication::applicationDirPath()).absolutePath()};
#ifdef Q_OS_LINUX
	files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/../share/").absolutePath());
#endif
#ifdef Q_OS_OSX
	files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/").absolutePath());
#endif
	return files;
}

bool Launcher::launch_process(const QString& program, const QStringList& args) const {
	return QProcess::startDetached(program, args);
}

bool Launcher::openUrl(const QUrl& url) const {

	qDebug() << url;
	QuickView* w = new QuickView;
	w->rootContext()->setContextProperty("Utils", (QObject*)this);
	w->rootContext()->setContextProperty("appUrl", url);
#ifdef MOBSYA_USE_WEBENGINE
	w->setSource(QUrl("qrc:/qml/webview.qml"));
#else
	 w->setSource(QUrl("qrc:/qml/webview_native.qml"));
#endif
	w->rootContext()->setContextProperty("window", w);
	w->setWidth(1024);
	w->setHeight(700);
	const auto screen_geom = w->screen()->availableGeometry();
	w->setGeometry((screen_geom.width() - w->width()) / 2, (screen_geom.height() - w->height()) / 2,
						w->width(), w->height());
	w->show();

	return true;
}

QString Launcher::getDownloadPath(const QUrl& url) {
	QFileDialog d;
	const auto name = url.fileName();
	const auto extension = QFileInfo(name).suffix();
	d.setWindowTitle(tr("Save %1").arg(name));
	d.setNameFilter(QString("%1 (*.%1)").arg(extension));
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	d.setDefaultSuffix(extension);

	if(!d.exec())
		return {};
	const auto files = d.selectedFiles();
	if(files.size() != 1)
		return {};
	return files.first();
}

QUrl Launcher::webapp_base_url(const QString& name) const {
	static const std::map<QString, QString> default_folder_name = {{"blockly", "thymio_blockly"},
																   {"scratch", "scratch"}};
	auto it = default_folder_name.find(name);
	if(it == default_folder_name.end())
		return {};
	for(auto dirpath : webappsFolderSearchPaths()) {
		QFileInfo d(dirpath + "/" + it->second);
		if(d.exists()) {
			auto q = QUrl::fromLocalFile(d.absoluteFilePath());
			return q;
		}
	}
	return {};
}

QByteArray Launcher::readFileContent(QString path) {
	path.replace(QStringLiteral("qrc:/"), QStringLiteral(":/"));
	QFile f(path);
	f.open(QFile::ReadOnly);
	return f.readAll();
}

}  // namespace mobsya

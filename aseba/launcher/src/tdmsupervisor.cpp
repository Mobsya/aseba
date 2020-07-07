#include "tdmsupervisor.h"
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <errno.h>
#include <QThread>

namespace mobsya {

static const auto tdm_program_name = QByteArrayLiteral("thymio-device-manager");
static const auto max_launch_count = 10;

TDMSupervisor::TDMSupervisor(const Launcher& launcher, QObject* parent)
    : QObject(parent), m_launcher(launcher), m_tdm_process(nullptr), m_launches(0) {}


TDMSupervisor::~TDMSupervisor() {}

void TDMSupervisor::startLocalTDM() {
#ifndef Q_OS_IOS
    if(m_tdm_process != nullptr)
        return;

    if(m_launches++ >= max_launch_count) {
        qCritical("thymio-device-manager Relaunched too many times");
        return;
    }

    const auto path = m_launcher.search_program(tdm_program_name);
    if(path.isEmpty()) {
        qCritical("thymio-device-manager not found");
        Q_EMIT error();
        return;
    }

    m_tdm_process = new QProcess(this);
	QThread::msleep(1000);
    connect(m_tdm_process, &QProcess::stateChanged, [this](QProcess::ProcessState state) {
        switch(state) {
            case QProcess::NotRunning: {
                qInfo("thymio-device-manager stopped");
                m_tdm_process->deleteLater();
                m_tdm_process = nullptr;
                break;
            }
            case QProcess::Starting: {
                qInfo("thymio-device-manager starting");
                break;
            }
            case QProcess::Running: {
                qInfo("thymio-device-manager started");
                break;
            }
        }
    });
    connect(m_tdm_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if(exitStatus == QProcess::CrashExit) {
                    qCritical("Thymio device manager crashed, relaunching");
                    QTimer::singleShot(1000, this, &TDMSupervisor::startLocalTDM);
                } else {
                    if(exitCode == EALREADY) {
                        qInfo("thymio-device-manager already launched");
                    }
                    qInfo("thymio-device-manager stopped with exit code %d", exitCode);
                }
            });
    // The empty argument list QStringList{} is necessary to make sure Qt will not try
    // to split the path if it contains space
	QThread::msleep(1000);
    m_tdm_process->start(path, QStringList{});
#endif
}

}  // namespace mobsya

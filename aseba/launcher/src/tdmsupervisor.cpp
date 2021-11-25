#include "tdmsupervisor.h"
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <errno.h>

namespace mobsya {

#ifdef Q_OS_OSX
static const auto tdm_program_name = QByteArrayLiteral("../Helpers/thymio-device-manager");
#else
static const auto tdm_program_name = QByteArrayLiteral("thymio-device-manager");
#endif
static const auto max_launch_count = 10;

TDMSupervisor::TDMSupervisor(const Launcher& launcher, QObject* parent)
    : QObject(parent), m_launcher(launcher), m_tdm_process(nullptr), m_launches(0) {

    // When the configuration for remote connection changes,
    // We want to restart the TDM
    // This will stop all existing connections
    connect(&launcher,
            &mobsya::Launcher::remoteConnectionsAllowedChanged, this, &TDMSupervisor::restart);

}


TDMSupervisor::~TDMSupervisor() {}

void TDMSupervisor::startLocalTDM() {
#ifndef Q_OS_IOS
    if(m_tdm_process != nullptr) {
        return;
        m_restart = false;
    }

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
    connect(m_tdm_process, &QProcess::stateChanged, [this](QProcess::ProcessState state) {
        switch(state) {
            case QProcess::NotRunning: {
                qInfo("thymio-device-manager stopped");
                m_tdm_process->deleteLater();
                m_tdm_process = nullptr;
                if(m_restart) {
                     QTimer::singleShot(500, this, &TDMSupervisor::startLocalTDM);
                }
                break;
            }
            case QProcess::Starting: {
                qInfo("thymio-device-manager starting");
                break;
            }
            case QProcess::Running: {
                qInfo("thymio-device-manager started");
                QTimer::singleShot(500, this, &TDMSupervisor::started);
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
    QStringList args;
    if(m_launcher.getAllowRemoteConnections()) {
        args.append("--allow-remote-connections");
    }
    m_tdm_process->setReadChannelMode(QProcess::ForwardedOutputChannel);
    m_tdm_process->start(path, args);
    m_restart = false;
#endif
}

void TDMSupervisor::restart() {
#ifndef Q_OS_IOS
    if(m_restart)
        return;

    m_restart = true;
    m_launches = 0;
    if(m_tdm_process) {
        if(m_launcher.client()) {
            m_launcher.client()->requestDeviceManagersShutdown();
            QTimer::singleShot(500, [&] {
                if(m_tdm_process)
                    m_tdm_process->kill();
                startLocalTDM();
            });
            return;
        }
        m_tdm_process->kill();
        return;
    }
    QTimer::singleShot(100, [&] {
        startLocalTDM();
    });
 #endif
}

}  // namespace mobsya

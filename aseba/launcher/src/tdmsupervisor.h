#pragma once
#include "launcher.h"

class QProcess;

namespace mobsya {

class TDMSupervisor : public QObject {
    Q_OBJECT
public:
    TDMSupervisor(const Launcher& launcher, QObject* parent = nullptr);
    ~TDMSupervisor();

public Q_SLOTS:

    void startLocalTDM();

Q_SIGNALS:

    void statusChanged();
    void error();
    void started();

private:
    const Launcher& m_launcher;
    QProcess* m_tdm_process;
    quint8 m_launches;
};

}  // namespace mobsya

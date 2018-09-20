#pragma once

#include <QObject>

namespace mobsya {

class Launcher : public QObject {
    Q_OBJECT
public:
    Launcher(QObject* parent = nullptr);
    Q_INVOKABLE QString search_program(const QString& name) const;

private:
    QStringList applicationsSearchPaths() const;
};


}  // namespace mobsya
#ifndef THYMIO2WIRELESSDONGLE_H
#define THYMIO2WIRELESSDONGLE_H

#include <QObject>

class Thymio2WirelessDongle : public QObject
{
    Q_OBJECT
public:
    explicit Thymio2WirelessDongle(QObject *parent = nullptr);

signals:

public slots:
};

#endif // THYMIO2WIRELESSDONGLE_H

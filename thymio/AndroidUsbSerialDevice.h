#pragma once
#include <QIODevice>

namespace mobsya {
class AndroidUsbSerialDevice : public QIODevice {
public:
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;
    bool canReadLine() const override;
    void close() override;
    bool open(OpenMode mode) override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;
};
}    // namespace mobsya

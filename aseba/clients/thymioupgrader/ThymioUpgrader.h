#ifndef THYMIO_FLASHER_H
#define THYMIO_FLASHER_H

#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QTemporaryFile>
#include <utility>
#include <dashel/dashel.h>
#include "common/utils/BootloaderInterface.h"

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QLineEdit;
class QListWidget;
class QGroupBox;
class QNetworkAccessManager;
class QNetworkReply;

namespace Aseba {
/** \addtogroup thymioupdater */
/*@{*/

class MessageHub;

class QtBootloaderInterface : public QObject, public BootloaderInterface {
    Q_OBJECT

public:
    QtBootloaderInterface(Dashel::Stream* stream, int dest, int bootloaderDest);

protected:
    void writeHexGotDescription(unsigned pagesCount) override;
    void writePageStart(unsigned pageNumber, const uint8_t* data, bool simple) override;
    void errorWritePageNonFatal(unsigned pageNumber) override;

protected:
    unsigned pagesCount;
    unsigned pagesDoneCount;

signals:
    void flashProgress(int percentage);
};

class ThymioUpgraderDialog : public QWidget {
    Q_OBJECT

private:
    struct FlashResult {
        enum Status { SUCCESS = 0, WARNING, FAILURE } status;
        QString title;
        QString text;

        FlashResult() : status(SUCCESS) {}
        FlashResult(Status status, QString title, QString text)
            : status(status), title(std::move(title)), text(std::move(text)) {}
    };

private:
    std::string target;

    unsigned currentVersion;
    unsigned currentDevStatus;
    unsigned officialVersion;
    unsigned officialDevStatus;
    QTemporaryFile officialHexFile;

    QGroupBox* officialGroupBox;
    QGroupBox* fileGroupBox;
    QVBoxLayout* mainLayout;
    QHBoxLayout* fileLayout;
    QHBoxLayout* flashLayout;
    QLabel* officialFirmwareText;
    QLabel* firmwareVersion;
    QLabel* nodeIdText;
    QLineEdit* lineEdit;
    QPushButton* fileButton;
    QProgressBar* progressBar;
    QPushButton* flashButton;
    QPushButton* quitButton;
    QFuture<FlashResult> flashFuture;
    QFutureWatcher<FlashResult> flashFutureWatcher;
    QNetworkAccessManager* networkManager;

public:
    ThymioUpgraderDialog(std::string target);
    ~ThymioUpgraderDialog() override;

private:
    unsigned readId(MessageHub& hub, Dashel::Stream* stream) const;
    void readIdVersion();
    FlashResult flashThread(const std::string& _target, const std::string& hexFileName) const;
    void networkError();
    QString versionDevStatusToString(unsigned version, unsigned devStatus) const;
    void selectOfficialFirmware();
    void selectUserFirmware();

private slots:
    void officialGroupChecked(bool on);
    void fileGroupChecked(bool on);
    void setupFlashButtonState();
    void openFile();
    void doFlash();
    void flashProgress(int percentage);
    void flashFinished();
    void networkReplyFinished(QNetworkReply*);
};

/*@}*/
}  // namespace Aseba

#endif  // THYMIO_FLASHER_H

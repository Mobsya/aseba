#include <QQuickWidget>

namespace mobsya {

class LauncherWindow : public QQuickWidget {
    Q_OBJECT
public:
    using QQuickWidget::QQuickWidget;
Q_SIGNALS:
    void closingRequested();


protected:
    void closeEvent(QCloseEvent* event) override {
        event->accept();
        Q_EMIT closingRequested();
    }
};

}  // namespace mobsya
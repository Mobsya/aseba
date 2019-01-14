#include <QtWidgets>

namespace Aseba {

class LockButton : public QAbstractButton {
    Q_OBJECT
public:
    LockButton(QWidget* parent);

protected:
    QSize sizeHint() const override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent*) override;

Q_SIGNALS:
    void locked();
    void unlocked();

public Q_SLOTS:
    void setLocked();
    void setUnlocked();
    void setUnAvailable();

private:
    enum {
        Locked,
        Unlocked,
        Unavailable,
    } m_state = Unavailable;

    QIcon m_lockedIcon;
    QIcon m_unlockedIcon;
};

};  // namespace Aseba

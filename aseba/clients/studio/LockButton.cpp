#include "LockButton.h"

namespace Aseba {

LockButton::LockButton(QWidget* parent) : QAbstractButton(parent) {
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Lock Thymio"));
    resize(sizeHint());

    m_lockedIcon = QIcon(":/images/locked");
    m_unlockedIcon = QIcon(":/images/unlocked");
}


void LockButton::setLocked() {
    m_state = Locked;
}

void LockButton::setUnlocked() {
    m_state = Unlocked;
}

void LockButton::setUnAvailable() {
    m_state = Unavailable;
}

QSize LockButton::sizeHint() const {
    ensurePolished();
    int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
    int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
    return QSize(width, height);
}

void LockButton::enterEvent(QEvent* event) {
    if(isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void LockButton::leaveEvent(QEvent* event) {
    if(isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void LockButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    QStyleOption opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;
    if(isEnabled() && underMouse() && !isChecked() && !isDown())
        opt.state |= QStyle::State_Raised;
    if(isChecked())
        opt.state |= QStyle::State_On;
    if(isDown())
        opt.state |= QStyle::State_Sunken;

    if(const QTabBar* tb = qobject_cast<const QTabBar*>(parent())) {
        int index = tb->currentIndex();
        QTabBar::ButtonPosition position =
            (QTabBar::ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, tb);
        if(tb->tabButton(index, position) == this)
            opt.state |= QStyle::State_Selected;
    }

    QPixmap icon;
    switch(m_state) {
        case Locked: {
            icon = m_lockedIcon.pixmap(opt.rect.size(), QIcon::Normal);
            break;
        }
        case Unlocked: {
            icon = m_unlockedIcon.pixmap(opt.rect.size(), QIcon::Normal);
            break;
        }
        case Unavailable: {
            icon = m_lockedIcon.pixmap(opt.rect.size(), QIcon::Disabled);
            break;
        }
    }

    style()->drawItemPixmap(&p, opt.rect, Qt::AlignHCenter | Qt::AlignVCenter, icon);
}

}  // namespace Aseba

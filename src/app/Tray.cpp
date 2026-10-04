#include "Tray.h"

#include "Controller.h"

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

Tray::Tray(Controller* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
{
    m_statusAction = m_menu.addAction(QString());
    m_statusAction->setEnabled(false);
    m_permissionAction = m_menu.addAction(QStringLiteral("Разрешить доступ к клавиатуре…"), m_controller,
                                          &Controller::requestAccessibility);
    m_menu.addSeparator();

    m_menu.addAction(QStringLiteral("Настройки…"), m_controller, &Controller::openSettings);
    m_pauseAction = m_menu.addAction(QStringLiteral("Пауза"));
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::triggered, m_controller, &Controller::setPaused);

    m_loginAction = m_menu.addAction(QStringLiteral("Запускать при входе"));
    m_loginAction->setCheckable(true);
    connect(m_loginAction, &QAction::triggered, m_controller, &Controller::setLaunchAtLogin);

    m_menu.addSeparator();
    m_menu.addAction(QStringLiteral("Выйти из Leap"), qApp, &QCoreApplication::quit);

    connect(&m_menu, &QMenu::aboutToShow, this, &Tray::sync);
    connect(m_controller, &Controller::statusChanged, this, &Tray::sync);
    connect(m_controller, &Controller::settingsChanged, this, &Tray::sync);
    connect(m_controller, &Controller::actionFailed, this, &Tray::notify);

    m_tray.setToolTip(QStringLiteral("Leap"));
    m_tray.setContextMenu(&m_menu);
    sync();
    m_tray.show();
}

void Tray::notify(const QString& message)
{
    m_tray.showMessage(QStringLiteral("Leap"), message, QSystemTrayIcon::Warning, 4000);
}

void Tray::sync()
{
    const bool granted = m_controller->accessibilityGranted();
    if (!granted)
        m_statusAction->setText(QStringLiteral("Нет доступа к клавиатуре"));
    else if (m_controller->paused())
        m_statusAction->setText(QStringLiteral("На паузе"));
    else if (!m_controller->listening())
        m_statusAction->setText(QStringLiteral("Запускается…"));
    else
        m_statusAction->setText(QStringLiteral("Ведущая клавиша: %1").arg(m_controller->leaderLabel()));
    m_permissionAction->setVisible(!granted);

    const QSignalBlocker blockPause(m_pauseAction);
    m_pauseAction->setChecked(m_controller->paused());
    const QSignalBlocker blockLogin(m_loginAction);
    m_loginAction->setChecked(m_controller->launchAtLogin());
    m_tray.setIcon(makeIcon(m_controller->paused() || !m_controller->listening()));
}

QIcon Tray::makeIcon(bool paused)
{
    // A key cap with an arrow leaping out of it, 18 pt @2x, as a template
    // image so macOS tints it for light and dark menu bars. Dimmed when
    // Leap isn't listening.
    constexpr qreal dpr = 2.0;
    QPixmap pm(QSize(18, 18) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QColor ink(0, 0, 0, paused ? 110 : 255);
    QPen pen(ink, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(1.8, 5.8, 10.4, 10.4), 2.6, 2.6);

    // Arc from the key cap up and to the right, with an arrowhead.
    QPainterPath arc;
    arc.moveTo(7.0, 9.6);
    arc.cubicTo(8.2, 4.4, 11.6, 2.4, 15.2, 2.9);
    p.drawPath(arc);
    QPainterPath head;
    head.moveTo(12.6, 1.2);
    head.lineTo(15.6, 2.9);
    head.lineTo(13.4, 5.6);
    p.drawPath(head);
    p.end();

    QIcon icon(pm);
    icon.setIsMask(true);
    return icon;
}

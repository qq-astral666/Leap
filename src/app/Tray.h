#pragma once

#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

class Controller;
class QAction;

// Menu bar icon: settings, pause, permission status, launch at login, quit.
class Tray : public QObject {
    Q_OBJECT

public:
    explicit Tray(Controller* controller, QObject* parent = nullptr);

    void notify(const QString& message);

private:
    static QIcon makeIcon(bool paused);
    void sync();

    Controller* m_controller;
    QSystemTrayIcon m_tray;
    QMenu m_menu;
    QAction* m_statusAction = nullptr;
    QAction* m_permissionAction = nullptr;
    QAction* m_pauseAction = nullptr;
    QAction* m_loginAction = nullptr;
};

#include "CapsLockRemap.h"

#include "HidMapping.h"

#include <QProcess>

namespace capslock {

namespace {

bool hidutil(const QStringList& args, QByteArray* output, QString* error)
{
    QProcess p;
    p.start(QStringLiteral("/usr/bin/hidutil"), args);
    if (!p.waitForFinished(3000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (error)
            *error = QStringLiteral("hidutil: %1").arg(
                p.error() == QProcess::UnknownError ? QString::fromUtf8(p.readAllStandardError().trimmed())
                                                    : p.errorString());
        return false;
    }
    if (output)
        *output = p.readAllStandardOutput();
    return true;
}

} // namespace

bool setRemapped(bool enable, QString* error)
{
#ifdef Q_OS_MACOS
    QByteArray current;
    if (!hidutil({ QStringLiteral("property"), QStringLiteral("--get"), QStringLiteral("UserKeyMapping") }, &current,
                 error))
        return false;
    const auto before = leap::parseHidutilMappings(current.toStdString());
    const auto after = leap::withCapsLockRemap(before, enable);
    if (after == before)
        return true;
    const QString json = QString::fromStdString(leap::hidutilSetJson(after));
    return hidutil({ QStringLiteral("property"), QStringLiteral("--set"), json }, nullptr, error);
#else
    Q_UNUSED(enable);
    Q_UNUSED(error);
    return true;
#endif
}

} // namespace capslock

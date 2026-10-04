#pragma once

#include <QString>

// Caps Lock -> F18 through /usr/bin/hidutil (see core/HidMapping.h). No
// permission needed; lasts until reboot or until Leap turns it off on quit.
namespace capslock {

bool setRemapped(bool enable, QString* error);

} // namespace capslock

// Compiled as Objective-C++ with ARC.

#include "Native.h"

#include "Layout.h"

#include <QFileInfo>
#include <QWindow>

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ServiceManagement/ServiceManagement.h>

#include <algorithm>
#include <time.h>

namespace {

// Stamped into events Leap posts itself (⌘V, ⌃⌘Q) so the tap lets them by.
constexpr int64_t kLeapEventMarker = 0x4C454150; // 'LEAP'

CFMachPortRef g_tap = nullptr;
CFRunLoopSourceRef g_tapSource = nullptr;
native::KeyHandler g_handler;

NSWindow* nsWindowOf(QWindow* window)
{
    if (!window)
        return nil;
    NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
    return view.window;
}

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

std::uint32_t modsFrom(CGEventFlags f)
{
    std::uint32_t m = 0;
    if (f & kCGEventFlagMaskShift)
        m |= leap::ModShift;
    if (f & kCGEventFlagMaskControl)
        m |= leap::ModControl;
    if (f & kCGEventFlagMaskAlternate)
        m |= leap::ModOption;
    if (f & kCGEventFlagMaskCommand)
        m |= leap::ModCommand;
    return m;
}

CGEventRef tapCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void*)
{
    // macOS switches a tap off if a callback takes too long; switch it back on.
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (g_tap)
            CGEventTapEnable(g_tap, true);
        return event;
    }
    if (!g_handler || CGEventGetIntegerValueField(event, kCGEventSourceUserData) == kLeapEventMarker)
        return event;

    leap::KeyEvent e;
    switch (type) {
    case kCGEventKeyDown:
        e.kind = leap::KeyEvent::Kind::Down;
        break;
    case kCGEventKeyUp:
        e.kind = leap::KeyEvent::Kind::Up;
        break;
    case kCGEventFlagsChanged:
        e.kind = leap::KeyEvent::Kind::Flags;
        break;
    default:
        return event;
    }
    e.code = static_cast<int>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
    e.mods = modsFrom(CGEventGetFlags(event));
    e.repeat = CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat) != 0;
    e.time = static_cast<double>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1e9;
    return g_handler(e) ? nullptr : event;
}

void postKey(CGKeyCode code, CGEventFlags flags)
{
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStatePrivate);
    for (const bool down : { true, false }) {
        CGEventRef ev = CGEventCreateKeyboardEvent(source, code, down);
        CGEventSetFlags(ev, flags);
        CGEventSetIntegerValueField(ev, kCGEventSourceUserData, kLeapEventMarker);
        CGEventPost(kCGHIDEventTap, ev);
        CFRelease(ev);
    }
    if (source)
        CFRelease(source);
}

bool runAppleScript(NSString* source, QString* error)
{
    NSAppleScript* script = [[NSAppleScript alloc] initWithSource:source];
    NSDictionary* info = nil;
    [script executeAndReturnError:&info];
    if (info) {
        NSString* message = info[NSAppleScriptErrorMessage] ?: @"AppleScript error";
        setError(error, QString::fromNSString(message));
        return false;
    }
    return true;
}

// ---- Accessibility geometry: global, y down from the primary screen's top.

leap::Rect axRect(NSRect r, CGFloat primaryHeight)
{
    return { r.origin.x, primaryHeight - r.origin.y - r.size.height, r.size.width, r.size.height };
}

bool readFrame(AXUIElementRef window, leap::Rect* out)
{
    CFTypeRef posValue = nullptr;
    CFTypeRef sizeValue = nullptr;
    CGPoint pos {};
    CGSize size {};
    bool ok = AXUIElementCopyAttributeValue(window, kAXPositionAttribute, &posValue) == kAXErrorSuccess
        && AXUIElementCopyAttributeValue(window, kAXSizeAttribute, &sizeValue) == kAXErrorSuccess
        && AXValueGetValue((AXValueRef)posValue, kAXValueTypeCGPoint, &pos)
        && AXValueGetValue((AXValueRef)sizeValue, kAXValueTypeCGSize, &size);
    if (posValue)
        CFRelease(posValue);
    if (sizeValue)
        CFRelease(sizeValue);
    if (ok)
        *out = { pos.x, pos.y, size.width, size.height };
    return ok;
}

void writeFrame(AXUIElementRef window, const leap::Rect& r)
{
    CGPoint pos = CGPointMake(r.x, r.y);
    CGSize size = CGSizeMake(r.w, r.h);
    AXValueRef posValue = AXValueCreate(kAXValueTypeCGPoint, &pos);
    AXValueRef sizeValue = AXValueCreate(kAXValueTypeCGSize, &size);
    // Size, move, size again: moving to another display can clamp the size,
    // and a big window can't move where it doesn't fit yet.
    AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeValue);
    AXUIElementSetAttributeValue(window, kAXPositionAttribute, posValue);
    AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeValue);
    CFRelease(posValue);
    CFRelease(sizeValue);
}

} // namespace

namespace native {

// ------------------------------------------------------------ permissions

bool accessibilityTrusted()
{
    return AXIsProcessTrusted();
}

void requestAccessibility()
{
    NSDictionary* options = @{ (__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES };
    AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
}

void openAccessibilitySettings()
{
    NSURL* url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
    [[NSWorkspace sharedWorkspace] openURL:url];
}

// ------------------------------------------------------------ keyboard

bool startKeyTap(KeyHandler handler)
{
    g_handler = std::move(handler);
    if (g_tap)
        return true;
    if (!AXIsProcessTrusted())
        return false;

    const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp)
        | CGEventMaskBit(kCGEventFlagsChanged);
    g_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, mask,
                             tapCallback, nullptr);
    if (!g_tap)
        return false;
    g_tapSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), g_tapSource, kCFRunLoopCommonModes);
    CGEventTapEnable(g_tap, true);
    return true;
}

void stopKeyTap()
{
    if (!g_tap)
        return;
    CGEventTapEnable(g_tap, false);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), g_tapSource, kCFRunLoopCommonModes);
    CFRelease(g_tapSource);
    CFMachPortInvalidate(g_tap);
    CFRelease(g_tap);
    g_tapSource = nullptr;
    g_tap = nullptr;
}

bool keyTapRunning()
{
    return g_tap && CGEventTapIsEnabled(g_tap);
}

bool secureInputActive()
{
    return IsSecureEventInputEnabled();
}

// ------------------------------------------------------------ overlay

void configureOverlay(QWindow* window)
{
    NSWindow* w = nsWindowOf(window);
    if (!w)
        return;
    if ([w isKindOfClass:[NSPanel class]]) {
        NSPanel* panel = (NSPanel*)w;
        panel.styleMask = panel.styleMask | NSWindowStyleMaskNonactivatingPanel;
        panel.becomesKeyOnlyIfNeeded = YES;
        panel.floatingPanel = YES;
    }
    w.level = NSPopUpMenuWindowLevel;
    w.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
                         | NSWindowCollectionBehaviorFullScreenAuxiliary
                         | NSWindowCollectionBehaviorStationary
                         | NSWindowCollectionBehaviorIgnoresCycle;
    w.hidesOnDeactivate = NO; // Qt sets YES for tool windows
    w.hasShadow = NO;
    w.opaque = NO;
    w.backgroundColor = NSColor.clearColor;
    w.ignoresMouseEvents = YES;
    w.animationBehavior = NSWindowAnimationBehaviorNone;
}

void orderOverlay(QWindow* window, bool shown)
{
    NSWindow* w = nsWindowOf(window);
    if (!w)
        return;
    if (shown)
        [w orderFrontRegardless];
    else
        [w orderOut:nil];
}

void activateApp()
{
    if (@available(macOS 14.0, *))
        [NSApp activate];
    else
        [NSApp activateIgnoringOtherApps:YES];
}

// ------------------------------------------------------------ actions

bool openApp(const QString& bundlePath, QString* error)
{
    NSString* path = bundlePath.toNSString();
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        setError(error, QStringLiteral("Приложение не найдено: %1").arg(bundlePath));
        return false;
    }
    NSURL* url = [NSURL fileURLWithPath:path];
    NSWorkspaceOpenConfiguration* config = [NSWorkspaceOpenConfiguration configuration];
    config.activates = YES;
    // A running app gets a "reopen" event, like a click on its Dock icon:
    // it comes forward and opens a window if it had none.
    [[NSWorkspace sharedWorkspace] openApplicationAtURL:url
                                          configuration:config
                                      completionHandler:^(NSRunningApplication* app, NSError*) {
                                          dispatch_async(dispatch_get_main_queue(), ^{
                                              [app activateWithOptions:NSApplicationActivateAllWindows];
                                          });
                                      }];
    return true;
}

bool openWithDefaultApp(const QString& pathOrUrl, bool isUrl, QString* error)
{
    NSURL* url = nil;
    if (isUrl) {
        url = [NSURL URLWithString:pathOrUrl.toNSString()];
    } else {
        if (!QFileInfo::exists(pathOrUrl)) {
            setError(error, QStringLiteral("Не найдено: %1").arg(pathOrUrl));
            return false;
        }
        url = [NSURL fileURLWithPath:pathOrUrl.toNSString()];
    }
    if (!url || ![[NSWorkspace sharedWorkspace] openURL:url]) {
        setError(error, QStringLiteral("Не удалось открыть: %1").arg(pathOrUrl));
        return false;
    }
    return true;
}

void postPaste()
{
    postKey(static_cast<CGKeyCode>(leap::key::V), kCGEventFlagMaskCommand);
}

bool runWindowCommand(const QString& command, double gap, QString* error)
{
    if (!AXIsProcessTrusted()) {
        setError(error, QStringLiteral("Нужен доступ к универсальному доступу"));
        return false;
    }
    NSRunningApplication* front = NSWorkspace.sharedWorkspace.frontmostApplication;
    if (!front) {
        setError(error, QStringLiteral("Нет активного приложения"));
        return false;
    }
    AXUIElementRef app = AXUIElementCreateApplication(front.processIdentifier);
    AXUIElementRef window = nullptr;
    const AXError axError = AXUIElementCopyAttributeValue(app, kAXFocusedWindowAttribute, (CFTypeRef*)&window);
    if (axError != kAXErrorSuccess || !window) {
        CFRelease(app);
        setError(error, QStringLiteral("У приложения нет активного окна"));
        return false;
    }

    bool ok = false;
    leap::Rect current;
    NSArray<NSScreen*>* screens = NSScreen.screens;
    if (screens.count > 0 && readFrame(window, &current)) {
        const CGFloat primaryHeight = screens.firstObject.frame.size.height;
        std::vector<leap::Rect> frames;
        std::vector<leap::Rect> visible;
        for (NSScreen* s in screens) {
            frames.push_back(axRect(s.frame, primaryHeight));
            visible.push_back(axRect(s.visibleFrame, primaryHeight));
        }
        const int index = std::max(0, leap::screenIndexFor(current, frames));
        std::optional<leap::Rect> target;
        if (command == QLatin1String("next-display")) {
            if (screens.count > 1) {
                const int next = (index + 1) % static_cast<int>(screens.count);
                target = leap::moveToScreen(current, visible[static_cast<size_t>(index)],
                                            visible[static_cast<size_t>(next)]);
            } else {
                setError(error, QStringLiteral("Подключён только один монитор"));
            }
        } else {
            target = leap::layoutWindow(command.toStdString(), visible[static_cast<size_t>(index)], current, gap);
            if (!target)
                setError(error, QStringLiteral("Неизвестная команда: %1").arg(command));
        }

        if (target) {
            // Apps with "enhanced UI" on (set by VoiceOver-aware tools)
            // animate every change; switch it off for the move.
            CFTypeRef enhanced = nullptr;
            const bool hadEnhanced
                = AXUIElementCopyAttributeValue(app, CFSTR("AXEnhancedUserInterface"), &enhanced) == kAXErrorSuccess
                && enhanced && CFGetTypeID(enhanced) == CFBooleanGetTypeID()
                && CFBooleanGetValue((CFBooleanRef)enhanced);
            if (enhanced)
                CFRelease(enhanced);
            if (hadEnhanced)
                AXUIElementSetAttributeValue(app, CFSTR("AXEnhancedUserInterface"), kCFBooleanFalse);
            writeFrame(window, *target);
            if (hadEnhanced)
                AXUIElementSetAttributeValue(app, CFSTR("AXEnhancedUserInterface"), kCFBooleanTrue);
            ok = true;
        }
    } else {
        setError(error, QStringLiteral("Не удалось прочитать размер окна"));
    }
    CFRelease(window);
    CFRelease(app);
    return ok;
}

bool runSystemCommand(const QString& command, QString* error)
{
    if (command == QLatin1String("volume-up"))
        return runAppleScript(@"set volume output muted false\n"
                              @"set volume output volume ((output volume of (get volume settings)) + 6)",
                              error);
    if (command == QLatin1String("volume-down"))
        return runAppleScript(@"set volume output volume ((output volume of (get volume settings)) - 6)", error);
    if (command == QLatin1String("mute"))
        return runAppleScript(@"set volume output muted (not (output muted of (get volume settings)))", error);
    if (command == QLatin1String("dark-mode"))
        return runAppleScript(@"tell application \"System Events\" to tell appearance preferences "
                              @"to set dark mode to not dark mode",
                              error);
    if (command == QLatin1String("sleep-display")) {
        NSTask* task = [[NSTask alloc] init];
        task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/pmset"];
        task.arguments = @[ @"displaysleepnow" ];
        NSError* err = nil;
        if (![task launchAndReturnError:&err]) {
            setError(error, QString::fromNSString(err.localizedDescription));
            return false;
        }
        return true;
    }
    if (command == QLatin1String("lock")) {
        // The system shortcut ⌃⌘Q: no private API needed.
        postKey(static_cast<CGKeyCode>(leap::key::Q), kCGEventFlagMaskCommand | kCGEventFlagMaskControl);
        return true;
    }
    setError(error, QStringLiteral("Неизвестная команда: %1").arg(command));
    return false;
}

// ------------------------------------------------------------ misc

QImage fileIcon(const QString& path, int size)
{
    if (path.isEmpty() || size <= 0)
        return {};
    NSImage* icon = [[NSWorkspace sharedWorkspace] iconForFile:path.toNSString()];
    if (!icon)
        return {};
    QImage out(size, size, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(out.bits(), size, size, 8, out.bytesPerLine(), space,
                                             kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    CGColorSpaceRelease(space);
    if (!ctx)
        return {};
    NSGraphicsContext* gc = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:NO];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:gc];
    [icon drawInRect:NSMakeRect(0, 0, size, size)
            fromRect:NSZeroRect
           operation:NSCompositingOperationSourceOver
            fraction:1.0];
    [NSGraphicsContext restoreGraphicsState];
    CGContextRelease(ctx);
    return out;
}

QString frontmostAppPath()
{
    NSRunningApplication* front = NSWorkspace.sharedWorkspace.frontmostApplication;
    return front.bundleURL ? QString::fromNSString(front.bundleURL.path) : QString();
}

QString bundleDisplayName(const QString& bundlePath)
{
    NSBundle* bundle = [NSBundle bundleWithPath:bundlePath.toNSString()];
    NSString* name = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"]
        ?: [bundle objectForInfoDictionaryKey:@"CFBundleName"];
    if (name.length > 0)
        return QString::fromNSString(name);
    return QFileInfo(bundlePath).completeBaseName();
}

bool launchAtLoginEnabled()
{
    if (@available(macOS 13.0, *))
        return SMAppService.mainAppService.status == SMAppServiceStatusEnabled;
    return false;
}

bool setLaunchAtLogin(bool enable, QString* error)
{
    if (@available(macOS 13.0, *)) {
        SMAppService* service = SMAppService.mainAppService;
        NSError* err = nil;
        const BOOL ok = enable ? [service registerAndReturnError:&err] : [service unregisterAndReturnError:&err];
        if (!ok) {
            setError(error, QString::fromNSString(err.localizedDescription ?: @"unknown error"));
            return false;
        }
        if (enable && service.status == SMAppServiceStatusRequiresApproval)
            [SMAppService openSystemSettingsLoginItems];
        return true;
    }
    setError(error, QStringLiteral("Нужна macOS 13 или новее"));
    return false;
}

} // namespace native

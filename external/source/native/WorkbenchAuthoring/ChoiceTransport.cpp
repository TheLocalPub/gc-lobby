#include "ChoiceTransport.h"
#include <QCoreApplication>
#include <QThread>
#include <QTcpServer>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace gc {
namespace detail { bool ownsListener(quint16 port); }
namespace {
template<class T> T at(const void *object, size_t offset)
{
    return *reinterpret_cast<const T *>(static_cast<const char *>(object) + offset);
}
const void *setting(void *settings, unsigned index)
{
    const auto *table = at<const quintptr *>(settings, 0);
    const auto *value = reinterpret_cast<const void *(*)(void *, unsigned)>(table[0x128 / sizeof(quintptr)])(settings, index);
    if (value) return value;
    const auto *schema = at<const void *>(settings, 0x18);
    if (!schema || index >= at<unsigned>(schema, 0x34)) return nullptr;
    const auto *fields = at<const char *>(schema, 0x28);
    return fields ? at<const void *>(fields + index * 0x58, 0x10) : nullptr;
}
}

QString initializeChoiceTransport(quint16 &port)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread())
        return QStringLiteral("Choice transport requires the application thread");
    const auto image = reinterpret_cast<quintptr>(GetModuleHandleW(nullptr));
    auto *app = *reinterpret_cast<void **>(image + 0x3e881e8);
    if (!app || at<quintptr>(app, 0) != image + 0x2c1c2f8)
        return QStringLiteral("Unsupported Workbench application owner");
    auto *settings = at<void *>(app, 0x1608);
    if (!settings || at<quintptr>(settings, 0) != image + 0x32b7828)
        return QStringLiteral("Unsupported Workbench settings owner");
    const auto *enabled = static_cast<const bool *>(setting(settings, 8));
    const auto *selectedPort = static_cast<const int *>(setting(settings, 9));
    if (!enabled || !selectedPort || *selectedPort < 1 || *selectedPort > 65535)
        return QStringLiteral("Workbench NET API settings are unavailable");
    port = quint16(*selectedPort);
    if (!detail::ownsListener(port)) {
        QTcpServer probe;
        if (!probe.listen(QHostAddress::AnyIPv4, port))
            return QStringLiteral("Workbench NET API port %1 is already in use").arg(port);
        probe.close();
    }
    const bool changed = !*enabled;
    if (changed) {
        const auto *table = at<const quintptr *>(settings, 0);
        auto *value = reinterpret_cast<bool *(*)(void *, unsigned)>(table[0x120 / sizeof(quintptr)])(settings, 8);
        if (!value) return QStringLiteral("Cannot enable Workbench NET API");
        *value = true;
    }
    // Workbench owns listener creation and preserves an already-running server.
    // 1.9.0.37 evidence: 6b14d975b26d4295bd31856a35be3f68.
    reinterpret_cast<void (*)(void *, const char *)>(image + 0x2343e0)(app, "NetAPI");
    if (!detail::ownsListener(port))
        return QStringLiteral("Workbench NET API listener did not start on port %1").arg(port);
    if (changed) {
        // Save only Enable NET API. The owning routine consumes/frees this array.
        // Evidence: 592fec3623754e45acafc5d6bcf498d8.
        auto *index = reinterpret_cast<int *(*)(size_t, int)>(image + 0x1e953c0)(sizeof(int), 0);
        if (!index) return QStringLiteral("Cannot persist Workbench NET API setting");
        *index = 8;
        struct Indices { int *data; unsigned capacity; unsigned count; } indices{index, 1, 1};
        reinterpret_cast<void (*)(void *, Indices *)>(image + 0x236440)(app, &indices);
    }
    return {};
}
}

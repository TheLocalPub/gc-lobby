#pragma once
#include <QString>

namespace gc {
// One registration per semantic type, independent of field/component names.
// Entity selectors retain their explicit provider and identity contract.
inline QString referenceProvider(const QString &type)
{
    if (type == QStringLiteral("FactionKey")) return QStringLiteral("GC_FactionChoices");
    if (type == QStringLiteral("GC_MissionFlagName")) return QStringLiteral("GC_MissionFlagChoices");
    if (type == QStringLiteral("GC_MissionEventName")) return QStringLiteral("GC_MissionEventChoices");
    if (type == QStringLiteral("GC_TimerName")) return QStringLiteral("GC_TimerChoices");
    if (type == QStringLiteral("GC_RandomResetName")) return QStringLiteral("GC_RandomResetChoices");
    if (type == QStringLiteral("GC_WeatherPresetName")) return QStringLiteral("GC_WeatherChoices");
    return {};
}
}

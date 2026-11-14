#pragma once
#include <QString>

namespace gc {
// One registration per semantic type, independent of field/component names.
// The same registration selects the provider and its identity contract.
struct ReferenceType { QString provider; bool entities = false; };

inline ReferenceType referenceType(const QString &type)
{
    if (type == QStringLiteral("FactionKey")) return {QStringLiteral("GC_FactionChoices"), false};
    if (type == QStringLiteral("GC_MissionFlagName")) return {QStringLiteral("GC_MissionFlagChoices"), false};
    if (type == QStringLiteral("GC_MissionEventName")) return {QStringLiteral("GC_MissionEventChoices"), false};
    if (type == QStringLiteral("GC_TimerName")) return {QStringLiteral("GC_TimerChoices"), false};
    if (type == QStringLiteral("GC_RandomResetName")) return {QStringLiteral("GC_RandomResetChoices"), false};
    if (type == QStringLiteral("GC_WeatherPresetName")) return {QStringLiteral("GC_WeatherChoices"), false};
    if (type == QStringLiteral("GC_AreaName")) return {QStringLiteral("GC_AreaChoices"), true};
    return {};
}

inline QString referenceProvider(const QString &type) { return referenceType(type).provider; }
}

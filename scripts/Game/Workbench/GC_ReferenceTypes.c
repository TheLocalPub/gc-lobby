//! String-compatible reference types. Workbench offers choices by declared type,
//! including array entries, without field-specific widget or provider annotations.
//! Follow the engine's FactionKey typedef pattern; retain these types at runtime
//! so authored data has the same schema in Workbench and the game.
typedef string GC_MissionFlagName;
class GC_MissionFlagName : string {};

typedef string GC_MissionEventName;
class GC_MissionEventName : string {};

typedef string GC_TimerName;
class GC_TimerName : string {};

typedef string GC_RandomResetName;
class GC_RandomResetName : string {};

typedef string GC_WeatherPresetName;
class GC_WeatherPresetName : string {};

//! Reference to any supported world area, with identity-based editor discovery.
typedef string GC_AreaName;
class GC_AreaName : string {};

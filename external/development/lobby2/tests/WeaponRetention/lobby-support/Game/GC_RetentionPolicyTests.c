//! Explicit temporary fixture, mounted only for embedded-Play validation.
modded class GC_LobbyGameMode
{
	void GC_TestSetWeaponRetention(bool enabled)
	{
		m_bKeepWeaponWhenUnconscious = enabled;
	}
}

modded class SCR_GameModeHealthSettings
{
	override protected bool GC_TestGetRetention()
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		return mode && mode.IsKeepWeaponWhenUnconsciousEnabled();
	}

	override protected void GC_TestSetRetention(bool enabled)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (mode)
			GC_LobbyGameMode.Cast(mode).GC_TestSetWeaponRetention(enabled);
	}
}

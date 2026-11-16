//! Readback and map opening for explicit embedded-play regression checks only.
modded class GC_BriefingMarkerLayer
{
	static GC_BriefingMarkerLayer s_GC_TestLayer;
	override void Start()
	{
		super.Start();
		s_GC_TestLayer = this;
	}
	override void Stop()
	{
		if (s_GC_TestLayer == this)
			s_GC_TestLayer = null;
		super.Stop();
	}

	string GC_TestRead()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		string result = string.Format("widgets=%1 manager=%2 current=%3 map=%4 open=%5", m_aMarkerRoots.Count(), m_LobbyManager != null, m_LobbyManager == GC_LobbyManager.GetManager(), mapEntity != null, mapEntity && mapEntity.IsOpen());
		for (int index; index < m_aMarkers.Count(); index++)
		{
			float width, height;
			m_aMarkerImages[index].GetScreenSize(width, height);
			result += string.Format(" | %1 visible=%2 size=%3x%4", m_aMarkers[index].GetDescription(), m_aMarkerRoots[index].IsVisibleInHierarchy(), width, height);
		}
		return result;
	}

	void GC_TestRefresh()
	{
		RefreshMarkers();
	}
}

modded class GC_Lobby_SpectatorMenu
{
	void GC_TestOpenMap(bool open)
	{
		if (open)
			OpenSpectatorMap();
		else
			CloseSpectatorMap();
	}
}

modded class GC_LobbyPlayerController
{
	string GC_TestMapMarkers(bool openSpectator, bool closeSpectator)
	{
		GC_Lobby_SpectatorMenu menu = GC_Lobby_SpectatorMenu.Cast(m_SpectatorMenu);
		if (menu && (openSpectator || closeSpectator))
			menu.GC_TestOpenMap(openSpectator);
		GC_BriefingMarkerLayer layer = GC_BriefingMarkerLayer.s_GC_TestLayer;
		if (!layer)
			return "missing marker owner";
		return layer.GC_TestRead();
	}
}

modded class GC_LobbyManager
{
	string GC_TestRefreshMapMarkers()
	{
		GC_BriefingMarkerLayer layer = GC_BriefingMarkerLayer.s_GC_TestLayer;
		if (!layer)
			return "missing marker owner";
		layer.GC_TestRefresh();
		return layer.GC_TestRead();
	}

	void GC_TestFullscreenMarkers()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	string GC_TestMapMarkers(bool openSpectator, bool closeSpectator)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		GC_LobbyManager manager = GC_LobbyManager.Cast(this);
		string result = string.Format("registered=%1 eligible=%2 phase=%3", manager.GetBriefingMarkers().Count(), manager.GetBriefingMarkersForPlayer(controller.GetPlayerId()).Count(), manager.GetLobbyPhase());
		foreach (GC_BriefingMarker marker : manager.GetBriefingMarkers())
			result += string.Format(" | %1=%2", marker.GetName(), marker.GetDescription());
		return result + " | " + controller.GC_TestMapMarkers(openSpectator, closeSpectator);
	}
}

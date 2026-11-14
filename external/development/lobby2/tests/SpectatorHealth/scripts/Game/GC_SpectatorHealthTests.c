//! Explicit development fixture. Uses the production overlay and health toggle.
modded class GC_SpectatorCharacterEditableEntityUIComponent
{
	string GC_HealthGeometry()
	{
		float width, height, fillWidth, fillHeight;
		m_wHealthBar.GetScreenSize(width, height);
		m_wHealthFill.GetScreenSize(fillWidth, fillHeight);
		return string.Format("visible=%1 width=%2 height=%3 fill=%4 opacity=%5 health=%6", m_wHealthBar.IsVisibleInHierarchy(), width, height, fillWidth, m_wHealthBar.GetOpacity(), m_iHealthValue);
	}
}

modded class GC_SpectatorEditableEntityMarker
{
	string GC_HealthGeometry()
	{
		if (!m_CharacterUI)
			return "inactive";
		return m_CharacterUI.GC_HealthGeometry();
	}
}

modded class GC_SpectatorMarkerOverlay
{
	string GC_HealthGeometry(IEntity target)
	{
		Update();
		GC_SpectatorEditableEntityMarker marker;
		if (!m_mMarkers.Find(SCR_EditableEntityComponent.GetEditableEntity(target), marker))
			return "missing marker";
		return marker.GC_HealthGeometry();
	}
}

modded class GC_LobbyPlayerController
{
	string GC_HealthGeometry(IEntity target)
	{
		if (!m_SpectatorMarkerOverlay)
			return "missing overlay";
		return m_SpectatorMarkerOverlay.GC_HealthGeometry(target);
	}
}

modded class GC_LobbyManager
{
	protected IEntity m_GC_HealthTestTarget;

	string GC_PrepareHealthTest()
	{
		if (!Replication.IsServer() || !GetGame().GetPlayerController())
			return "local authority required";
		if (!TryAdvanceLobbyPhase(GetGame().GetPlayerController().GetPlayerId(), SCR_EGameModeState.BRIEFING))
			return "briefing transition rejected";
		SetSpectator(GetGame().GetPlayerController().GetPlayerId(), true);
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = "900 39 2630";
		m_GC_HealthTestTarget = GetGame().SpawnEntityPrefab(Resource.Load("{E398E44759DA1A43}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_TL.et"), GetGame().GetWorld(), params);
		if (!m_GC_HealthTestTarget)
			return "spawn failed";
		AIControlComponent.Cast(m_GC_HealthTestTarget.FindComponent(AIControlComponent)).DeactivateAI();
		return "prepared";
	}

	string GC_PositionHealthTest(float distance, float fov, bool enabled)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (!m_GC_HealthTestTarget || !controller.GC_CanJumpSpectatorCamera())
			return "spectator/target not ready";
		if (controller.GC_AreWorldHealthBarsEnabled() != enabled)
			controller.GC_ToggleWorldHealth();
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		vector position = m_GC_HealthTestTarget.GetOrigin() + "0 2.1 0";
		position[2] = position[2] - distance;
		camera.SetOrigin(position);
		camera.SetAngles(vector.Zero);
		camera.SetVerticalFOV(fov);
		return "positioned; read after a camera frame";
	}

	string GC_ReadHealthTest()
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		return string.Format("fov=%1 %2", GetGame().GetCameraManager().CurrentCamera().GetVerticalFOV(), controller.GC_HealthGeometry(m_GC_HealthTestTarget));
	}
}

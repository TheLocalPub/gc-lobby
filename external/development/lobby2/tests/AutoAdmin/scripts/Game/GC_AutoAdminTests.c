modded class GC_LobbyManager
{
	string GC_TestAutoAdmin()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		EPlayerRole before = GetGame().GetPlayerManager().GetPlayerRoles(playerId);
		int failures;
		GC_AutoAdmin.OnPlayerAuditSuccess(null, playerId);
		if (GetGame().GetPlayerManager().GetPlayerRoles(playerId) != before)
			failures++;
		GC_AutoAdmin.OnPlayerAuditSuccess(this, 0);
		if (GetGame().GetPlayerManager().GetPlayerRoles(playerId) != before)
			failures++;
		return string.Format("AutoAdmin: 2 guard checks, %1 failures", failures);
	}
}

//! The native server admin list authorizes login; subsequent actions use native roles.
class GC_AutoAdmin
{
	//------------------------------------------------------------------------------------------------
	//! Called only after the engine has authenticated the joining player.
	static void OnPlayerAuditSuccess(GC_LobbyManager owner, int playerId)
	{
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		if (!Replication.IsServer() || !mode || mode.FindComponent(GC_LobbyManager) != owner)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (playerId <= 0 || !players.IsPlayerConnected(playerId))
			return;

		BackendApi backend = GetGame().GetBackendApi();
		if (!backend || backend.GetPlayerIdentityId(playerId).IsEmpty())
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", "server-config game.admins", "rejected-unresolved-identity");
			return;
		}
		if (!backend.IsListedServerAdmin(playerId))
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", "server-config game.admins", "not-listed");
			return;
		}
		if (players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
			return;

		string outcome = "rejected-native-role";
		if (players.GivePlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
			outcome = "accepted";
		GC_AdminActionLogger.Log(playerId, "auto-admin", "server-config game.admins", outcome);
	}
}

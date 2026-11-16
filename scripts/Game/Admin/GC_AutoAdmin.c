//! Server configuration authorizes this login bootstrap; subsequent actions use native roles.
class GC_AutoAdmin
{
	static const string CONFIG_PATH = "$profile:Lobby2Admins.json";

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

		string identity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (identity.IsEmpty())
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", CONFIG_PATH, "rejected-unresolved-identity");
			return;
		}

		if (!FileIO.FileExists(CONFIG_PATH))
			return;
		FileHandle file = FileIO.OpenFile(CONFIG_PATH, FileMode.READ);
		if (!file)
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", CONFIG_PATH, "rejected-unreadable-config");
			return;
		}
		string payload;
		file.Read(payload, 65537);
		file.Close();
		array<string> identities = {};
		if (payload.Length() > 65536 || !ParseIdentities(payload, identities))
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", CONFIG_PATH, "rejected-invalid-config");
			return;
		}
		if (!identities.Contains(identity))
		{
			GC_AdminActionLogger.Log(playerId, "auto-admin", CONFIG_PATH, "not-listed");
			return;
		}
		if (players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
			return;

		string outcome = "rejected-native-role";
		if (players.GivePlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
			outcome = "accepted";
		GC_AdminActionLogger.Log(playerId, "auto-admin", CONFIG_PATH, outcome);
	}

	//------------------------------------------------------------------------------------------------
	static bool ParseIdentities(string payload, out array<string> identities)
	{
		payload.TrimInPlace();
		if (!payload.StartsWith("[") || !payload.EndsWith("]"))
			return false;
		// The serialization context reads named members; the on-disk document is only an array.
		JsonLoadContext json = new JsonLoadContext(false);
		// Validate the original document first so wrapping cannot accept trailing members.
		return json.LoadFromString(payload) && json.LoadFromString("{\"admins\":" + payload + "}")
			&& json.ReadValue("admins", identities);
	}
}

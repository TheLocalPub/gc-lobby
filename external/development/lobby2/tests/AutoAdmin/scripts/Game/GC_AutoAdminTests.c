modded class GC_LobbyManager
{
	string GC_TestAutoAdmin()
	{
		array<string> inputs = {"[]", "[\"00000000-0000-0000-0000-000000000001\"]", " \n[\"one\",\"two\"]\n ", "", "{}", "null", "\"one\"", "[", "[\"one\",]", "[\"one\"] garbage", "[\"one\"],\"extra\":[]", "[],\"admins\":[\"one\"]"};
		array<bool> expected = {true, true, true, false, false, false, false, false, false, false, false, false};
		int failures;
		foreach (int index, string input : inputs)
		{
			array<string> values = {};
			bool actual = GC_AutoAdmin.ParseIdentities(input, values);
			if (actual != expected[index])
			{
				failures++;
				PrintFormat("GC | AutoAdmin test failed | case=%1 actual=%2", index, actual);
			}
			if (index == 1 && (values.Count() != 1 || values[0] != "00000000-0000-0000-0000-000000000001"))
				failures++;
		}
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		EPlayerRole before = GetGame().GetPlayerManager().GetPlayerRoles(playerId);
		GC_AutoAdmin.OnPlayerAuditSuccess(null, playerId);
		GC_AutoAdmin.OnPlayerAuditSuccess(this, 0);
		if (GetGame().GetPlayerManager().GetPlayerRoles(playerId) != before)
			failures++;
		return string.Format("AutoAdmin: 15 checks, %1 failures", failures);
	}
}

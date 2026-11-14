//! Explicit MCP-only checks in a disposable embedded Arland session.
modded class GC_LobbyManager
{
	string GC_TestGroupExpansion(int slotId)
	{
		GC_LobbyManager manager = GC_LobbyManager.Cast(this);
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		bool accepted = true;
		if (slotId >= 0)
			accepted = manager.TryAssignPlayerToSlot(playerId, slotId);
		SCR_PlayerControllerGroupComponent membership = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		SCR_AIGroup selected = manager.GetPlayerGroup(playerId);
		int expectedId = -1;
		if (selected)
			expectedId = selected.GetGroupID();
		string result = string.Format("accepted=%1 groups=%2 membershipMatches=%3", accepted, manager.GetRegisteredGroups().Count(), membership.GetGroupID() == expectedId);
		foreach (SCR_AIGroup group : manager.GetRegisteredGroups())
		{
			result += string.Format(" | faction=%1 id=%2 players=%3 max=%4", group.GetFaction().GetFactionKey(), group.GetGroupID(), group.GetPlayerCount(), group.GetMaxMembers());
		}
		GC_Slot slot = manager.GetSlotByPlayerId(playerId);
		if (slot)
			result += string.Format(" | slot=%1 slotGroupMatches=%2", slot.GetID(), slot.GetGroup() == selected);
		return result;
	}
}

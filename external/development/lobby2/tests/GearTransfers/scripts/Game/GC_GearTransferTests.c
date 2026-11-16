//! Explicit development checks; invokes the production inventory handlers.
modded class SCR_InventoryMenuUI
{
	string GC_TestGearInput(int x, int y, string action)
	{
		WidgetManager.ReportMouse(x, y, m_widget);
		Widget cursor = WidgetManager.GetWidgetUnderCursor();
		string result = "hit=";
		if (cursor)
			result += cursor.GetName();
		SCR_InventorySlotUI slot;
		Widget current = cursor;
		while (current && !slot)
		{
			slot = SCR_InventorySlotUI.Cast(current.FindHandler(SCR_InventorySlotUI));
			current = current.GetParent();
		}
		if (slot && slot.GetInventoryItemComponent())
			result += " item=" + slot.GetInventoryItemComponent().GetOwner().GetPrefabData().GetPrefabName();
		if (action == "right")
			OnAction(null, "Inventory_Drop");
		if (action == "down")
			Action_DragDown();
		if (action == "drag")
			Action_OnDrag();
		if (action == "up")
			Action_DragUp();
		if (action == "hide")
			m_widget.FindAnyWidget("GearCratePanel").SetVisible(false);
		return result;
	}

	string GC_TestGearSlots()
	{
		string result = string.Format("admin=%1 workbench=%2 locked=%3", GC_GearSession.IsAdmin(m_Player), GC_GearSession.IsWorkbench(m_Player), m_InventoryManager.IsInventoryLocked());
		Widget crate = m_widget.FindAnyWidget("GearCratePanel");
		if (crate)
			result += string.Format(" crateVisible=%1", crate.IsVisibleInHierarchy());
		return result + GC_TestGearWidgets(m_widget);
	}

	protected string GC_TestGearWidgets(Widget root)
	{
		string result;
		SCR_InventorySlotUI slot = SCR_InventorySlotUI.Cast(root.FindHandler(SCR_InventorySlotUI));
		if (slot && root.IsVisibleInHierarchy() && slot.GetInventoryItemComponent())
		{
			IEntity item = slot.GetInventoryItemComponent().GetOwner();
			InventoryStorageSlot parent = slot.GetInventoryItemComponent().GetParentSlot();
			float x, y, w, h;
			root.GetScreenPos(x, y);
			root.GetScreenSize(w, h);
			result += string.Format("\n%1 at=%2,%3 draggable=%4", item.GetPrefabData().GetPrefabName(), x + w / 2, y + h / 2, slot.IsDraggable());
			if (parent)
				result += string.Format(" parent=%1 remove=%2 nativeRemove=%3", parent.GetStorage().Type(), parent.GetStorage().CanRemoveItem(item), m_InventoryManager.CanRemoveItemFromStorage(item, parent.GetStorage()));
		}
		Widget child = root.GetChildren();
		while (child)
		{
			result += GC_TestGearWidgets(child);
			child = child.GetSibling();
		}
		return result;
	}
}

modded class GC_LobbyManager
{
	string GC_TestGear(string action)
	{
		GC_LobbyPlayerController controller = GC_LobbyPlayerController.Cast(GetGame().GetPlayerController());
		if (action == "lobby")
			controller.RequestLobbyPhaseAdvance(SCR_EGameModeState.SLOTTING);
		if (action == "briefing")
			controller.RequestLobbyPhaseAdvance(SCR_EGameModeState.BRIEFING);
		if (action == "game")
			controller.RequestLobbyPhaseAdvance(SCR_EGameModeState.GAME);
		if (action == "slot")
			controller.RequestLobbySlot(10);
		if (action == "open")
			controller.GC_RequestGearOpen();
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.Inventory20Menu));
		if (menu)
			return menu.GC_TestGearSlots();
		return "no inventory menu";
	}

	string GC_TestGearInput(int x, int y, string action)
	{
		SCR_InventoryMenuUI menu = SCR_InventoryMenuUI.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.Inventory20Menu));
		if (!menu)
			return "no inventory menu";
		return menu.GC_TestGearInput(x, y, action);
	}
}

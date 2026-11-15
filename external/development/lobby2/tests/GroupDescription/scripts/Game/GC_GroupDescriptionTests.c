//! Explicit MCP-only presentation checks; never loaded by the production addon.
modded class SCR_AIGroup
{
	void GC_TestSetDescription(string description)
	{
		m_sGC_Description = description;
	}
}

modded class GC_LobbyManager
{
	protected GC_GroupWidgetComponent GC_TestFindGroupCard(Widget root)
	{
		if (!root)
			return null;
		GC_GroupWidgetComponent card = GC_GroupWidgetComponent.Cast(root.FindHandler(GC_GroupWidgetComponent));
		if (card && card.GetGroup())
			return card;
		Widget child = root.GetChildren();
		while (child)
		{
			card = GC_TestFindGroupCard(child);
			if (card)
				return card;
			child = child.GetSibling();
		}
		return null;
	}

	string GC_TestGroupDescription(string description, bool clickHeader)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "FAIL no menu";
		GC_GroupWidgetComponent card = GC_TestFindGroupCard(menu.GetRootWidget());
		if (!card)
			return "FAIL no group card";
		SCR_AIGroup group = card.GetGroup();
		string callsign = GC_GroupCallsignHelper.GetDisplayName(group);
		group.GC_TestSetDescription(description);
		card.Refresh();
		Widget root = card.GetRootWidget();
		if (clickHeader)
		{
			Widget header = root.FindAnyWidget("SquadHeader");
			GC_GroupHeaderContextComponent handler = GC_GroupHeaderContextComponent.Cast(header.FindHandler(GC_GroupHeaderContextComponent));
			handler.OnMouseButtonDown(header, 0, 0, 0);
		}
		TextWidget label = TextWidget.Cast(root.FindAnyWidget("SquadDescription"));
		return string.Format("description='%1' visible=%2 callsignUnchanged=%3 collapsed=%4", label.GetText(), label.IsVisible(), callsign == GC_GroupCallsignHelper.GetDisplayName(group), card.IsCollapsed());
	}
}

//! Development-only interaction with the production briefing handlers.
modded class GC_BriefingInteractiveRenderer
{
	string GC_TestShowcaseClick(string kind)
	{
		foreach (int index, SCR_ButtonBaseComponent button : m_aInlineButtons)
		{
			bool match = kind == "marker" && !m_aInlineMarkerNames[index].IsEmpty();
			match = match || (kind == "action" && !m_aInlineActionIds[index].IsEmpty());
			match = match || (kind == "link" && !m_aInlineLinkUrls[index].IsEmpty());
			if (!match)
				continue;
			button.m_OnClicked.Invoke(button);
			return "dispatched " + kind;
		}
		return "missing " + kind;
	}
}

modded class GC_BriefingViewComponent
{
	protected GC_BriefingTitleRowComponent GC_TestFindRow(Widget root, int order)
	{
		GC_BriefingTitleRowComponent row = GC_BriefingTitleRowComponent.Cast(root.FindHandler(GC_BriefingTitleRowComponent));
		if (row && row.GetBriefingOrder() == order && root.IsVisibleInHierarchy())
			return row;
		Widget child = root.GetChildren();
		while (child)
		{
			row = GC_TestFindRow(child, order);
			if (row)
				return row;
			child = child.GetSibling();
		}
		return null;
	}

	string GC_TestShowcase(int order, string action)
	{
		if (action == "select")
		{
			GC_BriefingTitleRowComponent row = GC_TestFindRow(m_wRoot, order);
			if (!row)
				return "missing visible row";
			if (!row.IsSelected())
				row.m_OnClicked.Invoke(row);
		}
		else if (action == "down" || action == "up")
		{
			int wheel = -1;
			if (action == "up")
				wheel = 1;
			for (int i; i < 20; i++)
				m_SectionScrollComponent.OnMouseWheel(m_wSectionScroll, 0, 0, wheel);
		}
		else if (action == "marker" || action == "action" || action == "link")
			return m_InteractiveRenderer.GC_TestShowcaseClick(action);
		return string.Format("title=%1 document=%2 plain=%3 rich=%4 inline=%5 images=%6", m_wSectionTitle.GetText(), m_wDocumentPanel.IsVisibleInHierarchy(), m_wPlainSectionBody.IsVisible(), m_wRichSectionBody.IsVisible(), m_wActionMarkerFlow.IsVisible(), m_wBlockImageFlow.IsVisible()) + " richText=" + m_wRichSectionBody.GetText();
	}
}

modded class GC_LobbyManager
{
	string GC_TestShowcase(int order, string action)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "no menu";
		Widget root = menu.GetRootWidget().FindAnyWidget("BriefingView");
		if (!root)
			return "no briefing view";
		GC_BriefingViewComponent view = GC_BriefingViewComponent.Cast(root.FindHandler(GC_BriefingViewComponent));
		return view.GC_TestShowcase(order, action);
	}

	string GC_TestShowcaseAudience(string factionKey, int phase)
	{
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		string result;
		foreach (GC_Briefing briefing : GetBriefings())
		{
			if (briefing.IsVisibleToFaction(faction, phase))
				result += briefing.GetOrder().ToString() + ",";
		}
		return result;
	}
}

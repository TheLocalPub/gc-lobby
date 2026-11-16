//! Explicit development checks; never shipped with the gameplay addon.
modded class GC_BriefingViewComponent
{
	string GC_TestBriefingZero(string action)
	{
		Widget rowRoot = m_wRoot.FindAnyWidget("BriefingTitleRow01");
		if (!rowRoot)
			return "missing row";
		GC_BriefingTitleRowComponent row = GC_BriefingTitleRowComponent.Cast(rowRoot.FindHandler(GC_BriefingTitleRowComponent));
		if (action == "click")
			row.m_OnClicked.Invoke(row);
		else if (action == "refresh")
			RefreshBriefings();
		return string.Format("index=%1 display=%2 selected=%3 document=%4 title=%5 body=%6", row.GetBriefingOrder(), row.GetDisplayOrder(), row.IsSelected(), m_wDocumentPanel.IsVisibleInHierarchy(), m_wSectionTitle.GetText(), m_wPlainSectionBody.GetText());
	}
}

modded class GC_LobbyManager
{
	string GC_TestBriefingZero(string action)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "no menu";
		Widget root = menu.GetRootWidget().FindAnyWidget("BriefingView");
		if (!root)
			return "no briefing view";
		GC_BriefingViewComponent view = GC_BriefingViewComponent.Cast(root.FindHandler(GC_BriefingViewComponent));
		return view.GC_TestBriefingZero(action);
	}

	string GC_TestBriefingZeroCatalog()
	{
		GC_Briefing original = new GC_Briefing();
		original.Set(0, "Zero", "Original");
		array<ref GC_Briefing> entries = {original};
		GC_LobbyBriefingCatalog catalog = new GC_LobbyBriefingCatalog(entries);
		if (catalog.GetBriefing(0) != original || catalog.GetBriefing(-1))
			return "FAIL lookup";
		GC_Briefing update = new GC_Briefing();
		update.Set(0, "Zero", "Updated");
		GC_Briefing applied;
		if (!catalog.UpsertBriefing(update, applied) || entries.Count() != 1 || applied.GetContent() != "Updated")
			return "FAIL upsert";
		update.SetRevision(applied.GetRevision() + 1);
		if (!catalog.ApplyBriefing(update) || entries.Count() != 1)
			return "FAIL replicated update";
		int revision;
		if (!catalog.RemoveBriefing(0, revision) || catalog.GetBriefing(0))
			return "FAIL removal";
		return "PASS zero lookup, update, replication application and removal";
	}
}

//! Explicit checks of the live briefing view and its production click handler.
modded class GC_BriefingViewComponent
{
	string GC_TestScenarioTab(bool click)
	{
		if (!m_ScenarioTab)
			return "missing scenario tab";
		if (click)
			m_ScenarioTab.m_OnClicked.Invoke(m_ScenarioTab);
		Widget tab = m_wRoot.FindAnyWidget("BriefingScenarioTab");
		return string.Format("visible=%1 selected=%2 order=%3 last=%4 document=%5 title=%6", tab.IsVisibleInHierarchy(), m_ScenarioTab.IsSelected(), m_ScenarioTab.GetDisplayOrder(), tab.GetSibling() == null, m_wDocumentPanel.IsVisibleInHierarchy(), m_wSectionTitle.GetText());
	}
}

modded class GC_LobbyManager
{
	string GC_TestScenarioTab(bool click)
	{
		MenuBase menu = GetGame().GetMenuManager().GetTopMenu();
		if (!menu)
			return "no menu";
		Widget root = menu.GetRootWidget().FindAnyWidget("BriefingView");
		if (!root)
			return "no briefing view";
		GC_BriefingViewComponent view = GC_BriefingViewComponent.Cast(root.FindHandler(GC_BriefingViewComponent));
		return view.GC_TestScenarioTab(click);
	}
}

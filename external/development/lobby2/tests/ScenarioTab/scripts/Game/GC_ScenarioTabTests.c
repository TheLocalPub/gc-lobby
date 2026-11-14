//! Explicit checks of the live briefing view and its production click handler.
modded class GC_BriefingViewComponent
{
	protected bool m_bGC_TestParametersOverride;
	protected bool m_bGC_TestParametersVisible;

	override protected void RefreshParametersTab()
	{
		if (!m_bGC_TestParametersOverride)
		{
			super.RefreshParametersTab();
			return;
		}
		m_wParametersTabRoot.SetVisible(m_bGC_TestParametersVisible);
	}

	string GC_TestParameterVisibility(bool visible)
	{
		// Isolate late visibility delivery without altering the authority's catalog.
		m_bGC_TestParametersOverride = true;
		m_bGC_TestParametersVisible = visible;
		OnScenarioParametersChanged(0);
		string result = GC_TestScenarioTab(false);
		m_bGC_TestParametersOverride = false;
		return result;
	}

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
	string GC_TestParameterVisibility(bool visible)
	{
		Widget root = GetGame().GetMenuManager().GetTopMenu().GetRootWidget().FindAnyWidget("BriefingView");
		GC_BriefingViewComponent view = GC_BriefingViewComponent.Cast(root.FindHandler(GC_BriefingViewComponent));
		return view.GC_TestParameterVisibility(visible);
	}

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

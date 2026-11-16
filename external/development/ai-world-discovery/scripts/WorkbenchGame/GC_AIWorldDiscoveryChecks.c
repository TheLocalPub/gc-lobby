#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Check AI World Discovery", wbModules: {"WorldEditor"}, category: "GC Development")]
class GC_AIWorldDiscoveryChecks : GC_MissionTemplatePlugin
{
	override void Run()
	{
		string notice;
		float started = System.GetTickCount();
		ResourceName selected = DiscoverAIWorldPrefab("Worlds/BelleauWood/BelleauWood.ent", notice);
		PrintFormat("GC Discovery Check | Belleau=%1 count=%2 elapsed=%3 notice=%4", selected, m_aAIWorldPrefabs.Count(), System.GetTickCount() - started, notice);
		if (selected != "{CFDC9C151CAB3F8D}Prefabs/AI/SCR_AIWorld_BelleauWood.et" || notice.IsEmpty())
			Print("GC Discovery Check | FAIL Belleau", LogLevel.ERROR);
		m_aAIWorldPrefabs.Clear();
		OnAIWorldPrefab(TEMPLATE, "");
		if (!m_aAIWorldPrefabs.IsEmpty())
			Print("GC Discovery Check | FAIL non-AI accepted", LogLevel.ERROR);
		OnAIWorldPrefab(selected, "");
		OnAIWorldPrefab(selected, "");
		if (m_aAIWorldPrefabs.Count() != 1)
			Print("GC Discovery Check | FAIL deduplication/type", LogLevel.ERROR);
		Resource renamed = Resource.Load(TEMPLATE);
		if (!renamed || !renamed.IsValid())
			Print("GC Discovery Check | FAIL renamed resource", LogLevel.ERROR);
		PrintFormat("GC Discovery Check | Known Eden=%1 renamed=%2", GetAIWorldPrefab("worlds/Eden/Eden.ent"), TEMPLATE);
		Print("GC Discovery Check | Complete");
	}
}

[WorkbenchPluginAttribute(name: "Check AI World Fallback", wbModules: {"WorldEditor"}, category: "GC Development")]
class GC_AIWorldFallbackChecks : GC_MissionTemplatePlugin
{
	override void Run()
	{
		string notice;
		float started = System.GetTickCount();
		ResourceName selected = DiscoverAIWorldPrefab("Worlds/GC_NoMatchingTerrainName_20261116.ent", notice);
		PrintFormat("GC Discovery Check | Broad=%1 count=%2 elapsed=%3 notice=%4", selected, m_aAIWorldPrefabs.Count(), System.GetTickCount() - started, notice);
		if (selected.IsEmpty() || m_aAIWorldPrefabs.Count() < 2 || notice.IsEmpty())
			Print("GC Discovery Check | FAIL broad/multiple", LogLevel.ERROR);
	}
}

#endif

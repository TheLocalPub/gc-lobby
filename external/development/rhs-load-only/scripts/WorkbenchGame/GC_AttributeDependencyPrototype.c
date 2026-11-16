#ifdef WORKBENCH
[BaseContainerProps()]
class GC_AttributeDependencyReport
{
	[Attribute()]
	ResourceName m_sRoot;
	[Attribute()]
	string m_sError;
	[Attribute()]
	ref array<ref GC_MissionMetadataResource> m_aReferences = {};
	[Attribute()]
	ref array<ref GC_MissionMetadataAddon> m_aLoadedAddons = {};
	[Attribute()]
	ref array<string> m_aLimitations = {"Prototype resource-field evidence, not a launchable complete dependency manifest.", "Traverses effective inline objects, arrays, prefab children and referenced .et/.conf containers.", "Binary resources are recorded but not expanded or assigned an owner; dynamic scripts and plain string selectors are not resolved."};
}

[WorkbenchPluginAttribute(name: "Probe attribute dependencies", description: "Explicit test-only scan of the attribute fixture; writes attribute-evidence.conf in this test addon.", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_AttributeDependencyProbePlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		GC_MissionResourceReferences scanner = new GC_MissionResourceReferences(System.GetTickCount());
		GC_AttributeDependencyReport report = new GC_AttributeDependencyReport();
		report.m_sRoot = "{7E161126A01D0020}Prefabs/AttributeCharacter.et";
		scanner.AddResource(report.m_sRoot, "root");
		scanner.Expand();
		report.m_sError = scanner.GetError();
		report.m_aReferences = scanner.GetReferences();
		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
		{
			GC_MissionMetadataAddon addon = new GC_MissionMetadataAddon();
			addon.m_sId = GameProject.GetAddonID(guid);
			addon.m_sGuid = guid;
			report.m_aLoadedAddons.Insert(addon);
		}
		string directory;
		if (!Workbench.GetAbsolutePath("$GCRhsLoadOnlyTest:", directory))
			return;
		Resource data = BaseContainerTools.CreateContainerFromInstance(report);
		bool saved = data && data.IsValid() && BaseContainerTools.SaveContainer(data.GetResource().ToBaseContainer(), "", directory + "/attribute-evidence.conf");
		Print(string.Format("GC_ATTRIBUTE_TEST saved=%1 references=%2 error=%3", saved, report.m_aReferences.Count(), report.m_sError));
	}
}
#endif

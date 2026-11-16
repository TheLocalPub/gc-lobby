#ifdef WORKBENCH
[BaseContainerProps()]
class GC_AttributeDependencyRow
{
	[Attribute()]
	string m_sOrigin;
	[Attribute()]
	ResourceName m_sResource;
	[Attribute()]
	string m_sStatus;
	[Attribute()]
	ref array<string> m_aAddonIds = {};
}

[BaseContainerProps()]
class GC_AttributeDependencyReport
{
	[Attribute()]
	ResourceName m_sRoot;
	[Attribute()]
	string m_sError;
	[Attribute()]
	int m_iContainers;
	[Attribute()]
	int m_iExpandedResources;
	[Attribute()]
	ref array<ref GC_AttributeDependencyRow> m_aReferences = {};
	[Attribute()]
	ref array<ref GC_MissionMetadataAddon> m_aLoadedAddons = {};
	[Attribute()]
	ref array<string> m_aLimitations = {"Prototype resource-field evidence, not a launchable complete dependency manifest.", "Traverses effective inline objects, arrays, prefab children and referenced .et/.conf containers.", "Binary resources are recorded but not expanded or assigned an owner; dynamic scripts and plain string selectors are not resolved."};
}

//! No addon names or equipment field names are special-cased in this collector.
class GC_AttributeDependencyPrototype
{
	protected ref GC_AttributeDependencyReport m_Report;
	protected ref array<ResourceName> m_aQueue = {};
	protected ref map<ResourceName, ref array<string>> m_mAddons = new map<ResourceName, ref array<string>>();
	protected ref map<ResourceName, string> m_mStatus = new map<ResourceName, string>();
	protected int m_iStarted;

	//------------------------------------------------------------------------------------------------
	GC_AttributeDependencyReport Collect(ResourceName root)
	{
		m_Report = new GC_AttributeDependencyReport();
		m_Report.m_sRoot = root;
		m_iStarted = System.GetTickCount();
		m_aQueue.Clear();
		m_mAddons.Clear();
		m_mStatus.Clear();
		Reference(root, "root");
		for (int i; i < m_aQueue.Count() && m_Report.m_sError.IsEmpty(); i++)
		{
			ResourceName name = m_aQueue[i];
			Resource resource = BaseContainerTools.LoadContainer(name);
			BaseContainer container;
			if (resource && resource.IsValid())
				container = resource.GetResource().ToBaseContainer();
			if (!container)
			{
				m_Report.m_sError = "Queued container could not be reopened: " + name;
				break;
			}
			m_Report.m_iExpandedResources++;
			Visit(container, name, 0);
		}
		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
		{
			GC_MissionMetadataAddon addon = new GC_MissionMetadataAddon();
			addon.m_sGuid = guid;
			addon.m_sId = GameProject.GetAddonID(guid);
			addon.m_sTitle = GameProject.GetAddonTitle(guid);
			addon.m_bVanilla = GameProject.IsVanillaAddon(guid);
			m_Report.m_aLoadedAddons.Insert(addon);
		}
		return m_Report;
	}

	//------------------------------------------------------------------------------------------------
	protected bool WithinBudget(int depth)
	{
		if (!m_Report.m_sError.IsEmpty())
			return false;
		if (depth > 64 || m_Report.m_iContainers > 50000 || m_aQueue.Count() > 2000 || m_Report.m_aReferences.Count() > 50000 || System.GetTickCount(m_iStarted) > 30000)
		{
			m_Report.m_sError = "Attribute graph exceeded nesting, container, resource, reference or 30-second limit; evidence is incomplete.";
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Reference(ResourceName name, string origin)
	{
		if (name.IsEmpty() || !WithinBudget(0))
			return;
		GC_AttributeDependencyRow row = new GC_AttributeDependencyRow();
		row.m_sOrigin = origin;
		row.m_sResource = name;
		if (!m_mStatus.Contains(name))
		{
			array<string> addons = {};
			string status = "non_container_not_expanded";
			string path = name.GetPath();
			if (path.EndsWith(".et") || path.EndsWith(".conf"))
			{
				status = "unresolved";
				Resource resource = BaseContainerTools.LoadContainer(name);
				BaseContainer container;
				if (resource && resource.IsValid())
					container = resource.GetResource().ToBaseContainer();
				if (container)
				{
					status = "container";
					int depth;
					BaseContainer ancestor = container;
					while (ancestor && depth++ < 128)
					{
						array<string> sources = {};
						ancestor.GetSourceAddons(sources);
						foreach (string source : sources)
							if (!addons.Contains(source))
								addons.Insert(source);
						ancestor = ancestor.GetAncestor();
					}
					if (ancestor)
						m_Report.m_sError = "Resource ancestry exceeded 128 levels.";
					m_aQueue.Insert(name);
				}
			}
			addons.Sort();
			m_mAddons.Set(name, addons);
			m_mStatus.Set(name, status);
		}
		row.m_sStatus = m_mStatus.Get(name);
		row.m_aAddonIds.Copy(m_mAddons.Get(name));
		m_Report.m_aReferences.Insert(row);
	}

	//------------------------------------------------------------------------------------------------
	protected void Visit(BaseContainer container, string origin, int depth)
	{
		if (!container || !WithinBudget(depth))
			return;
		m_Report.m_iContainers++;
		for (int v; v < container.GetNumVars() && WithinBudget(depth); v++)
		{
			string key = container.GetVarName(v);
			string field = origin + "/" + container.GetClassName() + "." + key;
			DataVarType type = container.GetDataVarType(v);
			if (type == DataVarType.RESOURCE_NAME || type == DataVarType.STRING)
			{
				string value;
				if (container.Get(key, value) && (type == DataVarType.RESOURCE_NAME || value.StartsWith("{")))
					Reference(value, field);
			}
			else if (type == DataVarType.RESOURCE_NAME_ARRAY || type == DataVarType.STRING_ARRAY)
			{
				array<string> values = {};
				if (container.Get(key, values))
					foreach (int index, string item : values)
						if (type == DataVarType.RESOURCE_NAME_ARRAY || item.StartsWith("{"))
							Reference(item, field + "[" + index + "]");
			}
			else if (type == DataVarType.OBJECT)
				Visit(container.GetObject(key), field, depth + 1);
			else if (type == DataVarType.OBJECT_ARRAY)
			{
				BaseContainerList objects = container.GetObjectArray(key);
				for (int o; objects && o < objects.Count() && WithinBudget(depth); o++)
					Visit(objects.Get(o), field + "[" + o + "]", depth + 1);
			}
		}
		for (int child; child < container.GetNumChildren() && WithinBudget(depth); child++)
			Visit(container.GetChild(child), origin + "/child[" + child + "]", depth + 1);
	}
}

[WorkbenchPluginAttribute(name: "Probe attribute dependencies", description: "Explicit test-only scan of the attribute fixture; writes attribute-evidence.conf in this test addon.", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_AttributeDependencyProbePlugin : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		GC_AttributeDependencyPrototype scanner = new GC_AttributeDependencyPrototype();
		GC_AttributeDependencyReport report = scanner.Collect("{7E161126A01D0020}Prefabs/AttributeCharacter.et");
		string directory;
		if (!Workbench.GetAbsolutePath("$GCRhsLoadOnlyTest:", directory))
			return;
		Resource data = BaseContainerTools.CreateContainerFromInstance(report);
		bool saved = data && data.IsValid() && BaseContainerTools.SaveContainer(data.GetResource().ToBaseContainer(), "", directory + "/attribute-evidence.conf");
		Print(string.Format("GC_ATTRIBUTE_TEST saved=%1 resources=%2 containers=%3 references=%4 error=%5", saved, report.m_iExpandedResources, report.m_iContainers, report.m_aReferences.Count(), report.m_sError));
	}
}
#endif

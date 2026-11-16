#ifdef WORKBENCH
//! Bounded editor-only evidence graph. One row per resource, retaining its first field origin.
//! Effective fields include instance overrides; linked containers are expanded once to break cycles.
class GC_MissionResourceReferences
{
	protected ref array<ResourceName> m_aQueue = {};
	protected ref map<ResourceName, ref GC_MissionMetadataResource> m_mResources = new map<ResourceName, ref GC_MissionMetadataResource>();
	protected ref array<string> m_aAddonIds = {};
	protected int m_iStarted;
	protected int m_iContainers;
	protected string m_sError;
	protected ref array<string> m_aExcludedFields = {};
	protected bool m_bLoadoutFields;

	//------------------------------------------------------------------------------------------------
	//! Only native equipment/identity references; excludes gameplay services and catalogues.
	void UseLoadoutFields()
	{
		m_bLoadoutFields = true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsLoadoutReference(BaseContainer container, string key)
	{
		typename type = container.GetClassName().ToType();
		if (key == "Prefab")
			return type && (type.IsInherited(InventoryStorageSlot) || type.IsInherited(LoadoutSlotInfo));
		if (key == "WeaponTemplate")
			return type && type.IsInherited(WeaponSlotComponent);
		if (key == "MagazineTemplate")
			return type && type.IsInherited(BaseMuzzleComponent);
		if (key == "AmmoConfig")
			return type && type.IsInherited(BaseMagazineComponent);
		if (key == "AmmoResourceArray")
			return container.GetClassName() == "MagazineConfig";
		if (key == "PrefabsToSpawn")
			return container.GetClassName() == "ItemsInitConfigurationItem";
		if (key == "Head" || key == "Body")
			return container.GetClassName() == "VisualIdentity";
		if (key == "m_sVariantPrefab")
			return type && type.IsInherited(SCR_EditableEntityVariant);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Callers may narrow an evidence view without changing the mission export defaults.
	void ExcludeField(string field)
	{
		m_aExcludedFields.Insert(field);
	}

	//------------------------------------------------------------------------------------------------
	void GC_MissionResourceReferences(int started)
	{
		m_iStarted = started;
	}

	//------------------------------------------------------------------------------------------------
	void AddEntity(IEntitySource source)
	{
		// The mission exporter already visits each entity child separately.
		Visit(source, "entity:" + source.GetName(), 0, false);
	}

	//------------------------------------------------------------------------------------------------
	void AddResource(ResourceName name, string origin)
	{
		if (name.IsEmpty() || !WithinBudget(0) || m_mResources.Contains(name))
			return;
		GC_MissionMetadataResource row = new GC_MissionMetadataResource();
		row.m_sResource = name;
		row.m_sOrigin = origin;
		row.m_sStatus = "non_container_not_expanded";
		m_mResources.Set(name, row);
		string path = name.GetPath();
		if (!path.EndsWith(".et") && !path.EndsWith(".conf"))
			return;
		row.m_sStatus = "unresolved";
		Resource resource = BaseContainerTools.LoadContainer(name);
		BaseContainer container;
		if (resource && resource.IsValid())
			container = resource.GetResource().ToBaseContainer();
		if (!container)
			return;
		row.m_sStatus = "container";
		BaseContainer ancestor = container;
		int depth;
		while (ancestor && depth++ < 128)
		{
			array<string> sources = {};
			ancestor.GetSourceAddons(sources);
			foreach (string id : sources)
			{
				if (id.IsEmpty())
					continue;
				if (!row.m_aAddonIds.Contains(id))
					row.m_aAddonIds.Insert(id);
				if (!m_aAddonIds.Contains(id))
					m_aAddonIds.Insert(id);
			}
			ancestor = ancestor.GetAncestor();
		}
		if (ancestor)
			m_sError = "Resource ancestry exceeded 128 levels.";
		row.m_aAddonIds.Sort();
		m_aQueue.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	bool Expand()
	{
		for (int i; i < m_aQueue.Count() && WithinBudget(0); i++)
		{
			ResourceName name = m_aQueue[i];
			Resource resource = BaseContainerTools.LoadContainer(name);
			BaseContainer container;
			if (resource && resource.IsValid())
				container = resource.GetResource().ToBaseContainer();
			if (!container)
			{
				m_sError = "Referenced container could not be reopened: " + name;
				break;
			}
			Visit(container, name, 0);
		}
		m_aAddonIds.Sort();
		return m_sError.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	string GetError()
	{
		return m_sError;
	}

	//------------------------------------------------------------------------------------------------
	array<string> GetAddonIds()
	{
		return m_aAddonIds;
	}

	//------------------------------------------------------------------------------------------------
	array<ref GC_MissionMetadataResource> GetReferences()
	{
		array<ResourceName> names = {};
		foreach (ResourceName name, GC_MissionMetadataResource row : m_mResources)
			names.Insert(name);
		names.Sort();
		array<ref GC_MissionMetadataResource> result = {};
		foreach (ResourceName key : names)
			result.Insert(m_mResources.Get(key));
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Keep the mission header compact: retain addon ownership/unresolved evidence,
	//! count binary references and successfully expanded vanilla-only containers.
	void WriteSnapshot(GC_GeneratedMissionMetadata snapshot)
	{
		array<string> guids = {};
		array<string> vanillaIds = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
			if (GameProject.IsVanillaAddon(guid))
				vanillaIds.Insert(GameProject.GetAddonID(guid));
		snapshot.m_aResourceReferences = {};
		snapshot.m_iUnexpandedResourceCount = 0;
		snapshot.m_iOmittedVanillaResourceCount = 0;
		foreach (GC_MissionMetadataResource row : GetReferences())
		{
			if (row.m_sStatus == "non_container_not_expanded")
			{
				snapshot.m_iUnexpandedResourceCount++;
				continue;
			}
			bool vanillaOnly = row.m_sStatus == "container" && !row.m_aAddonIds.IsEmpty();
			foreach (string id : row.m_aAddonIds)
				if (!vanillaIds.Contains(id))
					vanillaOnly = false;
			if (vanillaOnly)
				snapshot.m_iOmittedVanillaResourceCount++;
			else
				snapshot.m_aResourceReferences.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool WithinBudget(int depth)
	{
		if (!m_sError.IsEmpty())
			return false;
		if (depth > 64 || m_iContainers >= 1000000 || m_mResources.Count() >= 50000 || System.GetTickCount(m_iStarted) > 30000)
		{
			m_sError = string.Format("Resource scan exceeded a safety limit: depth=%1, containers=%2, resources=%3, elapsed=%4ms. Metadata was not refreshed.", depth, m_iContainers, m_mResources.Count(), System.GetTickCount(m_iStarted));
			return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Visit(BaseContainer container, string origin, int depth, bool children = true)
	{
		if (!container || !WithinBudget(depth))
			return;
		if (m_bLoadoutFields)
		{
			typename containerType = container.GetClassName().ToType();
			// Match the loadout writer: temporary medical/hand storage is not equipment.
			if (containerType && (containerType.IsInherited(SCR_HandSlotStorageComponent) || containerType.IsInherited(SCR_SalineStorageComponent) || containerType.IsInherited(SCR_TourniquetStorageComponent)))
				return;
		}
		m_iContainers++;
		for (int v; v < container.GetNumVars() && WithinBudget(depth); v++)
		{
			string key = container.GetVarName(v);
			// Saved evidence is output, never a new dependency input.
			if (key == "m_GC_GeneratedMetadata" || m_aExcludedFields.Contains(key))
				continue;
			string field = origin + "/" + container.GetClassName() + "." + key;
			DataVarType type = container.GetDataVarType(v);
			if (type == DataVarType.RESOURCE_NAME || type == DataVarType.STRING)
			{
				if (m_bLoadoutFields && !IsLoadoutReference(container, key))
					continue;
				string value;
				if (container.Get(key, value) && (type == DataVarType.RESOURCE_NAME || value.StartsWith("{")))
					AddResource(value, field);
			}
			else if (type == DataVarType.RESOURCE_NAME_ARRAY || type == DataVarType.STRING_ARRAY)
			{
				if (m_bLoadoutFields && !IsLoadoutReference(container, key))
					continue;
				array<string> values = {};
				if (container.Get(key, values))
					foreach (int index, string item : values)
						if (type == DataVarType.RESOURCE_NAME_ARRAY || item.StartsWith("{"))
							AddResource(item, field + "[" + index + "]");
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
		for (int child; children && child < container.GetNumChildren() && WithinBudget(depth); child++)
			Visit(container.GetChild(child), origin + "/child[" + child + "]", depth + 1);
	}
}
#endif

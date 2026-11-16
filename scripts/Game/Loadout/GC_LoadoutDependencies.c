#ifdef WORKBENCH
//! Saved-prefab evidence, shared with mission discovery. Never alters project dependencies.
class GC_LoadoutDependencyMod
{
	string m_sId;
	string m_sGuid;
	string m_sTitle;
	ref array<string> m_aReasons = {};
}

class GC_LoadoutDependencies
{
	ResourceName m_sPrefab;
	bool m_bAllVariants;
	string m_sError;
	int m_iUnresolved;
	int m_iUnexpanded;
	ref array<ref GC_LoadoutDependencyMod> m_aMods = {};
	protected ref array<ref GC_MissionMetadataResource> m_aReferences = {};
	protected ref map<string, string> m_mGuids = new map<string, string>();
	protected ref map<string, ref array<string>> m_mOwners = new map<string, ref array<string>>();
	protected ref map<ResourceName, ref array<ResourceName>> m_mParents = new map<ResourceName, ref array<ResourceName>>();

	//------------------------------------------------------------------------------------------------
	//! UI policy for the GC authoring baseline; Encode/Save retain all requirements.
	bool IsImplied(GC_LoadoutDependencyMod mod)
	{
		array<string> authoring = {"GlobalConflictsCoreDev", "GlobalConflictsUnitsDev", "GlobalConflictsLobbyDev", "GlobalConflictsMissionsDev", "GlobalConflictsWorkshopDev"};
		if (authoring.Contains(mod.m_sId))
			return true;
		if (!m_mGuids.Contains("GlobalConflictsCoreDev"))
			return false;
		if (mod.m_sId == "CoalitionSquadInterface")
			return true;
		// Core bundles the official ACE Dev suite. Other content packs remain visible,
		// including packs that happen to be loaded through Units or Workshop.
		return m_mGuids.Contains("ACE_AllInOne_Dev") && mod.m_sId.StartsWith("ACE_") && mod.m_sId.EndsWith("_Dev");
	}

	//------------------------------------------------------------------------------------------------
	static GC_LoadoutDependencies Scan(ResourceName prefab, bool allVariants)
	{
		GC_LoadoutDependencies result = new GC_LoadoutDependencies();
		result.m_sPrefab = prefab;
		result.m_bAllVariants = allVariants;
		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
			result.m_mGuids.Set(GameProject.GetAddonID(guid), guid);

		GC_MissionResourceReferences graph = new GC_MissionResourceReferences(System.GetTickCount());
		graph.UseLoadoutFields();
		if (!allVariants)
			graph.ExcludeField("m_VariantData");
		graph.AddResource(prefab, "loadout");
		if (!graph.Expand())
			result.m_sError = graph.GetError();
		result.m_aReferences = graph.GetReferences();

		// Collect native resource definitions by GUID; distinguish original content
		// from global patches before adding loadout requirements.
		foreach (GC_MissionMetadataResource entry : result.m_aReferences)
		{
			array<ResourceName> parents = {};
			result.TrackOwner(entry.m_sResource);
			Resource resource = BaseContainerTools.LoadContainer(entry.m_sResource);
			if (resource && resource.IsValid())
			{
				BaseContainer source = resource.GetResource().ToBaseContainer();
				int depth;
				while (source && depth++ < 128)
				{
					ResourceName name = source.GetResourceName();
					if (!name.IsEmpty() && !parents.Contains(name))
					{
						parents.Insert(name);
						result.TrackOwner(name);
					}
					source = source.GetAncestor();
				}
			}
			result.m_mParents.Set(entry.m_sResource, parents);
		}
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.fileExtensions = { "et", "conf" };
		if (!ResourceDatabase.SearchResources(filter, result.OnResourceOwner))
			result.m_sError = "Resource ownership unavailable";

		foreach (GC_MissionMetadataResource row : result.m_aReferences)
		{
			if (row.m_sStatus == "unresolved")
				result.m_iUnresolved++;
			if (row.m_sStatus == "non_container_not_expanded")
				result.m_iUnexpanded++;
			row.m_aAddonIds.Clear();
			array<ResourceName> names = result.m_mParents.Get(row.m_sResource);
			if (!names.Contains(row.m_sResource))
				names.Insert(row.m_sResource);
			foreach (ResourceName name : names)
			{
				string id = result.GetOwner(name);
				// Newly saved native resources may not be enumerated until Play stops.
				if (id.IsEmpty() && GC_LoadoutPrefab.IsWritable(name))
					id = "GlobalConflictsUnitsDev";
				if (id.IsEmpty())
				{
					result.m_iUnresolved++;
					continue;
				}
				if (!row.m_aAddonIds.Contains(id))
					row.m_aAddonIds.Insert(id);
				string reason = name;
				if (name != row.m_sResource)
					reason = "Inherited: " + reason;
				result.Add(id, result.m_mGuids.Get(id), reason);
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected void TrackOwner(ResourceName resource)
	{
		string key = SCR_ConfigHelper.GetGUID(resource);
		if (!m_mOwners.Contains(key))
			m_mOwners.Set(key, {});
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResourceOwner(ResourceName resource, string exactPath)
	{
		string key = SCR_ConfigHelper.GetGUID(resource);
		if (!m_mOwners.Contains(key) || !exactPath.StartsWith("$"))
			return;
		int end = exactPath.IndexOf(":");
		if (end <= 1)
			return;
		string id = exactPath.Substring(1, end - 1);
		array<string> owners = m_mOwners.Get(key);
		if (!owners.Contains(id))
			owners.Insert(id);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetOwner(ResourceName resource)
	{
		array<string> owners = m_mOwners.Get(SCR_ConfigHelper.GetGUID(resource));
		// Native enumeration includes overrides, sometimes with differently cased paths.
		// A vanilla identity remains vanilla. Multiple third-party definitions are
		// ambiguous: report partial evidence rather than invent a mod requirement.
		foreach (string id : owners)
			if (GameProject.IsVanillaAddon(m_mGuids.Get(id)))
				return id;
		if (owners.Count() == 1)
			return owners[0];
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void Add(string id, string guid, string reason)
	{
		if (!guid.IsEmpty() && GameProject.IsVanillaAddon(guid))
			return;
		foreach (GC_LoadoutDependencyMod existing : m_aMods)
		{
			if (existing.m_sId != id)
				continue;
			if (!existing.m_aReasons.Contains(reason))
				existing.m_aReasons.Insert(reason);
			return;
		}
		GC_LoadoutDependencyMod mod = new GC_LoadoutDependencyMod();
		mod.m_sId = id;
		mod.m_sGuid = guid;
		mod.m_sTitle = GameProject.GetAddonTitle(guid);
		if (mod.m_sTitle.IsEmpty())
			mod.m_sTitle = id;
		mod.m_aReasons.Insert(reason);
		m_aMods.Insert(mod);
	}

	//------------------------------------------------------------------------------------------------
	string Encode()
	{
		JsonSaveContext json = new JsonSaveContext();
		json.WriteValue("schemaVersion", 1);
		json.WriteValue("prefab", m_sPrefab);
		json.WriteValue("allVariants", m_bAllVariants);
		json.WriteValue("coverage", "loadout_resource_references_partial");
		json.WriteValue("error", m_sError);
		json.WriteValue("unresolvedResources", m_iUnresolved);
		json.WriteValue("unexpandedResources", m_iUnexpanded);
		int modCount = m_aMods.Count();
		json.StartArray("mods", modCount);
		foreach (GC_LoadoutDependencyMod mod : m_aMods)
		{
			json.StartObject();
			json.WriteValue("id", mod.m_sId);
			json.WriteValue("guid", mod.m_sGuid);
			json.WriteValue("title", mod.m_sTitle);
			json.WriteValue("reasons", mod.m_aReasons);
			json.EndObject();
		}
		json.EndArray();
		int resourceCount = m_aReferences.Count();
		json.StartArray("resources", resourceCount);
		foreach (GC_MissionMetadataResource row : m_aReferences)
		{
			json.StartObject();
			json.WriteValue("resource", row.m_sResource);
			json.WriteValue("origin", row.m_sOrigin);
			json.WriteValue("status", row.m_sStatus);
			json.WriteValue("addonIds", row.m_aAddonIds);
			json.EndObject();
		}
		json.EndArray();
		return json.SaveToString();
	}

	//------------------------------------------------------------------------------------------------
	bool Save()
	{
		if (!m_sError.IsEmpty() || !GC_LoadoutPrefab.IsWritable(m_sPrefab))
			return false;
		string path;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:" + m_sPrefab.GetPath(), path))
			return false;
		FileHandle file = FileIO.OpenFile(path + ".dependencies.json", FileMode.WRITE);
		if (!file)
			return false;
		string data = Encode();
		int written = file.Write(data);
		file.Close();
		return written == data.Length();
	}
}
#endif

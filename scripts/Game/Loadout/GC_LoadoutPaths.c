#ifdef WORKBENCH
//! GC Units authoring locations. Folder categories do not change native affiliation.
class GC_LoadoutPaths
{
	static const string CHARACTERS = "Prefabs/Characters/Factions/";
	static const string LEGACY = "Prefabs/Loadouts/";
	static const string ROOT_TEAM = "(Root)";

	//------------------------------------------------------------------------------------------------
	static bool Parts(ResourceName resource, out string faction, out string team)
	{
		string path = resource.GetPath();
		array<string> parts = {};
		path.Split("/", parts, true);
		int first;
		if (path.StartsWith(CHARACTERS) && parts.Count() >= 6)
		{
			faction = parts[3] + "/" + parts[4];
			first = 5;
		}
		else if (path.StartsWith(LEGACY) && parts.Count() >= 4)
		{
			faction = "Loadouts/" + parts[2];
			first = 3;
		}
		else
			return false;
		team = string.Empty;
		for (int i = first; i < parts.Count() - 1; i++)
		{
			if (!team.IsEmpty())
				team += "/";
			team += parts[i];
		}
		if (team.IsEmpty())
			team = ROOT_TEAM;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool ValidSegments(string value)
	{
		if (value.IsEmpty() || value.StartsWith("/") || value.EndsWith("/") || value.Contains("//") || value.Contains("\\"))
			return false;
		array<string> parts = {};
		value.Split("/", parts, true);
		foreach (string part : parts)
			if (!GC_LoadoutPrefab.ValidName(part))
				return false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static string Directory(string faction, string team)
	{
		if (!ValidSegments(faction) || (team != ROOT_TEAM && !ValidSegments(team)))
			return string.Empty;
		array<string> parts = {};
		faction.Split("/", parts, true);
		string directory;
		if (parts.Count() == 2 && parts[0] == "Loadouts")
			directory = LEGACY + parts[1];
		else if (parts.Count() == 2 && (parts[0] == "BLUFOR" || parts[0] == "OPFOR" || parts[0] == "INDFOR" || parts[0] == "CIV"))
			directory = CHARACTERS + faction;
		else
			return string.Empty;
		if (team != ROOT_TEAM)
			directory += "/" + team;
		return directory + "/";
	}

	//------------------------------------------------------------------------------------------------
	static bool Allowed(string relativePath)
	{
		if ((!relativePath.StartsWith(CHARACTERS) && !relativePath.StartsWith(LEGACY)) || !relativePath.EndsWith(".et"))
			return false;
		return ValidSegments(FilePath.StripExtension(relativePath));
	}

	//------------------------------------------------------------------------------------------------
	static bool Equipment(string path)
	{
		path.ToLower();
		return path.Contains("/weapons/") || path.Contains("/equipment/");
	}

	//------------------------------------------------------------------------------------------------
	static string EquipmentDirectory(ResourceName resource, bool weapon = false)
	{
		string directory = FilePath.StripFileName(resource.GetPath());
		string folded = directory;
		folded.ToLower();
		int equipment = folded.IndexOf("/equipment/");
		if (equipment < 0)
			equipment = folded.IndexOf("/weapons/");
		if (equipment >= 0)
			directory = directory.Substring(0, equipment + 1);
		if (weapon)
			return directory + "Weapons/";
		return directory + "Equipment/";
	}

	//------------------------------------------------------------------------------------------------
	static bool Template(string path)
	{
		path.ToLower();
		string name = FilePath.StripPath(path);
		return path.Contains("/deprecated/") || name.Contains("_base.") || name.Contains("_base_") || name.Contains("baseloadout");
	}
}
#endif

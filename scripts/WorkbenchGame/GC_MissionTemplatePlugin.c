#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Spawn GC Lobby Game Mode", description: "Place the GC Lobby game mode and a terrain AI world. From base terrain, opens New World to create a mission subscene first. Does not save.", wbModules: { "WorldEditor" }, category: "GC", awesomeFontCode: 0xF14E)]
class GC_MissionTemplatePlugin : WorkbenchPlugin
{
	protected static const ResourceName TEMPLATE = "{A79C4E0A43DEB821}Prefabs/MP/Modes/GC_LobbyGameMode.et";
	protected ref array<ResourceName> m_aAIWorldPrefabs = {};
	protected ref map<ResourceName, bool> m_mCandidatePrefabs = new map<ResourceName, bool>();
	protected ref array<string> m_aLoadedAddons = {};
	protected int m_iExistingPerceptionManagers;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api || !api.IsEditMode() || api.IsGameMode() || api.IsPrefabEditMode() || api.IsDoingEditAction())
		{
			Report("Open a mission world in edit mode before spawning its game mode.");
			return;
		}

		int subscene = api.GetCurrentSubScene();
		IEntitySource terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
		if (!terrain || !api.GetWorld())
		{
			Report("No loaded terrain was found. Nothing was placed.");
			return;
		}
		string terrainScene = NormalizeTerrainScene(api.GetWorld().GetSubSceneName(terrain.GetSubScene()));
		// Prefab-inherited children are not all present in GetEditorEntity's flat list.
		// Walk each root's source hierarchy, including roots in inherited layers.
		m_iExistingPerceptionManagers = 0;
		for (int i, count = api.GetEditorEntityCount(); i < count; i++)
		{
			IEntitySource source = api.GetEditorEntity(i);
			if (!source || source.GetParent())
				continue;
			IEntitySource conflict = FindConflict(source);
			if (conflict)
			{
				Report(string.Format("An existing %1 (%2, subscene %3) conflicts with the game mode. Nothing was placed; keep or remove that setup explicitly before trying again.", conflict.GetClassName(), conflict.GetName(), conflict.GetSubScene()));
				return;
			}
		}
		if (api.FindEntityByName("GC_LobbyGameMode") || api.FindEntityByName("GC_MissionAIWorld"))
		{
			Report("The game mode entity names are already in use. Nothing was placed.");
			return;
		}
		if (subscene == terrain.GetSubScene())
		{
			// Workbench's native dialog owns subscene creation and unsaved-work prompts.
			// The GameStateTransitions API is for the in-game editor, not Workbench.
			int previousCount = api.GetNumSubScenes();
			if (!editor.ExecuteAction({"File", "New World"}))
			{
				Report("Could not open New World. Create a subscene of the current world, then run this command again.");
				return;
			}
			api = editor.GetApi();
			terrain = SCR_WorldEditorToolHelper.GetTerrainEntitySource();
			if (!api || !api.GetWorld() || !terrain || api.GetNumSubScenes() <= previousCount
				|| api.GetCurrentSubScene() == terrain.GetSubScene()
				|| NormalizeTerrainScene(api.GetWorld().GetSubSceneName(terrain.GetSubScene())) != terrainScene)
				return;
			// Re-query the new scene, layer and conflicts after the native operation.
			Run();
			return;
		}
		int layer = api.GetCurrentEntityLayerId();
		if (layer < 0 || api.IsEntityLayerLockedHierarchy(subscene, layer))
		{
			Report("Select an unlocked mission layer first.");
			return;
		}
		if (!GC_ModWorkflow.BeforeTemplate(api.GetWorld().GetSubSceneName(subscene)))
			return;
		string notice;
		ResourceName aiPrefab = GetAIWorldPrefab(terrainScene);
		if (aiPrefab.IsEmpty())
			aiPrefab = DiscoverAIWorldPrefab(terrainScene, notice);
		if (aiPrefab.IsEmpty())
		{
			Report(notice + " Nothing was placed.");
			return;
		}

		Resource templateResource = Resource.Load(TEMPLATE);
		Resource aiResource = Resource.Load(aiPrefab);
		if (!templateResource || !templateResource.IsValid() || !aiResource || !aiResource.IsValid())
		{
			Report("The GC Lobby game mode or selected AI-world prefab could not be loaded. Nothing was placed.");
			return;
		}
		if (!api.BeginEntityAction("Spawn GC Lobby game mode"))
			return;

		IEntitySource root = api.CreateEntity(TEMPLATE, "GC_LobbyGameMode", layer, null, vector.Zero, vector.Zero);
		IEntitySource aiWorld;
		bool perceptionReady = root && (m_iExistingPerceptionManagers == 0 || DisableBundledPerception(api, root));
		if (perceptionReady)
			// Editing the game mode recreates its children, including any terrain navigation meshes.
			aiWorld = api.CreateEntity(aiPrefab, "GC_MissionAIWorld", layer, null, vector.Zero, vector.Zero);
		bool cleaned = true;
		if (root && !aiWorld)
			cleaned = api.DeleteEntity(root);
		api.EndEntityAction();
		if (!root || !aiWorld)
		{
			PrintFormat("GC | Lobby Game Mode | Placement result: root=%1 perceptionReady=%2 AI=%3 cleanup=%4", root != null, perceptionReady, aiWorld != null, cleaned);
			if (cleaned)
				Report("Game-mode placement failed. No game mode was retained.");
			else
				Report("AI-world placement failed and the partial game mode could not be removed. Inspect the world or Undo before trying again.");
			return;
		}
		api.ClearEntitySelection();
		api.AddToEntitySelection(root);
		PrintFormat("GC | Lobby Game Mode | Placed %1 alongside GC_LobbyGameMode for %2. Save the mission to keep it.", aiPrefab, terrainScene);
		if (m_iExistingPerceptionManagers > 0)
			PrintFormat("GC | Lobby Game Mode | Reused %1 existing perception manager(s); disabled the bundled child in this instance.", m_iExistingPerceptionManagers);
		if (!notice.IsEmpty())
			Report(notice);
	}

	//------------------------------------------------------------------------------------------------
	//! Names only prioritize candidates; the resolved root type decides eligibility.
	protected ResourceName DiscoverAIWorldPrefab(string terrainScene, out string notice)
	{
		terrainScene.Replace("\\", "/");
		string terrainName = FilePath.StripExtension(FilePath.StripPath(terrainScene));
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		m_aLoadedAddons.Clear();
		foreach (string addon : addons)
			m_aLoadedAddons.Insert(GameProject.GetAddonID(addon));
		m_mCandidatePrefabs.Clear();
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.fileExtensions = {"et"};
		if (!ResourceDatabase.SearchResources(filter, OnPrefabResource))
		{
			notice = "Could not enumerate prefab resources.";
			return ResourceName.Empty;
		}
		array<ResourceName> candidates = {};
		foreach (ResourceName name, bool found : m_mCandidatePrefabs)
			candidates.Insert(name);
		m_mCandidatePrefabs.Clear();
		terrainName.ToLower();
		WBProgressDialog progress;
		for (int pass; pass < 2; pass++)
		{
			if (pass == 1)
				progress = new WBProgressDialog("Finding AI worlds in loaded addons", Workbench.GetModule(WorldEditor));
			m_aAIWorldPrefabs.Clear();
			foreach (int index, ResourceName candidate : candidates)
			{
				if (progress && index % 100 == 0)
					progress.SetProgress((index + 1.0) / candidates.Count());
				string path = candidate.GetPath();
				path.ToLower();
				bool nameMatches = !terrainName.IsEmpty() && path.Contains(terrainName);
				if ((pass == 0 && !nameMatches) || (pass == 1 && nameMatches))
					continue;
				OnAIWorldPrefab(candidate, "");
			}
			if (m_aAIWorldPrefabs.IsEmpty())
				continue;
			m_aAIWorldPrefabs.Sort();
			ResourceName selected = m_aAIWorldPrefabs[0];
			string scope = "with paths matching the terrain name";
			if (pass == 1)
				scope = "across the loaded addons (none matched the terrain name)";
			notice = string.Format("Automatically selected %1 from %2 AI-world prefab(s) %3. Check GC_MissionAIWorld and its navigation meshes for terrain %4 before saving or playing. Replace it if needed; prefab type and name do not guarantee terrain compatibility.", selected, m_aAIWorldPrefabs.Count(), scope, terrainScene);
			return selected;
		}
		notice = "No loadable AI-world prefab was found in the loaded addons.";
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPrefabResource(ResourceName name, string exactPath)
	{
		int end = exactPath.IndexOf(":");
		if (!exactPath.StartsWith("$") || end <= 1 || !m_aLoadedAddons.Contains(exactPath.Substring(1, end - 1)))
			return;
		m_mCandidatePrefabs.Set(name, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAIWorldPrefab(ResourceName name, string exactPath)
	{
		if (m_aAIWorldPrefabs.Contains(name))
			return;
		Resource resource = BaseContainerTools.LoadContainer(name);
		if (!resource || !resource.IsValid())
			return;
		BaseContainer source = resource.GetResource().ToBaseContainer();
		if (!source)
			return;
		typename type = source.GetClassName().ToType();
		if (type && type.IsInherited(AIWorld))
			m_aAIWorldPrefabs.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntitySource FindConflict(IEntitySource source)
	{
		EntityFlags flags;
		if (source.Get("Flags", flags) && (flags & (EntityFlags.DISABLED | EntityFlags.EDITOR_ONLY)))
			return null;
		typename type = source.GetClassName().ToType();
		if (type && type.IsInherited(PerceptionManager))
			m_iExistingPerceptionManagers++;
		if (type && (type.IsInherited(BaseGameMode) || type.IsInherited(AIWorld) || type.IsInherited(FactionManager)
			|| type.IsInherited(RadioManagerEntity) || type.IsInherited(ScriptedChatEntity)))
			return source;
		for (int i, count = source.GetNumChildren(); i < count; i++)
		{
			IEntitySource conflict = FindConflict(source.GetChild(i));
			if (conflict)
				return conflict;
		}
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! DISABLED prevents spawning anywhere; preserve the child and prefab inheritance in this instance.
	protected bool DisableBundledPerception(WorldEditorAPI api, IEntitySource root)
	{
		for (int i = root.GetNumChildren() - 1; i >= 0; i--)
		{
			IEntitySource child = root.GetChild(i);
			typename type = child.GetClassName().ToType();
			if (type && type.IsInherited(PerceptionManager))
			{
				EntityFlags flags;
				if (!child.Get("Flags", flags))
					return false;
				flags |= EntityFlags.DISABLED;
				if (!api.SetVariableValue(child, null, "Flags", flags.ToString()))
					return false;
			}
			else if (!DisableBundledPerception(api, child))
				return false;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Exact terrain-owner world mapping; mission filenames and directory substrings are not map identifiers.
	protected ResourceName GetAIWorldPrefab(string terrainScene)
	{
		terrainScene = NormalizeTerrainScene(terrainScene);
		switch (terrainScene)
		{
			case "worlds/eden/eden.ent":
				return "{70CCCF16487C927F}Prefabs/AI/SCR_AIWorld_Eden.et";
			case "worlds/arland/arland.ent":
				return "{01DC74137CFDDB6A}Prefabs/AI/SCR_AIWorld_Arland.et";
			case "worlds/cain/cain.ent":
				return "{672531FAC0834C06}Prefabs/AI/SCR_AIWorld_Cain.et";
		}
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Workbench drops the $addon: prefix when the same world becomes an inherited subscene.
	protected string NormalizeTerrainScene(string scene)
	{
		int end = scene.IndexOf(":");
		if (scene.StartsWith("$") && end > 1)
			scene = scene.Substring(end + 1, scene.Length() - end - 1);
		scene.Replace("\\", "/");
		scene.ToLower();
		return scene;
	}

	//------------------------------------------------------------------------------------------------
	protected void Report(string message)
	{
		Print("GC | Lobby Game Mode | " + message, LogLevel.WARNING);
		Workbench.Dialog("GC Lobby Game Mode", message);
	}
}
#endif

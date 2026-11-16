#ifdef WORKBENCH
modded class GC_LoadoutFilesMenu
{
	ref map<ResourceName, ref GC_LoadoutVariants> m_GC_NestedLists;

	override protected bool ReadVariants()
	{
		if (m_GC_NestedLists && m_GC_NestedLists.Contains(m_sSelected))
		{
			if (!CanChangeVariantOwner(m_sSelected))
				return false;
			m_Variants = m_GC_NestedLists.Get(m_sSelected);
			return true;
		}
		return super.ReadVariants();
	}
	string GC_TestLoadout(string action, string value)
	{
		if (action == "nestedFixture")
		{
			// Presentation-only graph using real resources; never writes user prefabs.
			Widget childWidget = GetRootWidget().FindAnyWidget("SavedRows").GetChildren();
			while (childWidget)
			{
				Widget next = childWidget.GetSibling();
				childWidget.RemoveFromHierarchy();
				childWidget = next;
			}
			m_aRows.Clear();
			m_aRowResources.Clear();
			m_aRowDepths.Clear();
			m_aParentRows.Clear();
			m_aVariantRows.Clear();
			m_aVariantOwners.Clear();
			m_aVariantIndices.Clear();
			m_aVariantPrefabs.Clear();
			m_aResources.Clear();
			m_mParents.Clear();
			ResourceName root = "{6AE1251DC11B53F6}Prefabs/Loadouts/US/test/test.et";
			ResourceName first = "{6AE1326839A9BF9D}Prefabs/Loadouts/US/test/US_variant_test_1.et";
			ResourceName second = "{E45F1E163F5CA080}Prefabs/Characters/Factions/BLUFOR/US_Army/Deprecated/Character_US_SL.et";
			ResourceName nested = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
			m_aResources = {root, first, second, nested};
			m_mParents.Set(root, string.Empty);
			m_mParents.Set(first, root);
			m_mParents.Set(second, root);
			m_mParents.Set(nested, first);
			m_Variants = new GC_LoadoutVariants();
			m_sVariantOwner = root;
			SCR_EditableEntityVariant firstVariant = new SCR_EditableEntityVariant();
			firstVariant.m_sVariantPrefab = first;
			m_Variants.m_aEntries.Insert(firstVariant);
			SCR_EditableEntityVariant secondVariant = new SCR_EditableEntityVariant();
			secondVariant.m_sVariantPrefab = second;
			m_Variants.m_aEntries.Insert(secondVariant);
			map<ResourceName, ref GC_LoadoutVariants> lists = new map<ResourceName, ref GC_LoadoutVariants>();
			lists.Insert(root, m_Variants);
			GC_LoadoutVariants childVariants = new GC_LoadoutVariants();
			SCR_EditableEntityVariant nestedVariant = new SCR_EditableEntityVariant();
			nestedVariant.m_sVariantPrefab = nested;
			childVariants.m_aEntries.Insert(nestedVariant);
			lists.Insert(first, childVariants);
			m_GC_NestedLists = lists;
			array<ResourceName> visited = {};
			AddPrefabRows(root, 0, lists, visited);
			SelectPrefab(root);
			return "synthetic nested graph ready";
		}
		if (action == "branchWidgets")
		{
			Widget branch = m_aRows[0].GetRootWidget().FindAnyWidget("TreeBranch");
			return GC_BranchWidgetSizes(branch, "");
		}
		if (action == "state")
			return "selected=" + m_sSelected + " current=" + m_bCurrentGear + " status=" + m_Status.GetText() + " mods=" + TextWidget.Cast(GetRootWidget().FindAnyWidget("Dependencies")).GetText();
		if (action == "removeWeaponPart")
		{
			SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(m_Owner.FindComponent(SCR_InventoryStorageManagerComponent));
			set<BaseInventoryStorageComponent> storages = new set<BaseInventoryStorageComponent>();
			SCR_PlayerArsenalLoadout.FindStorageComponents(m_Owner, storages);
			foreach (BaseInventoryStorageComponent storage : storages)
				for (int i; i < storage.GetSlotsCount(); i++)
				{
					IEntity weapon = storage.GetSlot(i).GetAttachedEntity();
					if (!weapon || !weapon.FindComponent(BaseWeaponComponent))
						continue;
					set<BaseInventoryStorageComponent> attachments = new set<BaseInventoryStorageComponent>();
					SCR_PlayerArsenalLoadout.FindStorageComponents(weapon, attachments);
					foreach (BaseInventoryStorageComponent attachment : attachments)
						for (int j; j < attachment.GetSlotsCount(); j++)
						{
							IEntity item = attachment.GetSlot(j).GetAttachedEntity();
							if (item && !SCR_ResourceNameUtils.GetPrefabName(item).Contains("Magazines/") && !SCR_ResourceNameUtils.GetPrefabName(item).Contains("ACE_Overheating"))
							{
								string path = SCR_ResourceNameUtils.GetPrefabName(item);
								return manager.TryDeleteItem(item).ToString() + " " + path;
							}
						}
				}
			return "no weapon part";
		}
		if (action == "waitResource")
		{
			ResourceManager manager = Workbench.GetModule(ResourceManager);
			string path;
			Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:" + m_sSelected.GetPath(), path);
			return manager.WaitForFile(path, 2000).ToString();
		}
		if (action == "paths")
		{
			array<string> names = {"../escape", "CON", "NUL.txt", "name.", "good (name)", "name/child"};
			string result;
			foreach (string name : names)
				result += name + "=" + GC_LoadoutPrefab.ValidName(name) + "\n";
			result += "escape=" + GC_LoadoutPaths.Directory("BLUFOR/US_Army", "../Lobby") + "\n";
			result += "nestedEquipment=" + GC_LoadoutPaths.EquipmentDirectory("Prefabs/Characters/Factions/BLUFOR/US_Army/Team/Equipment/Gun.et");
			return result;
		}
		if (action == "catalogue")
			return "indexed=" + m_aLibraryResources.Count() + " resolved=" + m_aResources.Count() + " rows=" + m_aRows.Count() + " faction=" + m_Faction.GetText() + " team=" + m_Group.GetText();
		if (action == "folder")
		{
			array<string> parts = {};
			value.Split("|", parts, true);
			m_Faction.SetText(parts[0]);
			m_Group.SetText(parts[1]);
			RefreshTeamChoices();
			ApplyScopeFilter();
			return GC_TestLoadout("catalogue", "");
		}
		if (action == "filter")
		{
			m_Search.SetText(value);
			m_SearchEvents.GetOnChange().Invoke(m_Search);
			return GC_TestLoadout("tree", "");
		}
		if (action == "filename")
		{
			m_Name.SetText(value);
			return GC_LoadoutPrefab.Destination(m_Faction.GetText(), m_Group.GetText(), value);
		}
		if (action == "scope")
		{
			m_Faction.SetText("Loadouts/US");
			m_Group.SetText(value);
			m_Group.m_OnFilterChanged.Invoke();
			return "scope changed";
		}
		if (action == "tree")
		{
			string result = "expanded=" + m_sExpandedPrefab + " parent=" + m_sParentPrefab + "\n";
			foreach (int row, SCR_ButtonBaseComponent rootRow : m_aRows)
				if (rootRow.GetRootWidget().IsVisible())
					result += "root " + m_aRowResources[row] + "\n";
			foreach (int index, SCR_ButtonBaseComponent variant : m_aVariantRows)
				if (variant.GetRootWidget().IsVisible())
					result += "variant " + m_aVariantOwners[index] + " #" + m_aVariantIndices[index] + "\n";
			return result;
		}
		if (action == "select")
		{
			foreach (int index, ResourceName prefab : m_aRowResources)
			{
				if (prefab.GetPath().Contains(value))
				{
					m_aRows[index].m_OnClicked.Invoke(m_aRows[index]);
					return "selected";
				}
			}
			return "not found";
		}
		if (action == "variant")
		{
			foreach (int rowIndex, ResourceName owner : m_aVariantOwners)
			{
				if (owner == m_sSelected && m_aVariantIndices[rowIndex] == value.ToInt())
				{
					m_aVariantRows[rowIndex].m_OnClicked.Invoke(m_aVariantRows[rowIndex]);
					return "variant selected";
				}
			}
			return "variant not found";
		}
		if (action == "variantPrefab")
		{
			foreach (int choice, ResourceName resource : m_aVariantChoices)
			{
				if (resource.GetPath().Contains(value))
				{
					m_VariantPicker.m_OnChanged.Invoke(m_VariantPicker, choice);
					return "variant changed";
				}
			}
			return "variant choice not found";
		}
		if (action == "name")
		{
			m_Faction.SetText("Loadouts/US");
			m_Group.SetText("DependencyTest");
			m_Name.SetText(value);
			return "named";
		}
		SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(action, GetRootWidget());
		if (!button)
			return "missing button";
		button.m_OnClicked.Invoke(button);
		return "dispatched";
	}

	string GC_BranchWidgetSizes(Widget widget, string indent)
	{
		float width, height;
		widget.GetScreenSize(width, height);
		string result = indent + widget.GetName() + " " + widget.Type().ToString() + " " + width + "x" + height + "\n";
		Widget child = widget.GetChildren();
		while (child)
		{
			result += GC_BranchWidgetSizes(child, indent + " ");
			child = child.GetSibling();
		}
		return result;
	}
}

modded class SCR_GameModeHealthSettings
{
	string m_GC_SearchResults;
	void GC_TestSearchResult(ResourceName resource, string path)
	{
		m_GC_SearchResults += resource + "=" + path + "\n";
	}
	string GC_TestLoadout(string action, string value)
	{
		if (action == "search")
		{
			SearchResourcesFilter filter = new SearchResourcesFilter();
			filter.searchStr = { value };
			filter.fileExtensions = { "et" };
			m_GC_SearchResults = string.Empty;
			bool success = ResourceDatabase.SearchResources(filter, GC_TestSearchResult);
			return success.ToString() + " " + m_GC_SearchResults;
		}
		if (action == "ancestry")
		{
			Resource resource = BaseContainerTools.LoadContainer(value);
			BaseContainer source = resource.GetResource().ToBaseContainer();
			string result;
			while (source)
			{
				array<string> addons = {};
				source.GetSourceAddons(addons);
				result += source.GetResourceName() + ":";
				foreach (string id : addons)
					result += " " + id;
				result += "\n";
				source = source.GetAncestor();
			}
			return result;
		}
		if (action == "spawn")
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = "1195 39 2715";
			params.Transform[3][1] = GetGame().GetWorld().GetSurfaceY(1195, 2715);
			if (value.IsEmpty())
				value = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
			Resource resource = Resource.Load(value);
			IEntity body = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
			if (!body)
				return "spawn failed";
			AIControlComponent ai = AIControlComponent.Cast(body.FindComponent(AIControlComponent));
			if (ai)
				ai.DeactivateAI();
			GetGame().GetPlayerController().SetControlledEntity(body);
			return "spawned";
		}
		if (action == "open")
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.GC_LoadoutFilesMenu, 0, true, false);
			return "opened";
		}
		if (action == "scan" || action == "summary" || action == "visibleMods")
		{
			GC_LoadoutDependencies report = GC_LoadoutDependencies.Scan(value, true);
			if (action == "scan")
				return report.Encode();
			string summary = "error=" + report.m_sError + " unresolved=" + report.m_iUnresolved + " unexpanded=" + report.m_iUnexpanded;
			foreach (GC_LoadoutDependencyMod mod : report.m_aMods)
			{
				if (action == "visibleMods" && report.IsImplied(mod))
					continue;
				summary += "\n" + mod.m_sId + " " + mod.m_sGuid;
			}
			return summary;
		}
		GC_LoadoutFilesMenu menu = GC_LoadoutFilesMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu));
		if (!menu)
			return "menu absent";
		return menu.GC_TestLoadout(action, value);
	}
}
#endif

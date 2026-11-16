#ifdef WORKBENCH
modded class GC_LoadoutFilesMenu
{
	string GC_TestLoadout(string action, string value)
	{
		if (action == "state")
			return "selected=" + m_sSelected + " current=" + m_bCurrentGear + " status=" + m_Status.GetText() + " mods=" + TextWidget.Cast(GetRootWidget().FindAnyWidget("Dependencies")).GetText();
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
			m_Faction.SetText("US");
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
		if (action == "scan" || action == "summary")
		{
			GC_LoadoutDependencies report = GC_LoadoutDependencies.Scan(value, true);
			if (action == "scan")
				return report.Encode();
			string summary = "error=" + report.m_sError + " unresolved=" + report.m_iUnresolved + " unexpanded=" + report.m_iUnexpanded;
			foreach (GC_LoadoutDependencyMod mod : report.m_aMods)
				summary += "\n" + mod.m_sId + " " + mod.m_sGuid;
			return summary;
		}
		GC_LoadoutFilesMenu menu = GC_LoadoutFilesMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.GC_LoadoutFilesMenu));
		if (!menu)
			return "menu absent";
		return menu.GC_TestLoadout(action, value);
	}
}
#endif

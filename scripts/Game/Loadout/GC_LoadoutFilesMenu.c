//! Modal native menu; file authoring is compiled only for Workbench.
class GC_LoadoutFilesMenu : ChimeraMenuBase
{
#ifdef WORKBENCH
	protected IEntity m_Owner;
	protected GC_LoadoutScopeSelector m_Faction;
	protected GC_LoadoutScopeSelector m_Group;
	protected string m_sScopeFaction;
	protected bool m_bScopeFactionChosen;
	protected EditBoxWidget m_Name;
	protected EditBoxWidget m_Search;
	protected SCR_EventHandlerComponent m_SearchEvents;
	protected GC_LoadoutIconSelector m_Icon;
	protected TextWidget m_Status;
	protected ItemPreviewWidget m_Preview;
	protected ItemPreviewManagerEntity m_PreviewManager;
	protected ResourceName m_sSelected;
	protected ResourceName m_sOverwriteTarget;
	protected SCR_ConfigurableDialogUi m_OverwriteDialog;
	protected bool m_bCurrentGear = true;
	protected ref array<ResourceName> m_aResources = {};
	protected ref array<ResourceName> m_aLibraryResources = {};
	protected ref map<string, ResourceName> m_mLibraryByGuid = new map<string, ResourceName>();
	protected string m_sBuiltScope;
	protected ref array<SCR_ButtonBaseComponent> m_aButtons = {};
	protected ref array<SCR_ButtonBaseComponent> m_aRows = {};
	protected ref array<ResourceName> m_aRowResources = {};
	protected ref array<SCR_ButtonBaseComponent> m_aVariantRows = {};
	protected ref array<ResourceName> m_aVariantOwners = {};
	protected ref array<int> m_aVariantIndices = {};
	protected ref array<ResourceName> m_aVariantPrefabs = {};
	protected ref GC_LoadoutVariants m_Variants;
	protected ResourceName m_sVariantOwner;
	protected int m_iVariant = -1;
	protected bool m_bVariantsDirty;
	protected SCR_ComboBoxComponent m_VariantPicker;
	protected ref array<ResourceName> m_aVariantChoices = {};
	protected SCR_ConfigurableDialogUi m_DeleteDialog;
	protected bool m_bVariantSaveFailed;
	protected ref GC_LoadoutDependencies m_Dependencies;
	protected ResourceName m_sDependencyTarget;
	protected bool m_bDependencyVariants;
	protected ResourceName m_sExpandedPrefab;
	protected ResourceName m_sParentPrefab;
	protected ref map<ResourceName, ResourceName> m_mParents = new map<ResourceName, ResourceName>();
	protected ref array<SCR_ButtonTextComponent> m_aParentRows = {};
	protected ref array<int> m_aRowDepths = {};

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_Owner = GetGame().GetPlayerController().GetControlledEntity();
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
		{
			Close();
			return;
		}
		Widget root = GetRootWidget();
		m_Faction = GC_LoadoutScopeSelector.Cast(root.FindAnyWidget("Faction").FindHandler(GC_LoadoutScopeSelector));
		m_Group = GC_LoadoutScopeSelector.Cast(root.FindAnyWidget("Group").FindHandler(GC_LoadoutScopeSelector));
		m_Faction.m_OnFilterChanged.Insert(OnFactionFilter);
		m_Group.m_OnFilterChanged.Insert(ApplyScopeFilter);
		m_Name = EditBoxWidget.Cast(root.FindAnyWidget("Name"));
		m_Search = EditBoxWidget.Cast(root.FindAnyWidget("LoadoutSearch"));
		m_SearchEvents = SCR_EventHandlerComponent.Cast(m_Search.FindHandler(SCR_EventHandlerComponent));
		m_SearchEvents.GetOnChange().Insert(OnSearch);
		m_Icon = GC_LoadoutIconSelector.Cast(root.FindAnyWidget("RoleSelector").FindHandler(GC_LoadoutIconSelector));
		m_Icon.m_OnChanged.Insert(OnIconChanged);
		m_Status = TextWidget.Cast(root.FindAnyWidget("Status"));
		m_Preview = ItemPreviewWidget.Cast(root.FindAnyWidget("LoadoutPreview"));
		ChimeraWorld world = GetGame().GetWorld();
		if (world)
			m_PreviewManager = world.GetItemPreviewManager();
		m_Group.SetPlaceholderText("My Team");
		m_Name.SetPlaceholderText("Base");
		m_VariantPicker = SCR_ComboBoxComponent.Cast(root.FindAnyWidget("VariantPrefab").FindHandler(SCR_ComboBoxComponent));
		m_VariantPicker.m_OnChanged.Insert(OnVariantPrefabChanged);
		array<string> actions = {"Close", "Load", "Save", "Inherit", "Duplicate", "Current", "AddVariant", "DeleteVariant", "VariantLoad", "VariantSave"};
		foreach (string action : actions)
		{
			SCR_ButtonTextComponent button = SCR_ButtonTextComponent.GetButtonText(action, root);
			button.SetText("#GC-Loadout_" + action);
			if (action == "VariantLoad")
				button.SetText("#GC-Loadout_Load");
			else if (action == "VariantSave")
				button.SetText("#GC-Loadout_SaveVariants");
			else if (action == "Save")
				button.SetText("#GC-Loadout_SaveGear");
			else if (action == "Inherit")
				button.SetText("#GC-Loadout_CreateChild");
			else if (action == "DeleteVariant")
				button.SetText("#GC-Loadout_RemoveVariant");
			button.m_OnClicked.Insert(OnAction);
			m_aButtons.Insert(button);
		}
		m_sSelected = GC_PrefabAuthoringSession.GetEditingPrefab();
		RefreshList();
		SetFields(m_sSelected);
		RefreshTeamChoices();
		ApplyScopeFilter();
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	protected string Label(ResourceName resource)
	{
		if (resource.IsEmpty())
			return WidgetManager.Translate("#GC-Loadout_Current");
		return FilePath.StripExtension(FilePath.StripPath(resource.GetPath()));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFields(ResourceName resource)
	{
		m_Name.SetText(Label(resource));
		GC_LoadoutMetadata metadata = GC_LoadoutMetadata.Read(resource);
		m_Icon.Populate(metadata);
		OnIconChanged(m_Icon, m_Icon.GetCurrentIndex());
		string faction, team;
		if (!GC_LoadoutPaths.Parts(resource, faction, team))
			return;
		m_Faction.SetText(faction);
		m_Group.SetText(team);
		RefreshTeamChoices();
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnIconChanged(SCR_ComboBoxComponent combo, int index)
	{
		GC_LoadoutIconSelector.DrawIcon(ImageWidget.Cast(GetRootWidget().FindAnyWidget("SelectedIcon")), m_Icon.GetSelectedIcon());
	}

	//------------------------------------------------------------------------------------------------
	protected GC_LoadoutMetadata GetMetadata()
	{
		GC_LoadoutMetadata icon = m_Icon.GetSelectedIcon();
		if (!icon)
			return null;
		GC_LoadoutMetadata metadata = new GC_LoadoutMetadata();
		metadata.m_sName = m_Name.GetText();
		GC_LoadoutMetadata original = GC_LoadoutMetadata.Read(m_sSelected);
		if (original)
			metadata.m_sFaction = original.m_sFaction;
		else
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(m_Owner.FindComponent(FactionAffiliationComponent));
			if (affiliation && affiliation.GetAffiliatedFaction())
				metadata.m_sFaction = affiliation.GetAffiliatedFaction().GetFactionKey();
		}
		metadata.m_sIcon = icon.m_sIcon;
		metadata.m_sIconName = icon.m_sIconName;
		return metadata;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshList()
	{
		m_aLibraryResources.Clear();
		m_mLibraryByGuid.Clear();
		array<string> roots = {GC_LoadoutPaths.CHARACTERS, GC_LoadoutPaths.LEGACY};
		foreach (string relative : roots)
		{
			string directory;
			if (Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:" + relative, directory, false))
				FileIO.FindFiles(OnFile, directory, ".et");
		}
		m_sBuiltScope = "!refresh";
		RefreshScopeChoices();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildRows()
	{
		foreach (SCR_ButtonTextComponent parentRow : m_aParentRows)
		{
			parentRow.GetRootWidget().RemoveFromHierarchy();
		}
		m_aParentRows.Clear();
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
		{
			variantRow.m_OnClicked.Remove(OnVariantSelect);
			variantRow.GetRootWidget().RemoveFromHierarchy();
		}
		m_aVariantRows.Clear();
		m_aVariantOwners.Clear();
		m_aVariantIndices.Clear();
		m_aVariantPrefabs.Clear();
		foreach (SCR_ButtonBaseComponent row : m_aRows)
		{
			row.m_OnClicked.Remove(OnSelect);
			row.GetRootWidget().RemoveFromHierarchy();
		}
		m_aRows.Clear();
		m_aRowResources.Clear();
		m_aRowDepths.Clear();
		m_aResources.Clear();
		m_mParents.Clear();
		// Index paths globally, but resolve native prefabs/widgets only for one team.
		if (m_Faction.GetText().IsEmpty() || m_Group.GetText().IsEmpty())
			return;
		foreach (ResourceName candidate : m_aLibraryResources)
			if (MatchesResourceScope(candidate) && (!GC_LoadoutPaths.Template(candidate.GetPath()) || candidate == m_sSelected) && GC_LoadoutVariants.Load(candidate))
				m_aResources.Insert(candidate);
		array<string> names = {};
		map<string, ResourceName> resources = new map<string, ResourceName>();
		foreach (ResourceName resource : m_aResources)
		{
			string key = Label(resource) + " " + resource;
			key.ToLower();
			names.Insert(key);
			resources.Insert(key, resource);
		}
		names.Sort();
		m_aResources.Clear();
		foreach (string key : names)
			m_aResources.Insert(resources.Get(key));
		map<ResourceName, ref GC_LoadoutVariants> lists = new map<ResourceName, ref GC_LoadoutVariants>();
		for (int resourceIndex; resourceIndex < m_aResources.Count(); resourceIndex++)
		{
			ResourceName resource = m_aResources[resourceIndex];
			GC_LoadoutVariants variants = GC_LoadoutVariants.Load(resource);
			if (m_Variants && resource == m_sVariantOwner)
				variants = m_Variants;
			lists.Insert(resource, variants);
			ResourceName parent = ParentPrefab(resource);
			ResourceName savedParent = m_mLibraryByGuid.Get(SCR_ConfigHelper.GetGUID(parent));
			if (!savedParent.IsEmpty())
			{
				parent = savedParent;
				if (MatchesResourceScope(parent) && !m_aResources.Contains(parent))
					m_aResources.Insert(parent);
			}
			m_mParents.Set(resource, parent);
		}
		array<ResourceName> visited = {};
		foreach (ResourceName resource : m_aResources)
			if (!m_aResources.Contains(m_mParents.Get(resource)))
				AddPrefabRows(resource, 0, lists, visited);
		// Malformed cyclic inheritance must not hide a saved file from the browser.
		foreach (ResourceName resource : m_aResources)
			if (!visited.Contains(resource))
				AddPrefabRows(resource, 0, lists, visited);
		RefreshRelationship();
	}

	//------------------------------------------------------------------------------------------------
	protected void AddPrefabRows(ResourceName resource, int depth, map<ResourceName, ref GC_LoadoutVariants> lists, array<ResourceName> visited)
	{
		if (visited.Contains(resource) || depth >= 128)
			return;
		visited.Insert(resource);
		Widget container = GetRootWidget().FindAnyWidget("SavedRows");
		Widget parentWidget = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", container);
		parentWidget.SetName("Parent");
		AlignableSlot.SetPadding(parentWidget, Math.Min(depth + 1, 5) * 20, 0, 0, 2);
		SCR_ButtonTextComponent link = SCR_ButtonTextComponent.Cast(parentWidget.FindHandler(SCR_ButtonTextComponent));
		link.SetEnabled(false);
		m_aParentRows.Insert(link);

		Widget widget = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", container);
		AlignableSlot.SetPadding(widget, Math.Min(depth, 4) * 20, 0, 0, 2);
		SCR_ButtonTextComponent row = SCR_ButtonTextComponent.Cast(widget.FindHandler(SCR_ButtonTextComponent));
		row.SetText(Label(resource));
		bool numbered;
		GC_LoadoutVariants parentVariants = lists.Get(m_mParents.Get(resource));
		if (parentVariants)
			foreach (int variantIndex, SCR_EditableEntityVariant parentVariant : parentVariants.m_aEntries)
				if (SCR_ConfigHelper.GetGUID(parentVariant.m_sVariantPrefab) == SCR_ConfigHelper.GetGUID(resource))
				{
					row.SetText((variantIndex + 1).ToString());
					numbered = true;
				}
		row.SetToggleable(true);
		SetSelection(row, !m_bCurrentGear && m_iVariant < 0 && resource == m_sSelected);
		if (!numbered)
			SetRoleIcon(widget, resource);
		row.m_OnClicked.Insert(OnSelect);
		m_aRows.Insert(row);
		m_aRowResources.Insert(resource);
		m_aRowDepths.Insert(depth);

		GC_LoadoutVariants variants = lists.Get(resource);
		if (variants)
		{
			foreach (int index, SCR_EditableEntityVariant entry : variants.m_aEntries)
			{
				Widget child = GetGame().GetWorkspace().CreateWidgets("{DC391DA320AE3002}UI/layouts/Inventory/GC_LoadoutButton.layout", container);
				AlignableSlot.SetPadding(child, Math.Min(depth + 1, 5) * 20, 0, 0, 2);
				SCR_ButtonTextComponent variantRow = SCR_ButtonTextComponent.Cast(child.FindHandler(SCR_ButtonTextComponent));
				variantRow.SetText((index + 1).ToString());
				variantRow.SetToggleable(true);
				SetSelection(variantRow, resource == m_sVariantOwner && index == m_iVariant);
				variantRow.m_OnClicked.Insert(OnVariantSelect);
				m_aVariantRows.Insert(variantRow);
				m_aVariantOwners.Insert(resource);
				m_aVariantIndices.Insert(index);
				m_aVariantPrefabs.Insert(entry.m_sVariantPrefab);
				// Keep the represented prefab's descendants immediately after its number.
				foreach (ResourceName variantChild : m_aResources)
					if (m_mParents.Get(variantChild) == resource && SCR_ConfigHelper.GetGUID(variantChild) == SCR_ConfigHelper.GetGUID(entry.m_sVariantPrefab))
						AddPrefabRows(variantChild, depth + 1, lists, visited);
			}
		}
		foreach (ResourceName childResource : m_aResources)
			if (m_mParents.Get(childResource) == resource)
				AddPrefabRows(childResource, depth + 1, lists, visited);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName ParentPrefab(ResourceName resourceName)
	{
		Resource resource = Resource.Load(resourceName);
		if (!resource || !resource.IsValid())
			return string.Empty;
		BaseContainer source = resource.GetResource().ToEntitySource();
		for (int depth; source && depth < 128; depth++)
		{
			ResourceName parent = source.GetResourceName();
			if (!parent.IsEmpty() && SCR_ConfigHelper.GetGUID(parent) != SCR_ConfigHelper.GetGUID(resourceName))
				return parent;
			source = source.GetAncestor();
		}
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ScopeParts(ResourceName resource, out string faction, out string team)
	{
		return GC_LoadoutPaths.Parts(resource, faction, team);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddScopeChoice(array<string> values, string value)
	{
		if (value.IsEmpty())
			return;
		string folded = value;
		folded.ToLower();
		foreach (string existing : values)
		{
			existing.ToLower();
			if (existing == folded)
				return;
		}
		values.Insert(value);
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesScope(string value, GC_LoadoutScopeSelector selector)
	{
		string query = selector.GetText();
		query.ToLower();
		value.ToLower();
		if (query.IsEmpty())
			return true;
		if (selector.HasChoice())
			return value == query;
		return value.Contains(query);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshScopeChoices()
	{
		array<string> values = {};
		foreach (ResourceName resource : m_aLibraryResources)
		{
			string factionName, team;
			if (ScopeParts(resource, factionName, team))
				AddScopeChoice(values, factionName);
		}
		values.Sort();
		m_Faction.Populate(values, "#GC-Loadout_AllFactions");
		RefreshTeamChoices();
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshTeamChoices()
	{
		m_sScopeFaction = m_Faction.GetText();
		m_bScopeFactionChosen = m_Faction.HasChoice();
		array<string> values = {};
		foreach (ResourceName resource : m_aLibraryResources)
		{
			string faction, team;
			if (ScopeParts(resource, faction, team) && MatchesScope(faction, m_Faction))
				AddScopeChoice(values, team);
		}
		values.Sort();
		m_Group.Populate(values, "#GC-Loadout_AllTeams");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFactionFilter()
	{
		GetGame().GetCallqueue().Remove(ApplyFactionFilter);
		GetGame().GetCallqueue().CallLater(ApplyFactionFilter, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyFactionFilter()
	{
		// Focus commits can report the same edit again while the team field is typing.
		if (m_sScopeFaction == m_Faction.GetText() && m_bScopeFactionChosen == m_Faction.HasChoice())
		{
			ApplyScopeFilter();
			return;
		}
		m_Group.CloseList();
		m_Group.SetText("");
		RefreshTeamChoices();
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected bool MatchesResourceScope(ResourceName resource)
	{
		string faction, team;
		if (!ScopeParts(resource, faction, team))
			return false;
		string chosenFaction = m_Faction.GetText();
		string chosenTeam = m_Group.GetText();
		faction.ToLower();
		team.ToLower();
		chosenFaction.ToLower();
		chosenTeam.ToLower();
		return faction == chosenFaction && team == chosenTeam;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyScopeFilter()
	{
		string scope = m_Faction.GetText() + "|" + m_Group.GetText();
		if (scope != m_sBuiltScope)
		{
			m_sBuiltScope = scope;
			BuildRows();
		}
		string query = m_Search.GetText();
		GetRootWidget().FindAnyWidget("SearchHint").SetVisible(query.IsEmpty());
		query.ToLower();
		set<ResourceName> matching = new set<ResourceName>();
		foreach (ResourceName resource : m_aResources)
		{
			if (!MatchesResourceScope(resource))
				continue;
			string name = Label(resource);
			name.ToLower();
			if ((!query.IsEmpty() && !name.Contains(query)) || (GC_LoadoutPaths.Template(resource.GetPath()) && resource != m_sSelected))
				continue;
			ResourceName branch = resource;
			for (int depth; m_aResources.Contains(branch) && depth < 128; depth++)
			{
				matching.Insert(branch);
				branch = m_mParents.Get(branch);
			}
		}
		foreach (int index, SCR_ButtonBaseComponent row : m_aRows)
		{
			ResourceName resource = m_aRowResources[index];
			row.GetRootWidget().SetVisible(IsMatchingBranch(resource, matching) && IsBranchVisible(resource));
		}
		foreach (int index, SCR_ButtonBaseComponent row : m_aVariantRows)
			row.GetRootWidget().SetVisible(!m_bCurrentGear && m_sExpandedPrefab == m_aVariantOwners[index] && m_sSelected == m_aVariantOwners[index] && IsMatchingBranch(m_aVariantOwners[index], matching) && IsBranchVisible(m_aVariantOwners[index]));
		// A numbered variant already represents its prefab in this branch.
		foreach (int index, SCR_ButtonBaseComponent variantRow : m_aVariantRows)
		{
			if (!variantRow.GetRootWidget().IsVisible())
				continue;
			foreach (int rowIndex, ResourceName resource : m_aRowResources)
				if (resource != m_sSelected && SCR_ConfigHelper.GetGUID(resource) == SCR_ConfigHelper.GetGUID(m_aVariantPrefabs[index]) && m_mParents.Get(resource) == m_aVariantOwners[index])
					m_aRows[rowIndex].GetRootWidget().SetVisible(false);
		}
		RefreshRelationship();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSearch(Widget widget)
	{
		ApplyScopeFilter();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMatchingBranch(ResourceName resource, set<ResourceName> matching)
	{
		return matching.Contains(resource);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsExpanded(ResourceName resource)
	{
		if (m_bCurrentGear)
			return false;
		if (resource == m_sExpandedPrefab)
			return true;
		if (m_iVariant >= 0 && m_Variants && SCR_ConfigHelper.GetGUID(resource) == SCR_ConfigHelper.GetGUID(m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab))
			return true;
		ResourceName ancestor = m_mParents.Get(m_sSelected);
		for (int depth; m_aResources.Contains(ancestor) && depth < 128; depth++)
		{
			if (resource == ancestor)
				return true;
			ancestor = m_mParents.Get(ancestor);
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsBranchVisible(ResourceName resource)
	{
		if (!m_Search.GetText().IsEmpty())
			return true;
		ResourceName parent = m_mParents.Get(resource);
		for (int depth; m_aResources.Contains(parent) && depth < 128; depth++)
		{
			// Ancestors outside this folder provide context, not another navigation gate.
			if (MatchesResourceScope(parent) && !GC_LoadoutPaths.Template(parent.GetPath()) && !IsExpanded(parent))
				return false;
			parent = m_mParents.Get(parent);
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CanChangeVariantOwner(ResourceName target)
	{
		if (m_bVariantsDirty && target != m_sVariantOwner)
		{
			m_Status.SetText("#GC-Loadout_PendingVariants");
			return false;
		}
		if (target != m_sVariantOwner)
		{
			m_Variants = null;
			m_sVariantOwner = target;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ReadVariants()
	{
		if (!CanChangeVariantOwner(m_sSelected))
			return false;
		if (!m_Variants)
			m_Variants = GC_LoadoutVariants.Load(m_sSelected);
		return m_Variants != null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnVariantSelect(SCR_ButtonBaseComponent button)
	{
		if (m_DeleteDialog || m_OverwriteDialog)
			return;
		int index = m_aVariantRows.Find(button);
		if (index < 0 || !CanChangeVariantOwner(m_aVariantOwners[index]))
			return;
		// Reopening the selected number enters its saved prefab and its own variants.
		if (m_iVariant == m_aVariantIndices[index] && m_sSelected == m_aVariantOwners[index])
		{
			foreach (ResourceName resource : m_aLibraryResources)
				if (SCR_ConfigHelper.GetGUID(resource) == SCR_ConfigHelper.GetGUID(m_aVariantPrefabs[index]))
				{
					SelectPrefab(resource);
					return;
				}
		}
		m_sSelected = m_aVariantOwners[index];
		if (!ReadVariants())
			return;
		m_iVariant = m_aVariantIndices[index];
		m_bCurrentGear = false;
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVariantSelection()
	{
		m_sExpandedPrefab = m_sSelected;
		ApplyScopeFilter();
		RefreshRelationship();
		RefreshActions();
		RefreshPreview();
		foreach (SCR_ButtonBaseComponent row : m_aRows)
			SetSelection(row, false);
		foreach (int i, SCR_ButtonBaseComponent row : m_aVariantRows)
			SetSelection(row, m_aVariantOwners[i] == m_sVariantOwner && m_aVariantIndices[i] == m_iVariant);
		if (m_bVariantsDirty)
			m_Status.SetText("#GC-Loadout_PendingVariants");
		else
			m_Status.SetText("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AddVariantChoice(ResourceName prefab)
	{
		if (prefab.IsEmpty() || prefab == m_sSelected || m_aVariantChoices.Contains(prefab) || !GC_LoadoutVariants.Load(prefab))
			return;
		m_aVariantChoices.Insert(prefab);
		m_VariantPicker.AddItem(Label(prefab));
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshVariantControls()
	{
		Widget root = GetRootWidget();
		bool variantSelected = m_iVariant >= 0;
		root.FindAnyWidget("PrefabFields").SetVisible(!variantSelected);
		root.FindAnyWidget("PrefabFields").SetEnabled(!m_bVariantsDirty);
		root.FindAnyWidget("VariantFields").SetVisible(variantSelected);
		root.FindAnyWidget("FileActions").SetVisible(!variantSelected);
		root.FindAnyWidget("VariantActions").SetVisible(variantSelected);
		root.FindAnyWidget("VariantSave").SetVisible(m_bVariantsDirty);
		bool writable = GC_LoadoutPrefab.IsWritable(m_sSelected);
		SCR_ButtonTextComponent.GetButtonText("AddVariant", root).SetEnabled(writable);
		SCR_ButtonTextComponent.GetButtonText("DeleteVariant", root).SetEnabled(writable && variantSelected);
		SCR_ButtonTextComponent.GetButtonText("Load", root).SetEnabled(!m_bCurrentGear && !m_bVariantsDirty);
		SCR_ButtonTextComponent.GetButtonText("VariantLoad", root).SetEnabled(!m_bVariantsDirty);
		root.FindAnyWidget("Inherit").SetVisible(!m_bVariantsDirty);
		root.FindAnyWidget("Duplicate").SetVisible(!m_bVariantsDirty);
		root.FindAnyWidget("AddVariant").SetVisible(!m_bVariantsDirty);
		if (!variantSelected)
			return;
		TextWidget.Cast(root.FindAnyWidget("VariantParent")).SetText(WidgetManager.Translate("#GC-Loadout_VariantOf") + " " + Label(m_sSelected));
		m_VariantPicker.ClearAll();
		m_aVariantChoices.Clear();
		m_aVariantChoices.Insert(string.Empty);
		m_VariantPicker.AddItem("#GC-Loadout_Current");
		foreach (ResourceName prefab : m_aResources)
			AddVariantChoice(prefab);
		foreach (SCR_EditableEntityVariant entry : m_Variants.m_aEntries)
			AddVariantChoice(entry.m_sVariantPrefab);
		SCR_EditableEntityVariant selected = m_Variants.m_aEntries[m_iVariant];
		m_VariantPicker.SetCurrentItem(m_aVariantChoices.Find(selected.m_sVariantPrefab), false, false, false);
		m_VariantPicker.SetEnabled(writable);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnVariantPrefabChanged(SCR_ComboBoxComponent combo, int index)
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner) || m_iVariant < 0 || index < 0 || index >= m_aVariantChoices.Count())
			return;
		m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab = m_aVariantChoices[index];
		m_bVariantsDirty = true;
		RefreshList();
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HandleVariantAction(string action)
	{
		if (action == "Save" && (m_iVariant >= 0 || m_bVariantsDirty))
		{
			GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
			if (m_bVariantSaveFailed)
			{
				m_Status.SetText("#GC-Loadout_VariantsFailed");
				return true;
			}
			if (!m_Variants.IsValid(m_sSelected, true))
			{
				m_Status.SetText("#GC-Loadout_InvalidVariants");
				return true;
			}
			foreach (int index, SCR_EditableEntityVariant entry : m_Variants.m_aEntries)
			{
				if (!entry.m_sVariantPrefab.IsEmpty())
					continue;
				ResourceName created;
				if (!GC_LoadoutPrefab.CreateGearVariant(m_Owner, m_sSelected, index + 1, created))
				{
					m_bVariantSaveFailed = true;
					m_Status.SetText(WidgetManager.Translate("#GC-Loadout_VariantsFailed") + " " + created.GetPath());
					return true;
				}
				entry.m_sVariantPrefab = created;
			}
			if (!writer.Open(m_sSelected) || !writer.SaveVariants(m_Owner, m_Variants))
				m_Status.SetText("#GC-Loadout_VariantsFailed");
			else
			{
				m_bVariantsDirty = false;
				RefreshRelationship();
				RefreshList();
				RefreshActions();
				m_Status.SetText("#GC-Loadout_VariantsSaved");
				if (!SaveDependencies(m_sSelected))
					m_Status.SetText("#GC-Loadout_DependenciesSaveFailed");
				RefreshDependencies(true);
			}
			return true;
		}
		if (action == "DeleteVariant")
		{
			if (m_iVariant < 0)
				return true;
			m_DeleteDialog = SCR_ConfigurableDialogUi.CreateFromPreset("{DC391DA320AE5000}Configs/UI/GC_LoadoutDialogs.conf", "deleteVariant");
			if (m_DeleteDialog)
			{
				m_DeleteDialog.GetRootWidget().GetParent().SetZOrder(GetRootWidget().GetZOrder() + 1);
				string title = WidgetManager.Translate("#GC-Loadout_DeleteVariant") + " " + Label(m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab);
				m_DeleteDialog.SetTitle(title + " · " + Label(m_sSelected) + "?");
				m_DeleteDialog.m_OnConfirm.Insert(OnDeleteVariantConfirmed);
				m_DeleteDialog.m_OnCloseStart.Insert(OnDeleteVariantClosed);
			}
			return true;
		}
		if (action == "AddVariant")
		{
			if (!GC_LoadoutPrefab.IsWritable(m_sSelected) || !ReadVariants())
				return true;
			SCR_EditableEntityVariant entry = new SCR_EditableEntityVariant();
			entry.m_bEnabled = true;
			entry.m_iRandomizerWeight = 1;
			m_iVariant = m_Variants.m_aEntries.Insert(entry);
			m_bVariantsDirty = true;
			m_bCurrentGear = false;
			RefreshList();
			RefreshVariantSelection();
			return true;
		}
		if (m_bVariantsDirty && (action == "Load" || action == "Inherit" || action == "Duplicate"))
		{
			m_Status.SetText("#GC-Loadout_PendingVariants");
			return true;
		}
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeleteVariantConfirmed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog != m_DeleteDialog || !GC_PrefabAuthoringSession.IsAvailable(m_Owner) || m_iVariant < 0)
			return;
		m_Variants.m_aEntries.Remove(m_iVariant);
		m_iVariant = Math.Min(m_iVariant, m_Variants.m_aEntries.Count() - 1);
		m_bVariantsDirty = true;
		RefreshList();
		SetFields(m_sSelected);
		RefreshVariantSelection();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDeleteVariantClosed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog == m_DeleteDialog)
		{
			dialog.m_OnConfirm.Remove(OnDeleteVariantConfirmed);
			dialog.m_OnCloseStart.Remove(OnDeleteVariantClosed);
			m_DeleteDialog = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearDeleteDialog()
	{
		SCR_ConfigurableDialogUi dialog = m_DeleteDialog;
		if (!dialog)
			return;
		OnDeleteVariantClosed(dialog);
		dialog.Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetSelection(SCR_ButtonBaseComponent button, bool selected)
	{
		button.SetToggled(selected, false, false);
		Widget edge = button.GetRootWidget().FindAnyWidget("SelectedEdge");
		if (edge)
			edge.SetVisible(selected);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRoleIcon(Widget widget, ResourceName prefab)
	{
		ImageWidget icon = ImageWidget.Cast(widget.FindAnyWidget("RoleIcon"));
		if (!icon)
			return;
		SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(prefab);
		bool visible = info && info.SetIconTo(icon);
		icon.SetVisible(visible);
		Widget label = widget.FindAnyWidget("Text");
		if (label && visible)
			AlignableSlot.SetPadding(label, 64, 4, 16, 4);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFile(string filename)
	{
		ResourceManager manager = Workbench.GetModule(ResourceManager);
		MetaFile meta = manager.GetMetaFile(filename);
		if (!meta)
			return;
		ResourceName resource = meta.GetResourceID();
		if (!resource.IsEmpty() && GC_LoadoutPaths.Allowed(resource.GetPath()) && !GC_LoadoutPaths.Equipment(resource.GetPath()))
		{
			m_aLibraryResources.Insert(resource);
			m_mLibraryByGuid.Set(SCR_ConfigHelper.GetGUID(resource), resource);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelect(SCR_ButtonBaseComponent button)
	{
		int index = m_aRows.Find(button);
		if (index >= 0)
			SelectPrefab(m_aRowResources[index], true);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectPrefab(ResourceName prefab, bool toggle = false)
	{
		if (!CanChangeVariantOwner(prefab))
			return;
		bool collapse = toggle && !m_bCurrentGear && m_iVariant < 0 && m_sSelected == prefab && m_sExpandedPrefab == prefab;
		m_iVariant = -1;
		m_sSelected = prefab;
		m_sExpandedPrefab = prefab;
		if (collapse)
			m_sExpandedPrefab = string.Empty;
		m_bCurrentGear = false;
		SetFields(m_sSelected);
		ApplyScopeFilter();
		m_Status.SetText("");
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		foreach (int i, SCR_ButtonBaseComponent row : m_aRows)
			SetSelection(row, m_aRowResources[i] == prefab);
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
			SetSelection(variantRow, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
		{
			Close();
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshPreview()
	{
		TextWidget message = TextWidget.Cast(GetRootWidget().FindAnyWidget("PreviewStatus"));
		message.SetText("#GC-Loadout_PreviewUnavailable");
		if (!m_PreviewManager || !m_Preview)
			return;

		// The native preview manager owns this entity; never possess or delete it.
		m_PreviewManager.SetPreviewItem(m_Preview, null);
		IEntity character = m_Owner;
		ResourceName previewPrefab = m_sSelected;
		if (m_iVariant >= 0)
			previewPrefab = m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab;
		bool currentPreview = m_bCurrentGear || previewPrefab.IsEmpty();
		if (!currentPreview)
			character = m_PreviewManager.ResolvePreviewEntityForPrefab(previewPrefab);
		if (!ChimeraCharacter.Cast(character))
			return;
		SCR_CharacterInventoryStorageComponent storage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		PreviewRenderAttributes attributes;
		if (storage && storage.GetAttributes())
			attributes = PreviewRenderAttributes.Cast(storage.GetAttributes().FindAttribute(SCR_CharacterInventoryPreviewAttributes));
		if (currentPreview)
			m_PreviewManager.SetPreviewItem(m_Preview, m_Owner, attributes, true);
		else
			m_PreviewManager.SetPreviewItemFromPrefab(m_Preview, previewPrefab, attributes, true);
		message.SetText("");
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshActions()
	{
		Widget root = GetRootWidget();
		SCR_ButtonTextComponent current = SCR_ButtonTextComponent.GetButtonText("Current", root);
		current.SetToggleable(true);
		SetSelection(current, m_bCurrentGear);
		SetRoleIcon(current.GetRootWidget(), GC_PrefabAuthoringSession.GetEditingPrefab());
		SCR_ButtonTextComponent save = SCR_ButtonTextComponent.GetButtonText("Save", root);
		save.SetEnabled(true);
		if (m_bVariantsDirty)
			save.SetText("#GC-Loadout_SaveVariants");
		else
			save.SetText("#GC-Loadout_SaveGear");
		SCR_ButtonTextComponent.GetButtonText("Load", root).SetEnabled(!m_bCurrentGear);
		RefreshVariantControls();
		RefreshDependencies();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshDependencies(bool force = false)
	{
		GetGame().GetCallqueue().Remove(ScanDependencies);
		TextWidget text = TextWidget.Cast(GetRootWidget().FindAnyWidget("Dependencies"));
		ResourceName target = m_sSelected;
		if (m_iVariant >= 0)
			target = m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab;
		if (m_bCurrentGear || target.IsEmpty() || m_bVariantsDirty)
		{
			m_sDependencyTarget = string.Empty;
			m_Dependencies = null;
			if (m_bVariantsDirty)
				text.SetText("#GC-Loadout_DependenciesPending");
			else
				text.SetText("#GC-Loadout_DependenciesUnsaved");
			return;
		}
		bool allVariants = m_iVariant < 0;
		if (!force && m_Dependencies && m_sDependencyTarget == target && m_bDependencyVariants == allVariants)
			return;
		m_sDependencyTarget = target;
		m_bDependencyVariants = allVariants;
		m_Dependencies = null;
		text.SetText("#GC-Loadout_DependenciesScanning");
		GetGame().GetCallqueue().CallLater(ScanDependencies, 1, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScanDependencies()
	{
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner) || m_sDependencyTarget.IsEmpty())
			return;
		m_Dependencies = GC_LoadoutDependencies.Scan(m_sDependencyTarget, m_bDependencyVariants);
		DrawDependencies();
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawDependencies()
	{
		string value;
		if (!m_Dependencies.m_sError.IsEmpty())
			value = WidgetManager.Translate("#GC-Loadout_DependenciesFailed") + "\n";
		else if (m_Dependencies.m_iUnresolved > 0 || m_Dependencies.m_iUnexpanded > 0)
			value = WidgetManager.Translate("#GC-Loadout_DependenciesPartial") + "\n";
		foreach (GC_LoadoutDependencyMod mod : m_Dependencies.m_aMods)
		{
			if (m_Dependencies.IsImplied(mod))
				continue;
			value += mod.m_sTitle;
			value += "\n";
		}
		if (value.IsEmpty())
			value = WidgetManager.Translate("#GC-Loadout_DependenciesNone");
		TextWidget.Cast(GetRootWidget().FindAnyWidget("Dependencies")).SetText(value);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SaveDependencies(ResourceName target)
	{
		GC_LoadoutDependencies report = GC_LoadoutDependencies.Scan(target, true);
		return report.Save();
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName BranchRoot(ResourceName resource)
	{
		for (int depth; depth < 128; depth++)
		{
			ResourceName parent = m_mParents.Get(resource);
			if (!m_aResources.Contains(parent))
				return resource;
			resource = parent;
		}
		return resource;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShowBaseParent(ResourceName resource)
	{
		return !m_bCurrentGear && !m_mParents.Get(resource).IsEmpty() && !m_aResources.Contains(m_mParents.Get(resource)) && (resource == m_sSelected || IsExpanded(resource));
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshRelationship()
	{
		m_sParentPrefab = m_mParents.Get(m_sSelected);
		foreach (int index, SCR_ButtonTextComponent link : m_aParentRows)
		{
			ResourceName resource = m_aRowResources[index];
			bool header = ShowBaseParent(resource) && m_aRows[index].GetRootWidget().IsVisible();
			link.GetRootWidget().SetVisible(header);
			if (header)
			{
				ResourceName parent = m_mParents.Get(resource);
				SCR_UIInfo info = GC_Slot.GetPrefabRoleInfo(parent);
				string title = Label(parent);
				if (info && !info.GetName().IsEmpty())
					title = WidgetManager.Translate(info.GetName());
				link.SetText(title);
				SetRoleIcon(link.GetRootWidget(), parent);
				AlignableSlot.SetPadding(link.GetRootWidget(), 0, 0, 0, 2);
			}
			int depth = m_aRowDepths[index];
			if (ShowBaseParent(BranchRoot(resource)))
				depth++;
			Widget node = m_aRows[index].GetRootWidget();
			AlignableSlot.SetPadding(node, Math.Min(depth, 6) * 28, 0, 0, 2);
			node.FindAnyWidget("TreeBranch").SetVisible(depth > 0);
		}
		foreach (int index, SCR_ButtonBaseComponent variant : m_aVariantRows)
		{
			ResourceName owner = m_aVariantOwners[index];
			int ownerIndex = m_aRowResources.Find(owner);
			int depth = m_aRowDepths[ownerIndex] + 1;
			if (ShowBaseParent(BranchRoot(owner)))
				depth++;
			AlignableSlot.SetPadding(variant.GetRootWidget(), Math.Min(depth, 6) * 28, 0, 0, 2);
			variant.GetRootWidget().FindAnyWidget("TreeBranch").SetVisible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAction(SCR_ButtonBaseComponent button)
	{
		if (m_OverwriteDialog || m_DeleteDialog)
			return;
		string action = button.GetRootWidget().GetName();
		if (action == "VariantLoad")
			action = "Load";
		else if (action == "VariantSave")
			action = "Save";
		if (action == "Close")
		{
			Close();
			return;
		}
		if (!GC_PrefabAuthoringSession.IsAvailable(m_Owner))
			return;
		if (HandleVariantAction(action))
			return;
		if (action == "Current")
		{
			if (!CanChangeVariantOwner(GC_PrefabAuthoringSession.GetEditingPrefab()))
				return;
			m_iVariant = -1;
			m_bCurrentGear = true;
			m_sExpandedPrefab = string.Empty;
			ApplyScopeFilter();
			m_sSelected = GC_PrefabAuthoringSession.GetEditingPrefab();
			m_Name.SetText("");
			SetFields(m_sSelected);
			m_Status.SetText("");
			foreach (SCR_ButtonBaseComponent row : m_aRows)
				SetSelection(row, false);
			RefreshRelationship();
			RefreshPreview();
			RefreshActions();
			return;
		}
		if (action == "Load" && m_bCurrentGear)
			return;
		m_Status.SetColor(Color.White);
		if (action == "Load")
		{
			ResourceName targetPrefab = m_sSelected;
			if (m_iVariant >= 0)
				targetPrefab = m_Variants.m_aEntries[m_iVariant].m_sVariantPrefab;
			if (targetPrefab.IsEmpty() || !GC_PrefabAuthoringSession.LoadPrefab(targetPrefab))
				m_Status.SetText("#GC-Loadout_LoadFailed");
			else
				Close();
			return;
		}
		string unitsRoot;
		if (!Workbench.GetAbsolutePath("$GlobalConflictsUnitsDev:", unitsRoot))
		{
			m_Status.SetText("Load GC Units Dev to save loadouts.");
			return;
		}
		string path = GC_LoadoutPrefab.Destination(m_Faction.GetText(), m_Group.GetText(), m_Name.GetText());
		if (path.IsEmpty())
		{
			m_Status.SetText("#GC-Loadout_InvalidName");
			return;
		}
		GC_LoadoutMetadata metadata = GetMetadata();
		if (!metadata || !metadata.IsValid())
		{
			m_Status.SetText("#GC-Loadout_InvalidMetadata");
			return;
		}
		ResourceName target;
		bool created;
		if (FileIO.FileExists(path))
		{
			if (action != "Save")
			{
				m_Status.SetText("#GC-Loadout_NameExists");
				return;
			}
			ResourceManager manager = Workbench.GetModule(ResourceManager);
			MetaFile meta = manager.GetMetaFile(path);
			if (meta)
				target = meta.GetResourceID();
			if (target.IsEmpty())
			{
				m_Status.SetText("#GC-Loadout_CreateFailed");
				return;
			}
			m_sOverwriteTarget = target;
			m_OverwriteDialog = SCR_ConfigurableDialogUi.CreateFromPreset("{DC391DA320AE5000}Configs/UI/GC_LoadoutDialogs.conf", "overwrite");
			if (m_OverwriteDialog)
			{
				// Native configurable dialogs place their layout inside a proxy menu.
				m_OverwriteDialog.GetRootWidget().GetParent().SetZOrder(GetRootWidget().GetZOrder() + 1);
				m_OverwriteDialog.SetTitle(WidgetManager.Translate("#GC-Loadout_Overwrite") + " " + Label(target) + "?");
				m_OverwriteDialog.m_OnConfirm.Insert(OnOverwrite);
				m_OverwriteDialog.m_OnCloseStart.Insert(OnOverwriteClosed);
			}
			return;
		}
		else
		{
			ResourceName parent = m_sSelected;
			target = GC_LoadoutPrefab.Create(parent, path, action == "Duplicate");
			created = !target.IsEmpty();
		}
		FinishSave(target, created, m_bCurrentGear || action == "Save");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		if (m_Faction && m_Faction.IsOpened())
		{
			m_Faction.CloseList();
			return;
		}
		if (m_Group && m_Group.IsOpened())
		{
			m_Group.CloseList();
			return;
		}
		if (m_VariantPicker && m_VariantPicker.IsOpened())
		{
			m_VariantPicker.CloseList();
			return;
		}
		if (m_Icon && m_Icon.IsExpanded())
		{
			m_Icon.CloseList();
			return;
		}
		if (!SCR_ConfigurableDialogUi.GetCurrentDialog())
			Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOverwrite(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog != m_OverwriteDialog || !GC_PrefabAuthoringSession.IsAvailable(m_Owner))
			return;
		ResourceName target = m_sOverwriteTarget;
		ClearOverwrite();
		FinishSave(target, false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOverwriteClosed(SCR_ConfigurableDialogUi dialog)
	{
		if (dialog == m_OverwriteDialog)
			ClearOverwrite();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearOverwrite()
	{
		if (m_OverwriteDialog)
		{
			m_OverwriteDialog.m_OnConfirm.Remove(OnOverwrite);
			m_OverwriteDialog.m_OnCloseStart.Remove(OnOverwriteClosed);
		}
		m_OverwriteDialog = null;
		m_sOverwriteTarget = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void FinishSave(ResourceName target, bool created, bool writeGear)
	{
		if (target.IsEmpty())
		{
			m_Status.SetText("#GC-Loadout_CreateFailed");
			return;
		}
		GC_LoadoutPrefab writer = new GC_LoadoutPrefab();
		GC_LoadoutMetadata metadata = GetMetadata();
		if (!metadata || !writer.Open(target) || !writer.Save(m_Owner, metadata, writeGear))
		{
			if (created)
				GC_PrefabAuthoringSession.SetEditingPrefab(target);
			m_sSelected = target;
			RefreshList();
			RefreshRelationship();
			m_Status.SetText("Save incomplete. Your current gear is retained; retry Save. " + writer.GetError());
			return;
		}
		// The current character already contains the gear just written. Saving
		// must not respawn it or restart the local Workbench session.
		bool requirementsSaved = SaveDependencies(target);
		if (!writeGear && !GC_PrefabAuthoringSession.LoadPrefab(target))
		{
			m_sSelected = target;
			RefreshList();
			SetFields(target);
			RefreshRelationship();
			RefreshPreview();
			RefreshDependencies(true);
			m_Status.SetText("#GC-Loadout_SavedLoadFailed");
			return;
		}
		GC_PrefabAuthoringSession.SetEditingPrefab(target);
		m_Owner = GetGame().GetPlayerController().GetControlledEntity();
		m_sSelected = target;
		m_sExpandedPrefab = target;
		m_bCurrentGear = false;
		m_iVariant = -1;
		m_Variants = null;
		m_sVariantOwner = string.Empty;
		RefreshList();
		SetFields(target);
		RefreshRelationship();
		RefreshPreview();
		RefreshActions();
		RefreshDependencies(true);
		if (requirementsSaved)
			m_Status.SetText("#GC-Loadout_LoadoutSaved");
		else
			m_Status.SetText("#GC-Loadout_DependenciesSaveFailed");
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_SearchEvents)
			m_SearchEvents.GetOnChange().Remove(OnSearch);
		GetGame().GetCallqueue().Remove(ScanDependencies);
		GetGame().GetCallqueue().Remove(ApplyFactionFilter);
		if (m_Faction)
		{
			m_Faction.m_OnFilterChanged.Remove(OnFactionFilter);
			m_Faction.CloseList();
		}
		if (m_Group)
		{
			m_Group.m_OnFilterChanged.Remove(ApplyScopeFilter);
			m_Group.CloseList();
		}
		ClearDeleteDialog();
		if (m_VariantPicker)
		{
			m_VariantPicker.m_OnChanged.Remove(OnVariantPrefabChanged);
			m_VariantPicker.CloseList();
		}
		SCR_ConfigurableDialogUi dialog = m_OverwriteDialog;
		ClearOverwrite();
		if (dialog)
			dialog.Close();
		if (m_PreviewManager && m_Preview)
			m_PreviewManager.SetPreviewItem(m_Preview, null);
		m_Preview = null;
		m_PreviewManager = null;
		if (m_Icon)
		{
			m_Icon.m_OnChanged.Remove(OnIconChanged);
			m_Icon.CloseList();
		}
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
		foreach (SCR_ButtonBaseComponent button : m_aButtons)
			button.m_OnClicked.Remove(OnAction);
		foreach (SCR_ButtonBaseComponent row : m_aRows)
			row.m_OnClicked.Remove(OnSelect);
		m_aButtons.Clear();
		m_aRows.Clear();
		foreach (SCR_ButtonBaseComponent variantRow : m_aVariantRows)
			variantRow.m_OnClicked.Remove(OnVariantSelect);
		m_aVariantRows.Clear();
		super.OnMenuClose();
	}
#else
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Close();
	}
#endif
}

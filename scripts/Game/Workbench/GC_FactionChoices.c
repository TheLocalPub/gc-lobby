#ifdef WORKBENCH
//! Reads factions used by placed groups/entities, without changing affiliations.
class GC_FactionChoices
{
	//------------------------------------------------------------------------------------------------
	array<string> GetChoices(WorldEditorAPI api)
	{
		array<string> values = {};
		if (!api || !api.GetWorld())
			return values;

		int mission = api.GetCurrentSubScene();
		if (mission < 0 || mission >= api.GetNumSubScenes())
			return values;

		set<IEntitySource> visited = new set<IEntitySource>();
		for (int index = 0, count = api.GetEntityCount(mission); index < count; index++)
			CollectUnits(api, api.GetEntity(mission, index), mission, visited, values);

		return values;
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectUnits(WorldEditorAPI api, IEntitySource source, int mission, set<IEntitySource> visited, array<string> values)
	{
		if (!source || source.GetSubScene() != mission || visited.Contains(source))
			return;
		visited.Insert(source);

		IEntity entity = api.SourceToEntity(source);
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (group)
		{
			// This accessor returns the authored m_faction even for unspawned groups.
			string groupKey = group.GetFactionName();
			if (groupKey.IsEmpty() && group.GetFaction())
				groupKey = group.GetFaction().GetFactionKey();
			AddKey(values, groupKey);
		}
		else if (ChimeraCharacter.Cast(entity) || Vehicle.Cast(entity))
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (affiliation)
			{
				string unitKey = affiliation.GetAffiliatedFactionKey();
				if (unitKey.IsEmpty())
					unitKey = affiliation.GetDefaultFactionKey();
				AddKey(values, unitKey);
			}
		}

		for (int child = 0, count = source.GetNumChildren(); child < count; child++)
			CollectUnits(api, source.GetChild(child), mission, visited, values);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddKey(array<string> values, string key)
	{
		// Many entities sharing one key is normal, not a duplicate definition.
		if (!key.IsEmpty() && !values.Contains(key))
			values.Insert(key);
	}
}
#endif

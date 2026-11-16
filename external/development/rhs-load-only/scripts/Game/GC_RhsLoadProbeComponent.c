class GC_RhsLoadProbeComponentClass : ScriptComponentClass
{
}

//! Test-only observer, enabled explicitly with -gcRhsProbe <run-id> in MCP launches.
class GC_RhsLoadProbeComponent : ScriptComponent
{
	protected string m_sRun;
	protected int m_iSamples;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode() || !System.GetCLIParam("gcRhsProbe", m_sRun))
			return;
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		foreach (string addon : addons)
			Print(string.Format("GC_RHS_TEST run=%1 addon=%2 id=%3", m_sRun, addon, GameProject.GetAddonID(addon)));
		Resource rifleResource = Resource.Load("{5961913EE3FF9467}Prefabs/Weapons/Rifles/M4A1/Rifle_M4A1_BLOCK_0.et");
		bool resourceValid = rifleResource && rifleResource.IsValid();
		Print(string.Format("GC_RHS_TEST run=%1 server=%2 rhsResource=%3", m_sRun, Replication.IsServer(), resourceValid));
		array<ResourceName> attributeResources = {"{7E161126A01D1002}Prefabs/AttributeCompass.et", "{7E161126A01D1003}Prefabs/AttributeBandage.et", "{7E161126A01D1004}Prefabs/AttributeHead.et"};
		foreach (ResourceName attributeResource : attributeResources)
		{
			Resource loaded = Resource.Load(attributeResource);
			bool valid = loaded && loaded.IsValid();
			Print(string.Format("GC_ATTRIBUTE_TEST run=%1 server=%2 resourceValid=%3 resource=%4", m_sRun, Replication.IsServer(), valid, attributeResource));
		}
		GetGame().GetCallqueue().CallLater(Check, 5000, true);
	}

	//------------------------------------------------------------------------------------------------
	void Check()
	{
		m_iSamples++;
		IEntity rifle = GetGame().GetWorld().FindEntityByName("GC_RhsPlacedRifle");
		ResourceName prefab;
		if (rifle && rifle.GetPrefabData())
			prefab = rifle.GetPrefabData().GetPrefabName();
		bool present = prefab == "{5961913EE3FF9467}Prefabs/Weapons/Rifles/M4A1/Rifle_M4A1_BLOCK_0.et";
		int players = GetGame().GetPlayerManager().GetPlayerCount();
		Print(string.Format("GC_RHS_TEST run=%1 server=%2 sample=%3 rhsEntity=%4 players=%5 prefab=%6", m_sRun, Replication.IsServer(), m_iSamples, present, players, prefab));
		if (m_iSamples == 2)
			CheckAttributes();
		if (m_iSamples >= 36)
			GetGame().GetCallqueue().Remove(Check);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAttributes()
	{
		IEntity character = GetGame().GetWorld().FindEntityByName("GC_AttributeCharacter");
		if (!character)
		{
			Print(string.Format("GC_ATTRIBUTE_TEST run=%1 server=%2 character=0", m_sRun, Replication.IsServer()));
			return;
		}
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		array<IEntity> items = {};
		if (inventory)
			inventory.GetItems(items);
		foreach (IEntity item : items)
			Print(string.Format("GC_ATTRIBUTE_TEST run=%1 server=%2 inventory=%3", m_sRun, Replication.IsServer(), SCR_ResourceNameUtils.GetPrefabName(item)));
		CharacterIdentityComponent identity = CharacterIdentityComponent.Cast(character.FindComponent(CharacterIdentityComponent));
		if (identity && identity.GetIdentity() && identity.GetIdentity().GetVisualIdentity())
			Print(string.Format("GC_ATTRIBUTE_TEST run=%1 server=%2 head=%3", m_sRun, Replication.IsServer(), identity.GetIdentity().GetVisualIdentity().GetHead()));
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Check);
	}
}

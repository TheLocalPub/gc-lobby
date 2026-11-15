//! Shared fixture also runs with GC Lobby absent.
modded class SCR_BaseGameMode
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!SCR_Global.IsEditMode())
		{
			if (GetName().IsEmpty())
				SetName("GC_RetentionTestMode");
			Print("GC | Retention fixture mode=" + GetName());
		}
	}
}

modded class SCR_GameModeHealthSettings
{
	protected bool GC_TestGetRetention()
	{
		return false;
	}

	protected void GC_TestSetRetention(bool enabled)
	{
	}

	protected ChimeraCharacter m_GC_RetentionBody;
	protected IEntity m_GC_RetentionWeapon;
	protected IEntity m_GC_RetentionOriginalBody;
	protected bool m_bGC_RetentionOriginal;

	string GC_TestWeaponRetention(int stage, bool enabled)
	{
		if (!Replication.IsServer())
			return "BLOCKED: local authority required";

		if (stage == 0)
		{
			if (m_GC_RetentionBody)
				return "BLOCKED: clean up previous fixture first";
			m_bGC_RetentionOriginal = GC_TestGetRetention();
			GC_TestSetRetention(enabled);
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = "1195 39 2715";
			params.Transform[3][1] = GetGame().GetWorld().GetSurfaceY(params.Transform[3][0], params.Transform[3][2]);
			Resource resource = Resource.Load("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et");
			m_GC_RetentionBody = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
			if (!m_GC_RetentionBody)
				return "FAIL: fixture spawn";
			AIControlComponent ai = AIControlComponent.Cast(m_GC_RetentionBody.FindComponent(AIControlComponent));
			if (ai)
				ai.DeactivateAI();
			m_GC_RetentionOriginalBody = GetGame().GetPlayerController().GetControlledEntity();
			GetGame().GetPlayerController().SetControlledEntity(m_GC_RetentionBody);
			return "spawned; run stage 1 after initialization";
		}

		if (stage == 4)
		{
			GC_TestSetRetention(m_bGC_RetentionOriginal);
			GetGame().GetPlayerController().SetControlledEntity(m_GC_RetentionOriginalBody);
			if (m_GC_RetentionBody)
				SCR_EntityHelper.DeleteEntityAndChildren(m_GC_RetentionBody);
			if (m_GC_RetentionWeapon)
				SCR_EntityHelper.DeleteEntityAndChildren(m_GC_RetentionWeapon);
			m_GC_RetentionBody = null;
			m_GC_RetentionWeapon = null;
			m_GC_RetentionOriginalBody = null;
			return "fixture removed; setting restored";
		}

		if (!m_GC_RetentionBody)
			return "BLOCKED: spawn first";
		CharacterControllerComponent controller = m_GC_RetentionBody.GetCharacterController();
		BaseWeaponManagerComponent weapons = controller.GetWeaponManagerComponent();
		if (stage == 1)
		{
			array<IEntity> items = {};
			weapons.GetWeaponsList(items);
			if (items.IsEmpty())
				return "FAIL: fixture has no weapon";
			foreach (IEntity item : items)
			{
				BaseWeaponComponent weapon = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
				if (weapon && weapon.GetWeaponType() == EWeaponType.WT_RIFLE)
					m_GC_RetentionWeapon = item;
			}
			if (!m_GC_RetentionWeapon)
				return "FAIL: fixture has no rifle";
			bool requested = controller.TryEquipRightHandItem(m_GC_RetentionWeapon, EEquipItemType.EEquipTypeWeapon, true);
			return string.Format("equip requested=%1 dead=%2 position=%3 controlled=%4 rifle=%5", requested, controller.IsDead(), m_GC_RetentionBody.GetOrigin(), GetGame().GetPlayerController().GetControlledEntity() == m_GC_RetentionBody, m_GC_RetentionWeapon.GetPrefabData().GetPrefabName());
		}
		if (stage == 2)
		{
			if (!weapons.GetCurrentWeapon() || weapons.GetCurrentWeapon().GetOwner() != m_GC_RetentionWeapon)
				return "BLOCKED: fixture weapon must be in hand";
			SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(m_GC_RetentionBody.GetDamageManager());
			damage.ForceUnconsciousness();
			return "native unconsciousness requested; run stage 3 after simulation";
		}
		if (stage == 3)
		{
			bool retained = m_GC_RetentionWeapon && m_GC_RetentionWeapon.GetRootParent() == m_GC_RetentionBody;
			AIControlComponent ai = AIControlComponent.Cast(m_GC_RetentionBody.FindComponent(AIControlComponent));
			return string.Format("enabled=%1 unconscious=%2 retained=%3 currentWeapon=%4 activeAI=%5", GC_TestGetRetention(), controller.IsUnconscious(), retained, weapons.GetCurrentWeapon() != null, ai && ai.IsAIActivated());
		}
		return "unknown stage";
	}
}

//! The native consciousness callback is the same synchronous stowing point used
//! by vanilla AI. Life-state notifications offer no weapon-drop veto, and an
//! opt-in character subclass would miss mission-selected third-party characters.
//! No state or subscriptions survive a mode change; inherited handling always runs.
modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnConsciousnessChanged(bool conscious)
	{
		super.OnConsciousnessChanged(conscious);

		GC_LobbyGameMode gameMode = GC_LobbyGameMode.GetGameMode();
		if (!gameMode || !gameMode.IsKeepWeaponWhenUnconsciousEnabled() || conscious || GetLifeState() != ECharacterLifeState.INCAPACITATED)
			return;

		// Vanilla already handles active AI. Extend its policy to non-AI characters.
		AIControlComponent aiControl = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		if (aiControl && aiControl.IsAIActivated())
			return;

		BaseWeaponManagerComponent weaponManager = GetWeaponManagerComponent();
		if (!weaponManager || !weaponManager.GetCurrentWeapon())
			return;

		IEntity weapon = weaponManager.GetCurrentWeapon().GetOwner();
		if (!weapon || !GetAnimationComponent())
			return;

		SCR_CharacterCommandHandlerComponent handler = SCR_CharacterCommandHandlerComponent.Cast(GetAnimationComponent().GetCommandHandler());
		if (weapon.FindComponent(GrenadeMoveComponent))
		{
			BaseTriggerComponent trigger = BaseTriggerComponent.Cast(weapon.FindComponent(BaseTriggerComponent));
			if ((trigger && trigger.WasTriggered()) || (handler && handler.IsThrowingAction()))
			{
				if (handler)
					handler.DropLiveGrenadeFromHand(false);
				return;
			}
		}

		TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
	}
}

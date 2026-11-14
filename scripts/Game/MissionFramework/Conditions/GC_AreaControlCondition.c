[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Control Condition")]
class GC_AreaControlCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.Auto, "Area to capture. Lists Radius Areas and all closed polygons, including Setup Zones and AO Limits. Unnamed areas are included; selecting one assigns its reference name. Refreshes when opened.")]
	GC_AreaName m_sArea;

	[Attribute("", UIWidgets.Auto, "The faction this condition checks, for example US. It captures when it has more living players inside than all other counted factions combined. 11 versus 10 captures; 10 versus 10 pauses. This condition passes once this faction has captured the area.")]
	FactionKey m_sFaction;

	[Attribute("30", UIWidgets.EditBox, "Seconds needed to capture or lose control. Starts neutral. A tie, including an empty area, freezes progress and keeps the current result. An opposing advantage reverses unfinished progress. Zero means instant capture.", "0 86400")]
	float m_fCaptureSeconds;

	[Attribute("0", UIWidgets.ComboBox, "Who counts inside the area. Dead characters, spectators and empty playable slots do not count.", enums: {ParamEnum("Players", "0"), ParamEnum("AI", "1"), ParamEnum("Players and AI", "2")}, category: "Advanced")]
	int m_iWho;

	[Attribute("", UIWidgets.Auto, "Optional factions that cannot capture or contest this area, for example civilians. All other factions compete with the selected faction.", category: "Advanced")]
	ref array<FactionKey> m_aIgnoredFactions;

	[Attribute("", UIWidgets.Object, "Optional capture and loss messages. Leave empty for no messages.", category: "Advanced")]
	ref GC_CaptureMessages m_Messages;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (m_sFaction.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 10;
		check.m_sAreaName = m_sArea;
		array<string> factions = {m_sFaction};
		if (!GC_AreaPopulation.BuildSelection(check, m_iWho, factions))
			return null;
		check.m_bLocalFactionShare = true;
		check.m_bAreaMajority = true;
		check.m_fAreaProportion = 0.5;
		check.m_aIgnoredFactions = GC_FactionKeys.ToStrings(m_aIgnoredFactions);
		check.m_Capture = new GC_AreaCaptureOptions();
		check.m_Capture.m_fSeconds = m_fCaptureSeconds;
		check.m_Capture.m_iCheckInterval = 1;
		check.m_Capture.m_bSkipDiscovery = true;
		if (m_Messages)
			check.m_Capture.m_Messages = m_Messages.Copy();
		if (!check.IsValid(depth))
			return null;
		return check;
	}
}

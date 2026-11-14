[BaseContainerProps(), BaseContainerCustomStringTitleField("Area Condition")]
class GC_AreaCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.Object, "Choose Named Entities for any specific vehicles, props, cargo or other world entities, or Players / AI for living characters with optional faction filters. Capture timing works with either selection. These are target settings, not nested conditions.")]
	ref GC_AreaTargetsBase m_Targets;

	[Attribute("", UIWidgets.Auto, "Choose an area from the world: Radius Area or any closed polygon, including Setup Zones and AO Limits. Unnamed areas are listed too; selecting one assigns its reference name. The list refreshes when opened. Radius uses 3D distance; polygons ignore height. Configure the boundary on the area.")]
	GC_AreaName m_sArea;

	[Attribute("0", UIWidgets.CheckBox, "Checked: count targets outside the area. Unchecked: count targets inside. Requirement controls how many are needed. Checked once per second; brief crossings can be missed. Missing targets or invalid areas never pass.")]
	bool m_bOutside;

	[Attribute("", UIWidgets.Object, "How many selected targets must be inside (or outside)? Leave empty for at least one. Count checks a number; Proportion checks a fraction, such as 0.9 for 90%. For teams contesting a capture point, use Area Control Condition instead: it needs no Requirement settings.")]
	ref GC_LivingRequirementBase m_Requirement;

	[Attribute("", UIWidgets.Hidden, "Legacy Count. Used only when Requirement is unset, preserving older saved area checks.")]
	ref GC_AreaCount m_Count;

	[Attribute("", UIWidgets.Object, "Optional capture timing and status messages for any selected target type, including players, AI, vehicles and objects. Leave unset for the existing immediate area check. Available during Match only.")]
	ref GC_AreaCaptureOptions m_Capture;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		GC_RuleCondition condition = new GC_RuleCondition();
		condition.m_iCheck = 10;
		condition.m_sAreaName = m_sArea;
		condition.m_bAreaOutside = m_bOutside;
		if (!m_Targets || !m_Targets.BuildTargets(condition))
			return null;
		GC_LivingRequirementBase requirement = m_Requirement;
		if (!requirement)
			requirement = m_Count;
		if (requirement && !requirement.BuildRequirement(condition))
			return null;
		if (m_Capture)
			condition.m_Capture = m_Capture.Copy();
		if (!condition.IsValid(depth))
			return null;
		return condition;
	}
}

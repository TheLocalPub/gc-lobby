#ifdef WORKBENCH
class GC_AreaChoices : GC_EntityReferenceChoices
{
	//------------------------------------------------------------------------------------------------
	override protected bool AcceptEntity(IEntity entity)
	{
		return GC_MissionAreaGeometry.SupportsEntity(entity);
	}
}
#endif

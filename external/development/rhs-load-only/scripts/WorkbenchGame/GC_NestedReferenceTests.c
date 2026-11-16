#ifdef WORKBENCH
[BaseContainerProps()]
class GC_NestedReferenceFixture
{
	[Attribute()]
	ResourceName m_sNext;
	[Attribute()]
	ResourceName m_sMissing;
	[Attribute()]
	ResourceName m_GC_GeneratedMetadata;
}

[WorkbenchPluginAttribute(name: "Test nested references", wbModules: {"WorldEditor"}, category: "GC Tests")]
class GC_NestedReferenceTests : WorkbenchPlugin
{
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		GC_MissionResourceReferences graph = new GC_MissionResourceReferences(System.GetTickCount());
		ResourceName root = "{7E161126A01D0031}Configs/ReferenceTests/CycleA.conf";
		graph.AddResource(root, "test");
		graph.AddResource(root, "duplicate");
		bool expanded = graph.Expand();
		int resolved, unresolved;
		foreach (GC_MissionMetadataResource row : graph.GetReferences())
		{
			if (row.m_sStatus == "container")
				resolved++;
			if (row.m_sStatus == "unresolved" && row.m_sResource.EndsWith("Missing.conf"))
				unresolved++;
		}
		bool cycle = expanded && graph.GetReferences().Count() == 3 && resolved == 2 && unresolved == 1;
		GC_MissionResourceReferences expired = new GC_MissionResourceReferences(System.GetTickCount() - 30001);
		expired.AddResource(root, "expired");
		bool limit = !expired.Expand() && !expired.GetError().IsEmpty() && expired.GetReferences().IsEmpty();
		Print(string.Format("GC_NESTED_TEST cycles_dedup_missing_exclusion=%1 timeout_rejected=%2", cycle, limit));
		Resource saved = BaseContainerTools.LoadContainer("{7E161126A01D0003}Configs/Missions/RhsLoadOnly.conf");
		BaseContainer metadata;
		if (saved && saved.IsValid())
			metadata = saved.GetResource().ToBaseContainer().GetObject("m_GC_GeneratedMetadata");
		int schema, targets;
		if (metadata)
		{
			metadata.Get("m_iSchemaVersion", schema);
			BaseContainerList references = metadata.GetObjectArray("m_aResourceReferences");
			for (int i; references && i < references.Count(); i++)
			{
				ResourceName name;
				references.Get(i).Get("m_sResource", name);
				if (name.EndsWith("AttributeCompass.et") || name.EndsWith("AttributeBandage.et") || name.EndsWith("AttributeHead.et") || name.EndsWith("AttributeInstanceCompass.et"))
					targets++;
			}
		}
		Print(string.Format("GC_NESTED_TEST native_header_reload=%1 schema=%2 targets=%3", schema == 2 && targets == 4, schema, targets));
	}
}
#endif

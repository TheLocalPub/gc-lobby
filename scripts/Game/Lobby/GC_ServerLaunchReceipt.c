//! Dedicated launcher acknowledgement. Only the active GC game mode invokes it.
//! A fresh per-launch profile and nonce prevent acceptance of an older receipt.
class GC_ServerLaunchReceipt
{
	//------------------------------------------------------------------------------------------------
	static void Write()
	{
		string launchId;
		if (!System.GetCLIParam("gcLaunchId", launchId) || launchId.IsEmpty() || launchId.Length() > 64)
			return;
		MissionHeader header = GetGame().GetMissionHeader();
		if (!header)
		{
			Print("GC | Launch | mission header unavailable", LogLevel.ERROR);
			return;
		}

		array<string> addonGuids = {};
		GameProject.GetLoadedAddons(addonGuids);

		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("version", 1);
		context.WriteValue("launchId", launchId);
		context.WriteValue("scenario", header.GetHeaderResourceName());
		context.WriteValue("world", header.GetWorldResourceName());
		context.WriteValue("worldFile", GetGame().GetWorldFile());
		context.WriteValue("headerWorldFile", header.GetWorldPath());
		context.WriteValue("addonGuids", addonGuids);
		FileHandle file = FileIO.OpenFile("$profile:GCLaunchReceipt.json", FileMode.WRITE);
		if (!file)
		{
			Print("GC | Launch | receipt file could not be opened", LogLevel.ERROR);
			return;
		}
		file.Write(context.SaveToString());
		file.Close();
		PrintFormat("GC | Launch | acknowledged id=%1 scenario=%2 addons=%3", launchId, header.GetHeaderResourceName(), addonGuids.Count());
	}
}

/*!
	This is meant to be executed on the client
	Where the server sends the current lootmap settings
*/
class TW_LootSettingsProxyInterface : TW_SettingsInterface<LootManagerSettings>
{
	private ref LootManagerSettings _lootSettings;
	private SCR_FactionManager _factionManager;
	
	private SCR_FactionManager GetFactionManager()
	{
		if(_factionManager)
			return _factionManager;
		
		_factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		return _factionManager;
	}
	
	override void Initialize(LootManagerSettings settings)
	{
		if(!Replication.IsClient())
		{
			PrintFormat("TrainWreck: TW_LootSettingsProxyInterface must be executed on client, not server", LogLevel.ERROR);
			return;
		}
		
		if(settings)
			_lootSettings = settings;
		else
			PrintFormat("TrainWreck TW_LootSettingsProxyInterface: LootManagerSettings was null", LogLevel.ERROR);
	
		isInitialized = true;
	}
	
	LootManagerSettings GetLootSettings() { return _lootSettings; }
	
	override void SaveSettings()
	{
		if(!Replication.IsClient())
		{
			PrintFormat("TrainWreck: TW_LootSettingsProxyInterface: SaveSettings must be invoked by client", LogLevel.ERROR);
			return;
		}
		
		if(!GetLootSettings())
		{
			PrintFormat("TrainWreck: Unable to save $profile:lootmap.json - settings are null", LogLevel.ERROR);
			return;
		}
		
		if(!GetLootSettings().LootTable)
			GetLootSettings().LootTable = new map<string, ref array<ref TW_LootConfigItem>>();
	
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		player.UpdateLootSettings(GetLootSettings());
	}
}

typedef TW_SettingsManager<ref TW_LootSettingsProxyInterface<LootManagerSettings>> LootSettingsManager;
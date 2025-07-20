class LootRespawnSettings
{
	int RespawnLootRadius;
	bool IsLootRespawnable;
	int RespawnLootTimerInSeconds;
	int RespawnAfterLastInteractionInMinutes;
	int NumberOfItemsToSpawnPerContainer;
	float UnlootedTimeRatio;
	float SearchedTimeRatio;
	
	int GridSize;
	int DeadZoneRadius;
	
	void LootRespawnSettings()
	{
		RespawnLootRadius = 5;
		IsLootRespawnable = true;
		RespawnLootTimerInSeconds = 10;
		RespawnAfterLastInteractionInMinutes = 60;
		NumberOfItemsToSpawnPerContainer = 4;
		UnlootedTimeRatio = 0.5;
		SearchedTimeRatio = 0.0;
		GridSize = 100;
		DeadZoneRadius = 2;
	}
};

class LootManagerSettings
{
	bool ShouldSpawnMagazine;
	bool IsLootEnabled;
	bool ShowDebug;
	
	ref LootRespawnSettings RespawnSettings;
	ref PercentageFieldSetting AmmoPercentageSetting;
	ref ScavLootSettings ScavSettings;
	
	ref map<string, ref array<ref TW_LootConfigItem>> LootTable;
	
	void LootManagerSettings()
	{
		ShouldSpawnMagazine = true;
		IsLootEnabled = true;
		RespawnSettings = new LootRespawnSettings();
		ScavSettings = new ScavLootSettings();
		AmmoPercentageSetting = new PercentageFieldSetting();
		
		AmmoPercentageSetting.Min = 80;
		AmmoPercentageSetting.Max = 100;
	}
	
	[NonSerialized()]
	const string FILENAME = "$profile:lootmap.json";
	
	static bool SaveToFile(LootManagerSettings settings)
	{
		if(!settings) return false;
		
		if(!settings.LootTable)
			settings.LootTable = new map<string, ref array<ref TW_LootConfigItem>>();
		
		return TW_Util.SaveJsonFile(FILENAME, settings, true);
	}
	
	static LootManagerSettings LoadFromFile(string json=string.Empty)
	{
		SCR_JsonLoadContext context;
		
		if(json != string.Empty)
		{
			context = new SCR_JsonLoadContext();
			context.ImportFromString(json);
		}
		else 
			context = TW_Util.LoadJsonFile(FILENAME, true);
		
		if(!context)
		{
			PrintFormat("TrainWreck: LootManagerSettings -> Failed to load settings from: %1", FILENAME, LogLevel.ERROR);
			ref LootManagerSettings settings = GetDefault();
			LootManagerSettings.SaveToFile(settings);
			return settings;
		}
		
		PrintFormat("TrainWreck: LootManagerSettings -> Successfully loaded file %1", FILENAME);
		
		ref LootManagerSettings settings = new LootManagerSettings();
		
		if(!context.ReadValue("", settings))
		{
			PrintFormat("TrainWreck: LootManagerSettings -> Loading settings failed", LogLevel.ERROR);
			settings = GetDefault();
			LootManagerSettings.SaveToFile(settings);
			return settings;	
		}
		
		return settings;
	}
	
	static LootManagerSettings GetDefault()
	{
		ref LootManagerSettings settings = new LootManagerSettings();
		return settings;
	}
};
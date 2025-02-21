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
};
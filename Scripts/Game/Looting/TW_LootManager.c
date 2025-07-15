enum TW_ResourceNameType
{
	ResourceName,
	DisplayName
};

class TW_LootSettingsInterface : TW_SettingsInterface<LootManagerSettings>
{
	
}

typedef TW_SettingsManager<ref TW_LootSettingsInterface<LootManagerSettings>> LootSettingsManager;


sealed class TW_LootManager 
{
	private static TW_LootManager s_Instance;
	static TW_LootManager GetInstance() { return s_Instance; }
	
	void TW_LootManager()
	{
		if(s_Instance)
		{
			PrintFormat("TrainWreck: TW_LootManager -> Instance of Loot Manager already exists", LogLevel.ERROR);
			return;
		}
		
		s_Instance = this;
	}
	
	// Provide the ability to grab 
	private static ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> s_LootTable = new map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();
	
	// This should contain the resource names of all items that are valid for saving/loading 
	private static ref set<string> s_GlobalItems = new set<string>();
	private static ref TW_GridCoordArrayManager<TW_LootableInventoryComponent> s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(100);
	private static ref array<SCR_EArsenalItemType> s_ArsenalItemTypes = {};
	static TW_GridCoordArrayManager<TW_LootableInventoryComponent> GetContainerGrid() { return s_GlobalContainerGrid; }
	
	private static bool HasLoaded = false;
	static const string LootFileName = "$profile:lootmap.json";	
	
	private ref LootManagerSettings m_Settings;
	private SCR_BaseGameMode m_GameMode;
		
	LootManagerSettings GetLootSettings() { return m_Settings; }
	bool ShouldSpawnMagazine() { return m_Settings.ShouldSpawnMagazine; }
	float GetUnlootedTimeRatio() 
	{ 
		if(!m_Settings || !m_Settings.RespawnSettings)
		{
			PrintFormat("TrainWreck: RespawnSettings not set", LogLevel.WARNING);
			return 0.5;
		}
		
		return m_Settings.RespawnSettings.UnlootedTimeRatio; 
	}
	float GetSearchedTimeRatio() 
	{ 
		if(!m_Settings || !m_Settings.RespawnSettings)
		{
			PrintFormat("TrainWreck: RespawnSettings not set", LogLevel.WARNING);
			return 0.5;
		}
		
		return m_Settings.RespawnSettings.SearchedTimeRatio; 
	}
	
	ScavLootSettings GetScavSettings() { return m_Settings.ScavSettings; }
	
	bool IsDebug()
	{
		if(!m_Settings)
			return false;
		
		return m_Settings.ShowDebug;
	}
	
	float GetRandomAmmoPercent() { return m_Settings.AmmoPercentageSetting.GetRandomPercentage() / m_Settings.AmmoPercentageSetting.Max; }
	
	//! Time after last player interaction loot can start to respawn
	int GetRespawnAfterLastInteractionInMinutes() { return m_Settings.RespawnSettings.RespawnAfterLastInteractionInMinutes; }
	
	//! Number of items that can respawn, at most, overtime
	int GetRespawnLootItemThreshold() { return m_Settings.RespawnSettings.NumberOfItemsToSpawnPerContainer; }
	
	int GetRespawnCheckInterval() { return m_Settings.RespawnSettings.RespawnLootTimerInSeconds; }

	static void RegisterLootableContainer(TW_LootableInventoryComponent container)
	{
		if(s_GlobalContainerGrid)
			s_GlobalContainerGrid.InsertByWorld(container.GetOwner().GetOrigin(), container);
	}
	
	static void UnregisterLootableContainer(TW_LootableInventoryComponent container)
	{
		if(s_GlobalContainerGrid)
			s_GlobalContainerGrid.RemoveByWorld(container.GetOwner().GetOrigin(), container);
	}
		
	//! Is this resource in the global items set? - IF not --> invalid.
	static bool IsValidItem(ResourceName resource)
	{
		return s_GlobalItems.Contains(resource);
	}
	
	static bool FlagHasResource(SCR_EArsenalItemType flags, ResourceName resource)
	{
		if(resource.IsEmpty())
			return false;
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			if(!s_LootTable.Contains(itemType))
				continue;
			
			if(!SCR_Enum.HasFlag(flags, itemType))
				continue;
			
			ref array<ref TW_LootConfigItem> items = s_LootTable.Get(itemType);
			foreach(ref TW_LootConfigItem item : items)
			{
				if(item.resourceName == resource)
					return true;
			}
		}
		
		return false;
	}
			
	void SelectRandomPrefabsFromFlags(SCR_EArsenalItemType flags, int count, notnull map<string, int> selected, TW_ResourceNameType type = TW_ResourceNameType.DisplayName)
	{
		int selectedCount = 0;
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			if(selectedCount >= count)
				return;
			
			if(!SCR_Enum.HasFlag(flags, itemType))
				continue;
			
			if(!s_LootTable.Contains(itemType))
				continue;
			
			ref array<ResourceName> items = {};
			selectedCount += SelectRandomPrefabsFromType(itemType, Math.RandomIntInclusive(1, count), items);
			
			foreach(ResourceName name : items)
			{
				string value = name;
				switch(type)
				{
					case TW_ResourceNameType.DisplayName:
						value = WidgetManager.Translate(TW_Util.GetPrefabDisplayName(name));
						break;
				}
				
				if(selected.Contains(value))
					selected.Set(value, selected.Get(value) + 1);
				else
					selected.Set(value, 1);
			}
		}
		
		// If we didn't select enough, we'll do it again.
		// Subtracting what we have from the desired amount
		if(selectedCount < count)
			SelectRandomPrefabsFromFlags(flags, count - selectedCount, selected, type);
	}
	
	void PrintSettings()
	{
		if(!IsDebug())
			return;
		
		Print("------------------------");
		Print("TrainWreck Loot Settings");
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			string typeName = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
			ref array<ref TW_LootConfigItem> items = s_LootTable.Get(itemType);
			PrintFormat("Arsenal Category: %1, Count: %2", typeName, items.Count());
		}
		Print("------------------------");
	}
	
	int SelectRandomPrefabsFromType(SCR_EArsenalItemType flag, int randomCount, notnull array<ResourceName> selected)
	{
		if(!s_LootTable.Contains(flag))
			return 0;
		
		ref array<ref TW_LootConfigItem> items = s_LootTable.Get(flag);
		ref set<int> indicies = new set<int>();
		
		int count = 0;
		for(int i = 0; i < randomCount; i++)
		{			
			ref TW_LootConfigItem item = items.GetRandomElement();
			
			int randomIndex = items.GetRandomIndex();
			
			while(indicies.Contains(randomIndex) && selected.Count() < items.Count())
				randomIndex = items.GetRandomIndex();	
			
			if(indicies.Contains(randomIndex))
				break;
			
			count++;
			indicies.Insert(randomIndex);
			selected.Insert(items.Get(randomIndex).resourceName);
		}
		
		return count;
	}
	
	array<SCR_EntityCatalogEntry> GetMergedFactionCatalogs()
	{		
		SCR_FactionManager manager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		if(!manager)
		{
			PrintFormat("TrainWreckLooting: Looting requires a faction manager to be present", LogLevel.ERROR);
			return null;
		}
		
		ref array<Faction> factions = {};
		manager.GetFactionsList(factions);
		
		SCR_EntityCatalog mainCatalog;
		ref array<SCR_EntityCatalogEntry> entries = {};
		ref array<SCR_EntityCatalogEntry> current = {};
		
		foreach(Faction fac : factions)
		{
			SCR_Faction faction = SCR_Faction.Cast(fac);
			
			if(!faction) 
				continue;
			
			SCR_EntityCatalog itemCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);			
			
			if(!itemCatalog)
			{
				PrintFormat("TrainWreckLooting: Faction does not have an item catalog: %1", WidgetManager.Translate(faction.GetFactionName()), LogLevel.ERROR);
				return null;
			}
			
			current.Clear();
			itemCatalog.GetEntityList(current);
			entries.InsertAll(current);
		}
		
		return entries;
	}
	
	void InitializeLootTable()
	{
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, s_ArsenalItemTypes);
		
		// If lootmap already exists -- load everything from file
		// Then merge things that are in-game
		if(HasLootTable())
		{
			Print(string.Format("TrainWreck: Detected loot table %1", LootFileName));
			IngestLootTableFromFile(m_Settings);
		}
		else m_Settings = new LootManagerSettings();			
		
		ref array<SCR_EntityCatalogEntry> catalogItems = GetMergedFactionCatalogs();
		int entityCount = catalogItems.Count();
		
		foreach(auto entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = {};
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching arsenal items 
			foreach(auto data : itemData)
			{
				SCR_ArsenalItem arsenalItem = SCR_ArsenalItem.Cast(data);
				
				if(!arsenalItem)
					continue;
				
				if(!arsenalItem.IsEnabled())
					break;
				
				auto itemType = arsenalItem.GetItemType();
				auto itemMode = arsenalItem.GetItemMode();
				ResourceName prefab = entry.GetPrefab();
				
				// If we already had a lootmap from file
				// and the items is loaded -- ignore readding it
				if(s_GlobalItems.Contains(prefab))
					continue;
				
				s_GlobalItems.Insert(prefab);
				
				arsenalItem.SetItemPrefab(prefab);
				
				int defaultCount = 1;
				int defaultChance = 25;
				
				//if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "MagazineComponent"))
				//	defaultCount = 4;
								
				//if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "SCR_RestrictedDeployableSpawnPointComponent"))
				//	defaultChance = 5;	
				
				//if(prefab.Contains("RearmingKit") || prefab.Contains("MedicalKit"))
				//	arsenalItem.SetShouldSpawn(false);
				
				arsenalItem.SetItemMaxSpawnCount(defaultCount);
				arsenalItem.SetItemChanceToSpawn(defaultChance);
				
				ref TW_LootConfigItem config = new TW_LootConfigItem();
				
				config.SetData(prefab, defaultChance, defaultCount, null, arsenalItem.ShouldSpawn());
				
				if(!s_LootTable.Contains(itemType))
					s_LootTable.Insert(itemType, {});
				
				s_LootTable.Get(itemType).Insert(config);
			}
		}
		
		bool success = OutputLootTableFile();
		if(!success)
			Print(string.Format("TrainWreck: Failed to write %1", LootFileName), LogLevel.ERROR);
		
		HasLoaded = true;
		
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{
			int count = items.Count();
			
			for(int i = 0; i < count; i++)
			{
				ref TW_LootConfigItem configItem = items.Get(i);
				if(configItem.isEnabled || configItem.chanceToSpawn <= 0) 
					continue;
				
				PrintFormat("TrainWreck-Looting: Removing '%1'. Enabled(%2) | Chance(%3)", configItem.resourceName, configItem.isEnabled, configItem.chanceToSpawn);
				items.RemoveItem(configItem);
				i -= 1;
				count -= 1;
			}
		}
		
	}
	
	
	//! Initialize the entire loot system
	void Initialize()
	{
		m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		if(!TW_MonitorPositions.GetInstance())
		{
			Print("TrainWreck: Position monitor is null. Unable to initialize Loot Manager", LogLevel.ERROR);
			return;
		}
		
		m_GameMode.GetOnInitializePlugins().Insert(AddListeners);
		
		m_GameMode.GetOnGameStarted().Insert(DelayInitialize);
	}
	
	const string LootSpawnRadius = "TRAINWRECK_LOOT_RADIUS";
	const string LootAntiSpawnRadius = "TRAINWRECK_LOOT_ANTIRADIUS";
	
	private void AddListeners()
	{
		ref TW_MonitorPositions monitor = TW_MonitorPositions.GetInstance();
		
		InitializeLootTable();
		
		if(m_Settings.RespawnSettings.IsLootRespawnable)
		{
			ref TW_OnPlayerPositionsChangedInvoker onRadiusCallback = monitor.AddGridSubscription(LootSpawnRadius, m_Settings.RespawnSettings.GridSize, m_Settings.RespawnSettings.RespawnLootRadius);
			ref TW_OnPlayerPositionsChangedInvoker onAntiRadiusCallback = monitor.AddGridSubscription(LootAntiSpawnRadius, m_Settings.RespawnSettings.GridSize, m_Settings.RespawnSettings.DeadZoneRadius);
			
			onRadiusCallback.Insert(OnPlayerPositionsChanged);
			onAntiRadiusCallback.Insert(OnPlayerPositionsChanged_AntiRadius);
		}
	}
	
	private void OnPlayerPositionsChanged(GridUpdateEvent gridInfo)
	{
		m_PlayerLocations.Clear();
		m_PlayerLocations.Copy(gridInfo.GetPlayerChunks());
	}
	
	private void OnPlayerPositionsChanged_AntiRadius(GridUpdateEvent gridInfo)
	{
		m_AntiSpawnPlayerLocations.Clear();
		m_AntiSpawnPlayerLocations.Copy(gridInfo.GetPlayerChunks());
	}
	
	//! This should keep
	private void OnLootGridSizeChanged(int oldSize, int newSize)
	{
		ref array<TW_LootableInventoryComponent> entries = {};
		int count = s_GlobalContainerGrid.GetAllItems(entries);
		
		/*
			s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(newGrid.GetGridSize());
		
		foreach(TW_LootableInventoryComponent container : entries)
			if(container)
				s_GlobalContainerGrid.InsertByWorld(container.GetOwner().GetOrigin(), container);
		*/
	}
	
	private void DelayInitialize()
	{
		if(IsDebug())
			Print("TrainWreck: Initializing Loot System");
		
		if(m_Settings.RespawnSettings.IsLootRespawnable)
		{
			if(IsDebug())
				Print("TrainWreck: Loot Respawn System Enabled...");
			
			GetGame().GetCallqueue().CallLater(RespawnLootProcessor, 1000 * GetRespawnCheckInterval(), true);
		}	
	}
	
	private static ref set<string> m_PlayerLocations = new set<string>();
	private static ref set<string> m_AntiSpawnPlayerLocations = new set<string>();
	private static ref array<int> m_PlayerIds = new array<int>();
	private static ref set<TW_LootableInventoryComponent> m_InteractedWithContainers = new set<TW_LootableInventoryComponent>();
	private static int m_RespawnLootProcessor_ContainerIndex = -1;
	private static int m_RespawnLootProcessor_BatchSize = 10;
	
	static void RegisterInteractedContainer(TW_LootableInventoryComponent container)
	{
		if(!m_InteractedWithContainers.Contains(container))
			m_InteractedWithContainers.Insert(container);
	}
	
	static void UnregisterInteractedContainer(TW_LootableInventoryComponent container)
	{
		if(m_InteractedWithContainers.Contains(container))
		{
			m_InteractedWithContainers.RemoveItem(container);
			container.SetInteractedWith(false);
		}		
	}
	
	private static int GetNextIndex(int current, int length)
	{
		current += 1;
		
		if(current >= length || current < 0)
			return 0;
		
		return current;
	}
	
	private static int GetPreviousIndex(int current, int length)
	{
		current -= 1;
		
		if(current < 0)
			current = length - 1;
		
		return current;
	}
	
	static void RespawnLootProcessor()
	{
		int length = m_InteractedWithContainers.Count();
		
		if(TW_LootManager.GetInstance().IsDebug())
		{
			PrintFormat("TrainWreck: # of looted containers: %1", length);
		}
		
		// Nothing to process if we don't have containers to check
		if(length == 0) 
			return;	
		
		int processLength = m_RespawnLootProcessor_BatchSize;
		
		if(processLength > length)
			processLength = length;
		
		// We want to make sure if we hit the upper boundary
		// we rollover to the beginning
		for(int i = 0; i < processLength; i++)
		{
			m_RespawnLootProcessor_ContainerIndex = GetNextIndex(m_RespawnLootProcessor_ContainerIndex, length);
			TW_LootableInventoryComponent container = m_InteractedWithContainers.Get(m_RespawnLootProcessor_ContainerIndex);
			
			if(!container)
			{
				if(TW_LootManager.GetInstance().IsDebug())
				{
					Print("TrainWreck: Loot Container is null: removing from list...", LogLevel.WARNING);
				}
				
				m_InteractedWithContainers.Remove(m_RespawnLootProcessor_ContainerIndex);
				length -= 1;
				m_RespawnLootProcessor_ContainerIndex = GetPreviousIndex(m_RespawnLootProcessor_ContainerIndex, length);
				continue;
			}
			
			if(TW_LootManager.GetInstance().IsDebug())
			{
				Print("TrainWreck: Can spawn loot: %1", container.CanRespawnLoot());
			}
			
			string coordinate = TW_Util.ToGridText(container.GetOwner().GetOrigin(), TW_LootManager.GetInstance().GetLootSettings().RespawnSettings.GridSize);
			
			if(TW_LootManager.GetInstance().IsDebug())
			{
				PrintFormat("TrainWreck: Loot Container Coordinate: %1", coordinate);
			}
			
			if(m_AntiSpawnPlayerLocations.Contains(coordinate))
			{
				if(TW_LootManager.GetInstance().IsDebug())
				{
					PrintFormat("TrainWreck: %1 - is within a no-respawn area around a player", coordinate, LogLevel.WARNING);
				}
				
				continue;
			}
			
			if(!container.CanRespawnLoot())
				continue;
			
			container.SetInteractedWith(false);
			m_InteractedWithContainers.Remove(m_RespawnLootProcessor_ContainerIndex);
			
			length -= 1;
			
			m_RespawnLootProcessor_ContainerIndex = GetPreviousIndex(m_RespawnLootProcessor_ContainerIndex, length);
		}		
	}
		
	//! Trickle spawn loot into a container
	void TrickleSpawnLootInContainer(TW_LootableInventoryComponent container, int remainingAmount)
	{
		if(!m_Settings.IsLootEnabled|| !container || remainingAmount < 0) 
			return;
		
		ref WeightedType<ref TW_LootConfigItem> pool = new WeightedType<ref TW_LootConfigItem>();
		GetLootPoolForContainer(container.GetTypeFlags(), pool);
		TW_LootConfigItem arsenalItem = pool.GetRandomItem();
		
		if(!arsenalItem)
		{
			GetGame().GetCallqueue().CallLater(TrickleSpawnLootInContainer, 250, false, container, remainingAmount - 1);
			return;
		}
		
		if(remainingAmount <= 0) 
			return;
	
		int itemCount = Math.RandomIntInclusive(1, arsenalItem.randomSpawnCount);
		container.InsertItem(arsenalItem);
		
		GetGame().GetCallqueue().CallLater(TrickleSpawnLootInContainer, 250, false, container, remainingAmount - 1);
	}
	
	//! Spawn loot in designated container
	void SpawnLootInContainer(TW_LootableInventoryComponent container)
	{
		if(!m_Settings.IsLootEnabled) 
			return;
		
		if(!container) 
			return;
			
		int spawnCount = Math.RandomIntInclusive(1, 4);
		
		ref WeightedType<ref TW_LootConfigItem> pool = new WeightedType<ref TW_LootConfigItem>();
		GetLootPoolForContainer(container.GetTypeFlags(), pool);
		
		// How many different things are we going to try spawning?								
		for(int i = 0; i < spawnCount; i++)
		{				
			ref TW_LootConfigItem arsenalItem = pool.GetRandomItem();
								
			if(!arsenalItem)
				continue;
			
			// Add item a random amount of times to the container based on settings
			int itemCount = Math.RandomIntInclusive(1, arsenalItem.randomSpawnCount);
			bool tryAgain = false;
			for(int x = 0; x < itemCount; x++)
			{
				bool success = container.InsertItem(arsenalItem);
					
				if(!success)
				{
					tryAgain = true;
					break;
				}
			}
				
			if(tryAgain)
				spawnCount--;
		}
	}
	
	static TW_LootConfigItem GetRandomByFlag(int type)
	{				
		if(type <= 0)
			return null;
		
		array<SCR_EArsenalItemType> selectedItems = {};
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
			if(SCR_Enum.HasFlag(type, itemType) && s_LootTable.Contains(itemType))
				selectedItems.Insert(itemType);
		
		// Check if nothing was selected
		if(selectedItems.IsEmpty())
			return null;
		
		auto items = s_LootTable.Get(selectedItems.GetRandomElement());
		
		// Check if nothing was available
		if(!items || items.IsEmpty())
			return null;
		
		ref TW_LootConfigItem item = items.GetRandomElement();
		
		if(item.isEnabled) return item;
		return null;
	}
	
	static void GetLootPoolForContainer(SCR_EArsenalItemType flags, notnull out WeightedType<ref TW_LootConfigItem> pool)
	{		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
		{
			if(SCR_Enum.HasFlag(flags, itemType) && s_LootTable.Contains(itemType))
			{
				ref array<ref TW_LootConfigItem> entries = s_LootTable.Get(itemType);				
				foreach(TW_LootConfigItem item : entries)
					if(item.isEnabled && item.chanceToSpawn > 0)
						pool.Add(item, item.chanceToSpawn);
			}
		}
	}
	
	static int GetWeapons(notnull array<ref TW_LootConfigItem> weapons)
	{
		int count = 0;
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem>> items : s_LootTable)
		{
			foreach(TW_LootConfigItem item : items)
			{				
				if(!SCR_BaseContainerTools.FindComponentSource(Resource.Load(item.resourceName), "WeaponComponent"))
				{
					continue;
				}
				weapons.Insert(item);
				count++;
			}
		}
		
		return count;
	}
	
	static int GetPrefabsOfType(SCR_EArsenalItemType type, notnull array<ref TW_LootConfigItem> items, string ensureHasComponent = string.Empty)
	{
		int count = 0;
		ref array<ref TW_LootConfigItem> configs = {};
		
		foreach(SCR_EArsenalItemType flagType : s_ArsenalItemTypes)
		{
			if(SCR_Enum.HasFlag(type, flagType))
			{				
				configs = s_LootTable.Get(flagType);
				
				foreach(TW_LootConfigItem config : configs)
				{
					if(ensureHasComponent != string.Empty)
					{
						if(!SCR_BaseContainerTools.FindComponentSource(Resource.Load(config.resourceName), ensureHasComponent))
							continue;
					}
					count++;
					items.Insert(config);
				}
			}
		}	
		
		return count;
	}
	
	private bool OutputLootTableFile()
	{
		if(!m_Settings.LootTable)
			m_Settings.LootTable = new map<string, ref array<ref TW_LootConfigItem>>();
				
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{	
			PrintFormat("TrainWreckLooting: Type: %1, Amount %2 -- Saving", TW_Util.ArsenalTypeAsString(type), items.Count());
						
			m_Settings.LootTable.Set(TW_Util.ArsenalTypeAsString(type), items);
		}
		
		return TW_Util.SaveJsonFile(LootFileName, m_Settings, true);
	}
	
	private static bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile(LootFileName);
	}
	
	private static bool IngestLootTableFromFile(out LootManagerSettings settings)
	{
		SCR_JsonLoadContext context = TW_Util.LoadJsonFile(LootFileName, true);
		bool loadSuccess = context.ReadValue("", settings);
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}
		
		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);
		
		ref map<string, SCR_EArsenalItemType> typeMap = new map<string, SCR_EArsenalItemType>();
		foreach(SCR_EArsenalItemType type : itemTypes)
			typeMap.Set(TW_Util.ArsenalTypeAsString(type), type);
		
		foreach(string name, ref array<ref TW_LootConfigItem> items : settings.LootTable)
		{
			if(!typeMap.Contains(name))
			{
				PrintFormat("TrainWreck: JsonFile '%1' -> Invalid SCR_EArsenalItemType '%2'. Skipping Section...", LootFileName, name, LogLevel.ERROR);
				continue;
			}
			
			SCR_EArsenalItemType itemType = typeMap.Get(name);
			
			if(!s_LootTable.Contains(itemType))
				s_LootTable.Insert(itemType, {});
			
			foreach(TW_LootConfigItem item : items)
			{
				Resource resource = Resource.Load(item.resourceName);
				if(!resource.IsValid())
				{
					PrintFormat("TrainWreck: LootType('%1') -> Prefab Invalid: '%2'", name, item.resourceName, LogLevel.WARNING);
					continue;
				}		
				
				if(HasLoaded)
					PrintFormat("TrainWreck: Item: %1, Chance: %2", item.resourceName, item.chanceToSpawn);
				
				if(!s_GlobalItems.Contains(item.resourceName))
					s_GlobalItems.Insert(item.resourceName);
				else 
					continue;
				
				s_LootTable.Get(itemType).Insert(item);
			}
		}
		
		return true;
	}
};
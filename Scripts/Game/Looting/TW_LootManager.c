enum TW_ResourceNameType
{
	ResourceName,
	DisplayName
};

class ScavLootSettings
{
	float spawnWithBackpackChance = 0.1;
	float spawnWithTwoWeaponsChance = 0.1;
	float spawnWithHealChance = 0.25;
	float spawnWithVestChance = 0.3;
};

class WeightedType<Class T>
{
	private ref array<T> items = {};
	private ref array<float> weights = {};
	
	T GetRandomItem()
	{
		if(items.IsEmpty() || weights.IsEmpty())
			return null;
		
		int index = SCR_ArrayHelper.GetWeightedIndex(weights, Math.RandomFloat01());
		return items.Get(index);
	}
	
	void Add(T item, float weight)
	{
		items.Insert(item);
		weights.Insert(weight);
	}
};

sealed class TW_LootManager 
{
	// Provide the ability to grab 
	private static ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> s_LootTable = new map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();
	
	// This should contain the resource names of all items that are valid for saving/loading 
	private static ref set<string> s_GlobalItems = new set<string>();
	
	private static ref array<SCR_EArsenalItemType> s_ArsenalItemTypes = {};
	
	static const string LootFileName = "$profile:lootmap.json";	
	
	//! Should magazines spawn with weapons
	static bool ShouldSpawnMagazine = true;
	
	//! Is the loot system enabled
	static bool IsLootEnabled = true;
	
	//! Minimum percentage of ammo magazines can spawn with (if spawn with magazine is enabled)
	private static int m_MinimumAmmoPercent = 80;
	
	//! Maximum percentage of ammo magazines can spawn with (if spawn with magazine is enabled)
	private static int m_MaximumAmmoPercent = 100;
	
	//! Size of grid squares the loot system shall utilize
	private static int m_GridSize = 100;
	
	//! Loot will not respawn within this amount of grid squares around player
	private static int m_DeadZoneRadius = 1;
	
	//! Loot will respawn outside the dead zone and within this amount of grid squares
	private static int m_RespawnLootRadius = 5;
	
	// Can loot respawn
	private static int m_IsLootRespawnable = true;
	
	//! Loot timer will check statuses at this interval
	private static int m_RespawnLootTimerInSeconds = 10;
	
	//! Time since last player interaction before being considered for loot respawn
	private static int m_RespawnAfterLastInteractionInMinutes = 1;
	
	//! Number of items that can respawn at most overtime
	private static int m_RespawnLootItemThreshold = 4;
	
	//! Ratio to apply to entity sizes for unsearched containers
	private static float m_UnlootedTimeRatio = 0.50;
	
	//! Ratio to apply to entity sizes for searched containers
	private static float m_SearchedTimeRatio = 0.1;
	
	
	private static ref ScavLootSettings m_ScavSettings = new ScavLootSettings();
	
	private static ref TW_GridCoordArrayManager<TW_LootableInventoryComponent> s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(100);
	
	static TW_GridCoordArrayManager<TW_LootableInventoryComponent> GetContainerGrid() { return s_GlobalContainerGrid; }
	
	static float GetUnlootedTimeRatio() { return m_UnlootedTimeRatio; }
	static float GetSearchedTimeRatio() { return m_SearchedTimeRatio; }
	
	static ScavLootSettings GetScavSettings() { return m_ScavSettings; }
	
	static float GetRandomAmmoPercent() { return Math.RandomFloatInclusive(m_MinimumAmmoPercent, m_MaximumAmmoPercent) / m_MaximumAmmoPercent; }
	
	//! Time after last player interaction loot can start to respawn
	static int GetRespawnAfterLastInteractionInMinutes() { return m_RespawnAfterLastInteractionInMinutes; }
	
	//! Number of items that can respawn, at most, overtime
	static int GetRespawnLootItemThreshold() { return m_RespawnLootItemThreshold; }
	
	static int GetRespawnCheckInterval() { return m_RespawnLootTimerInSeconds; }

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
			
	static void SelectRandomPrefabsFromFlags(SCR_EArsenalItemType flags, int count, notnull map<string, int> selected, TW_ResourceNameType type = TW_ResourceNameType.DisplayName)
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
	
	static int SelectRandomPrefabsFromType(SCR_EArsenalItemType flag, int randomCount, notnull array<ResourceName> selected)
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
	
	static array<SCR_EntityCatalogEntry> GetMergedFactionCatalogs()
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
	
	static void InitializeLootTable()
	{
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, s_ArsenalItemTypes);
		
		// If lootmap already exists -- load everything from file
		// Then merge things that are in-game
		if(HasLootTable())
		{
			Print(string.Format("TrainWreck: Detected loot table %1", LootFileName));
			IngestLootTableFromFile();
		}
		
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
				
				if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "MagazineComponent"))
					defaultCount = 4;
				
				if(SCR_BaseContainerTools.FindComponentSource(Resource.Load(prefab), "SCR_RestrictedDeployableSpawnPointComponent"))
					defaultChance = 5;	
				
				if(prefab.Contains("RearmingKit") || prefab.Contains("MedicalKit"))
					arsenalItem.SetShouldSpawn(false);		
				
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
		IngestLootTableFromFile();
		
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{
			int count = items.Count();
			
			for(int i = 0; i < count; i++)
			{
				ref TW_LootConfigItem configItem = items.Get(i);
				if(configItem.isEnabled) 
					continue;
				
				PrintFormat("TrainWreck-Looting: Removing '%1'. Enabled(%2) | Chance(%3)", configItem.resourceName, configItem.isEnabled, configItem.chanceToSpawn);
				items.RemoveItem(configItem);
				i -= 1;
				count -= 1;
			}
		}
		
	}
	
	//! Initialize the entire loot system
	static void Initialize()
	{
		GetGame().GetCallqueue().CallLater(DelayInitialize, 5000, false);
	}
	
	private static void DelayInitialize()
	{
		InitializeLootTable();
		
		ref array<TW_LootableInventoryComponent> entries = {};
		int count = s_GlobalContainerGrid.GetAllItems(entries);
		
		s_GlobalContainerGrid = new TW_GridCoordArrayManager<TW_LootableInventoryComponent>(m_GridSize);
		
		foreach(TW_LootableInventoryComponent container : entries)
			if(container)
				s_GlobalContainerGrid.InsertByWorld(container.GetOwner().GetOrigin(), container);
		
		if(IsLootEnabled)
		{
			GetGame().GetCallqueue().CallLater(CheckPlayerLocations, 1000 * 10, true);
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
	
	private static void CheckPlayerLocations()
	{
		ref set<string> currentPlayerChunks = new set<string>();
		
		m_PlayerIds.Clear();
		GetGame().GetPlayerManager().GetPlayers(m_PlayerIds);
		
		IEntity player;
		
		ref set<string> chunksAroundPlayer = new set<string>();
		bool locationsChanged = false;
		
		// Figure out where players are
		foreach(int playerId : m_PlayerIds)
		{
			player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!player)
				continue;
			
			chunksAroundPlayer.Clear();
			TW_Util.AddSurroundingGridSquares(m_AntiSpawnPlayerLocations, player.GetOrigin(), m_DeadZoneRadius, m_GridSize);
			TW_Util.AddSurroundingGridSquares(chunksAroundPlayer, player.GetOrigin(), m_RespawnLootRadius, m_GridSize);	
			
			foreach(string chunk : chunksAroundPlayer)
			{
				if(!m_PlayerLocations.Contains(chunk))
				{
					locationsChanged = true;
					m_PlayerLocations.Insert(chunk);
				}
				
				if(!currentPlayerChunks.Contains(chunk))
					currentPlayerChunks.Insert(chunk);
			}
		}
		
		// Remove old positions that aren't in use anymore
		int count = m_PlayerLocations.Count();
		for(int i = 0; i < count; i++)
		{
			string chunk = m_PlayerLocations.Get(i);
			if(!currentPlayerChunks.Contains(chunk))
			{
				m_PlayerLocations.Remove(i);
				i -= 1;
				count -= 1;
			}
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
				m_InteractedWithContainers.Remove(m_RespawnLootProcessor_ContainerIndex);
				length -= 1;
				m_RespawnLootProcessor_ContainerIndex = GetPreviousIndex(m_RespawnLootProcessor_ContainerIndex, length);
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
	static void TrickleSpawnLootInContainer(TW_LootableInventoryComponent container, int remainingAmount)
	{
		if(!IsLootEnabled || !container || remainingAmount < 0) 
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
	static void SpawnLootInContainer(TW_LootableInventoryComponent container)
	{
		if(!IsLootEnabled) 
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
				configs.Clear();
				configs = s_LootTable.Get(flagType);
				
				foreach(auto config : configs)
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
	
	private static bool OutputLootTableFile()
	{
		ContainerSerializationSaveContext saveContext = new ContainerSerializationSaveContext();
		PrettyJsonSaveContainer prettyContainer = new PrettyJsonSaveContainer();
		saveContext.SetContainer(prettyContainer);
		
		saveContext.WriteValue("scavSettings", m_ScavSettings);
		saveContext.WriteValue("magazineMinAmmoPercent", m_MinimumAmmoPercent);		
		saveContext.WriteValue("magazineMaxAmmoPercent", m_MaximumAmmoPercent);
		saveContext.WriteValue("shouldSpawnMagazine", ShouldSpawnMagazine);
		saveContext.WriteValue("isLootEnabled", IsLootEnabled);
		saveContext.WriteValue("isRespawnLootEnabled", m_IsLootRespawnable);
		saveContext.WriteValue("gridSize", m_GridSize);
		saveContext.WriteValue("deadZoneGridRadius", m_DeadZoneRadius);
		saveContext.WriteValue("respawnLootTimerInSeconds", m_RespawnLootTimerInSeconds);
		saveContext.WriteValue("respawnAfterLastInteractionInMinutes", m_RespawnAfterLastInteractionInMinutes);
		saveContext.WriteValue("respawnLootItemThreshold", m_RespawnLootItemThreshold);
		saveContext.WriteValue("respawnLootRadius", m_RespawnLootRadius);		
		saveContext.WriteValue("unsearchedTimeRatio", m_UnlootedTimeRatio);
		saveContext.WriteValue("searchedTimeRatio", m_SearchedTimeRatio);
		saveContext.WriteValue("respawnLootProcessorBatchSize", m_RespawnLootProcessor_BatchSize);		
		
		foreach(SCR_EArsenalItemType type, ref array<ref TW_LootConfigItem> items : s_LootTable)
		{	
			PrintFormat("TrainWreckLooting: Type: %1, Amount %2 -- Saving", TW_Util.ArsenalTypeAsString(type), items.Count());
			
			ref array<ref TW_LootConfigItem> typeLoot = {};			
			saveContext.WriteValue(TW_Util.ArsenalTypeAsString(type), items);
		}
		
		return prettyContainer.SaveToFile(LootFileName);
	}
	
	private static bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile(LootFileName);
	}
	
	private static bool IngestLootTableFromFile()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool loadSuccess = loadContext.LoadFromFile(LootFileName);
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}
		
		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);
		string name = string.Empty;
		
		loadContext.ReadValue("magazineMinAmmoPercent", m_MinimumAmmoPercent);
		loadContext.ReadValue("magazineMaxAmmoPercent", m_MaximumAmmoPercent);
		loadContext.ReadValue("shouldSpawnMagazine", ShouldSpawnMagazine);
		loadContext.ReadValue("isLootEnabled", IsLootEnabled);
		loadContext.ReadValue("isRespawnLootEnabled", m_IsLootRespawnable);
		loadContext.ReadValue("gridSize", m_GridSize);
		loadContext.ReadValue("deadZoneGridRadius", m_DeadZoneRadius);
		loadContext.ReadValue("respawnLootTimerInSeconds", m_RespawnLootTimerInSeconds);
		loadContext.ReadValue("respawnAfterLastInteractionInMinutes", m_RespawnAfterLastInteractionInMinutes);
		loadContext.ReadValue("respawnLootItemThreshold", m_RespawnLootItemThreshold);
		loadContext.ReadValue("respawnLootRadius", m_RespawnLootRadius);
		loadContext.ReadValue("unsearchedTimeRatio", m_UnlootedTimeRatio);
		loadContext.ReadValue("searchedTimeRatio", m_SearchedTimeRatio);
		loadContext.ReadValue("respawnLootProcessorBatchSize", m_RespawnLootProcessor_BatchSize);
		loadContext.ReadValue("scavSettings", m_ScavSettings);
		
		foreach(SCR_EArsenalItemType itemType : itemTypes)
		{
			if(!LoadSection(loadContext, itemType))
			{
				name = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
				Print(string.Format("TrainWreck: LootMap: Unable to load %1", name), LogLevel.ERROR);
			}
		}
		
		return true;
	}
	
	private static bool LoadSection(notnull SCR_JsonLoadContext context, SCR_EArsenalItemType type)
	{
		ref array<ref TW_LootConfigItem> items = {};
		string keyValue = TW_Util.ArsenalTypeAsString(type);
		
		bool success = context.ReadValue(keyValue, items);
		
		if(!success)
		{
			s_LootTable.Insert(type, {});
			return false;
		}
		
		if(!s_LootTable.Contains(type))
			s_LootTable.Insert(type, {});
		
		foreach(TW_LootConfigItem item : items)
		{
			Resource resource = Resource.Load(item.resourceName);
			if(resource.IsValid())
			{
				if(!item.isEnabled)
					continue;
				
				if(!s_GlobalItems.Contains(item.resourceName))
					s_GlobalItems.Insert(item.resourceName);
				else
					continue;
				
				s_LootTable.Get(type).Insert(item);
			}
			else
				PrintFormat("TrainWreckLooting: The following resource is invalid (maybe mod is not loaded?): %1", item.resourceName, LogLevel.WARNING);
		}
		
		PrintFormat("TrainWreckLooting: Type %1, Amount: %2", type, s_LootTable.Get(type).Count());
		return true;
	}
};
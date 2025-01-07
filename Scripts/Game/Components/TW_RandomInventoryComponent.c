class WeightedPrefabs
{
	private ref array<string> m_Prefabs = {};
	private ref array<float> m_Weights = {};
	
	void AddConfigItem(TW_LootConfigItem item)
	{
		if(!item.isEnabled || item.chanceToSpawn <= 0)
			return;
		
		m_Prefabs.Insert(item.resourceName);
		m_Weights.Insert(item.chanceToSpawn);
	}
	
	string GetRandomPrefab()
	{
		if(m_Prefabs.IsEmpty())
			return string.Empty;
		
		int index = SCR_ArrayHelper.GetWeightedIndex(m_Weights, Math.RandomFloat01());
		return m_Prefabs.Get(index);
	}
}

class TW_RandomInventoryComponentClass : ScriptComponentClass {};
class TW_RandomInventoryComponent : ScriptComponent
{
	private BaseWeaponManagerComponent m_WeaponManager;
	private RplComponent m_RplComponent;
	private SCR_CharacterControllerComponent m_Controller;
	private SCR_InventoryStorageManagerComponent m_StorageManager;
	
	private static ref WeightedPrefabs m_WeaponPrefabs;
	private static ref WeightedPrefabs m_VestPrefabs;
	private static ref WeightedPrefabs m_BackpackPrefabs;
	private static ref WeightedPrefabs m_PantsPrefabs;
	private static ref WeightedPrefabs m_ShirtPrefabs;
	private static ref WeightedPrefabs m_FootwearPrefabs;
	private static ref WeightedPrefabs m_HeadwearPrefabs;
	private static ref WeightedPrefabs m_HealPrefabs;
	
	private static bool Initialized = false;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		if(!TW_Global.IsServer(owner))
			return;
		
		m_WeaponManager = TW<BaseWeaponManagerComponent>.Find(owner);
		m_RplComponent = TW<RplComponent>.Find(owner);
		m_Controller = TW<SCR_CharacterControllerComponent>.Find(owner);
		m_StorageManager = TW<SCR_InventoryStorageManagerComponent>.Find(GetOwner());	
		GetGame().GetCallqueue().CallLater(InitializeLoadout, 1000, false);
	}
	
	private void InitializeType(SCR_EArsenalTypes type, out WeightedPrefabs entry, string hasComponent = string.Empty)
	{
		entry = new WeightedPrefabs();
		ref array<ref TW_LootConfigItem> configs = {};
		int count = TW_LootManager.GetPrefabsOfType(type, configs, hasComponent);
		
		if(count == 0)
			return;
		
		foreach(TW_LootConfigItem config : configs)
			entry.AddConfigItem(config);
	}
	
	private void InitializeWeightSystem()
	{
		ref array<ref TW_LootConfigItem> weapons = {};
		int weaponCount = TW_LootManager.GetWeapons(weapons);
		
		if(weaponCount <= 0)
		{
			PrintFormat("TrainWreckLootingSystem: Was unable to locate weapons from loot table", LogLevel.ERROR);
			return;
		}
		
		InitializeType(SCR_EArsenalItemType.RIFLE | SCR_EArsenalItemType.MACHINE_GUN | SCR_EArsenalItemType.SNIPER_RIFLE | SCR_EArsenalItemType.PISTOL, m_WeaponPrefabs, "WeaponComponent");
		InitializeType(SCR_EArsenalItemType.HEADWEAR, m_HeadwearPrefabs);
		InitializeType(SCR_EArsenalItemType.TORSO, m_ShirtPrefabs);
		InitializeType(SCR_EArsenalItemType.LEGS, m_PantsPrefabs);
		InitializeType(SCR_EArsenalItemType.FOOTWEAR, m_FootwearPrefabs);
		InitializeType(SCR_EArsenalItemType.BACKPACK, m_BackpackPrefabs);
		InitializeType(SCR_EArsenalItemType.VEST_AND_WAIST, m_VestPrefabs);
		InitializeType(SCR_EArsenalItemType.HEAL, m_HealPrefabs);
		
		Initialized = true;
	}
	
	private bool SpawnEquipment(ResourceName prefab)
	{
		if(prefab == ResourceName.Empty)
			return false;
		
		return m_StorageManager.TrySpawnPrefabToStorage(prefab, purpose: EStoragePurpose.PURPOSE_LOADOUT_PROXY);
	}
	
	private bool SpawnItem(ResourceName prefab)
	{
		return m_StorageManager.TrySpawnPrefabToStorage(prefab, purpose: EStoragePurpose.PURPOSE_DEPOSIT);
	}
	
	private ResourceName GetMagazineFromWeapon(ResourceName weaponPrefab)
	{
		EntitySpawnParams params = EntitySpawnParams();
		m_StorageManager.GetOwner().GetTransform(params.Transform);
		
		Resource weaponResource = Resource.Load(weaponPrefab);
		
		if(!weaponResource.IsValid())
			return ResourceName.Empty;
		
		IEntity weapon = GetGame().SpawnEntityPrefab(weaponResource, GetGame().GetWorld(), params);
		
		if(!weapon)
		{
			PrintFormat("TrainWreckLootingSystem: Was unable to spawn weapon %1", weaponPrefab, LogLevel.WARNING);
			return ResourceName.Empty;
		}
		
		WeaponComponent weaponComp = TW<WeaponComponent>.Find(weapon);
		BaseMagazineComponent comp = weaponComp.GetCurrentMagazine();
		
		if(!comp)
		{
			PrintFormat("TrainWreckLootingSystem: Weapon does not have weapon component %1", weaponPrefab, LogLevel.WARNING);
			return ResourceName.Empty;
		}
		
		if(!m_StorageManager.TryInsertItem(weapon, EStoragePurpose.PURPOSE_WEAPON_PROXY))
		{
			PrintFormat("TrainWreckLootingSystem: Was unable to insert weapon: %1", weaponPrefab, LogLevel.WARNING);
			return ResourceName.Empty;
		}
		
		if(m_Controller)
			m_Controller.TryEquipRightHandItem(weapon, EEquipItemType.EEquipTypeWeapon);
		
		return comp.GetOwner().GetPrefabData().GetPrefabName();
	}
	
	private void InitializeLoadout()
	{
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		if(!m_RplComponent.IsMaster())
			return;
		
		if(!m_StorageManager)
			return;
		
		if(!Initialized)
			InitializeWeightSystem();
		
		ref array<IEntity> entities = {};
		m_StorageManager.GetItems(entities);
		
		foreach(auto entity : entities)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		entities.Clear();
		delete entities;
		
		ResourceName pantsPrefab = m_PantsPrefabs.GetRandomPrefab();
		ResourceName headwearPrefab = m_HeadwearPrefabs.GetRandomPrefab();
		ResourceName footwearPrefab = m_FootwearPrefabs.GetRandomPrefab();
		ResourceName shirtPrefab = m_ShirtPrefabs.GetRandomPrefab();
		
		if(pantsPrefab) m_StorageManager.TrySpawnPrefabToStorage(pantsPrefab);
		if(headwearPrefab) m_StorageManager.TrySpawnPrefabToStorage(headwearPrefab);
		if(footwearPrefab) m_StorageManager.TrySpawnPrefabToStorage(footwearPrefab);
		if(shirtPrefab) m_StorageManager.TrySpawnPrefabToStorage(shirtPrefab);
		
		if(Math.RandomFloat01() < TW_LootManager.GetInstance().GetScavSettings().spawnWithBackpackChance)
		{
			ResourceName backpackPrefab = m_BackpackPrefabs.GetRandomPrefab();
			
			if(backpackPrefab != string.Empty)
			{
				m_StorageManager.TrySpawnPrefabToStorage(backpackPrefab, purpose: EStoragePurpose.PURPOSE_LOADOUT_PROXY);
			}
		}
		
		if(Math.RandomFloat01() < TW_LootManager.GetInstance().GetScavSettings().spawnWithVestChance)
		{
			ResourceName vestPrefab = m_VestPrefabs.GetRandomPrefab();
			if(vestPrefab != string.Empty)
				m_StorageManager.TrySpawnPrefabToStorage(vestPrefab, purpose: EStoragePurpose.PURPOSE_LOADOUT_PROXY);
		}				
		
		if(Math.RandomFloat01() < TW_LootManager.GetInstance().GetScavSettings().spawnWithHealChance)
		{
			int healCount = Math.RandomIntInclusive(0, 3);
			for(int i = 0; i < healCount; i++)
			{
				ResourceName healPrefab = m_HealPrefabs.GetRandomPrefab();
				if(healPrefab == string.Empty)
					continue;
				m_StorageManager.TrySpawnPrefabToStorage(healPrefab);
			}
		}
				
		ResourceName weaponPrefab = m_WeaponPrefabs.GetRandomPrefab();		
		ResourceName magazinePrefab = GetMagazineFromWeapon(weaponPrefab);
		
		if(magazinePrefab == ResourceName.Empty)
			return;
		
		int count = Math.RandomIntInclusive(0, 3);
		for(int i = 0; i < count; i++)
		{
			SpawnItem(magazinePrefab);
		}
		
		if(Math.RandomFloat01() < TW_LootManager.GetInstance().GetScavSettings().spawnWithTwoWeaponsChance)
		{
			weaponPrefab = m_WeaponPrefabs.GetRandomPrefab();		
			magazinePrefab = GetMagazineFromWeapon(weaponPrefab);
			
			if(magazinePrefab == ResourceName.Empty)
				return;
			
			count = Math.RandomIntInclusive(0, 3);
			for(int i = 0; i < count; i++)
			{
				SpawnItem(magazinePrefab);
			}	
		}
	}
};
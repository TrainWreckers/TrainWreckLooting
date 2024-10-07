class TW_RandomInventoryComponentClass : ScriptComponentClass {};
class TW_RandomInventoryComponent : ScriptComponent
{
	private BaseWeaponManagerComponent m_WeaponManager;
	private RplComponent m_RplComponent;
	private SCR_CharacterControllerComponent m_Controller;
	private SCR_InventoryStorageManagerComponent m_StorageManager;
	
	private static ref array<string> m_WeaponPrefabs = {};
	private static ref array<float> m_WeaponWeights = {};
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
	
	private void InitializeWeightSystem()
	{
		ref array<ref TW_LootConfigItem> weapons = {};
		int weaponCount = TW_LootManager.GetWeapons(weapons);
		
		if(weaponCount <= 0)
		{
			PrintFormat("TrainWreckLootingSystem: Was unable to locate weapons from loot table", LogLevel.ERROR);
			return;
		}
		
		Print("TrainWreckLootingSystem: Found %1 weapons");
		
		foreach(TW_LootConfigItem item : weapons)
		{
			m_WeaponPrefabs.Insert(item.resourceName);
			m_WeaponWeights.Insert(item.chanceToSpawn);
		}
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
		
		if(m_WeaponPrefabs.IsEmpty() && !Initialized)
			InitializeWeightSystem();
		
		if(m_WeaponPrefabs.IsEmpty())
			return;
		
		int index = SCR_ArrayHelper.GetWeightedIndex(m_WeaponWeights, Math.RandomFloat01());
		ResourceName weaponPrefab = m_WeaponPrefabs.Get(index);
		
		ResourceName magazinePrefab = GetMagazineFromWeapon(weaponPrefab);
		
		if(magazinePrefab == ResourceName.Empty)
			return;
		
		int count = Math.RandomIntInclusive(0, 3);
		for(int i = 0; i < count; i++)
		{
			SpawnItem(magazinePrefab);
		}
	}
};
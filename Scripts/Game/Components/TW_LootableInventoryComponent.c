class TW_LootableInventoryComponentClass : ScriptComponentClass {};

class TW_LootableInventoryComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "Item Pool Types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	private SCR_EArsenalItemType m_LootItemTypes;
	
	[Attribute("0", UIWidgets.Flags, "Item Mode Types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	private SCR_EArsenalItemMode m_LootItemModes;
	
	private InventoryStorageManagerComponent m_StorageManager;
	private BaseUniversalInventoryStorageComponent m_Storage;
	
	SCR_EArsenalItemType GetTypeFlags() { return m_LootItemTypes; }
	SCR_EArsenalItemMode GetModeFlags() { return m_LootItemModes; }
	InventoryStorageManagerComponent GetStorageManager() { return m_StorageManager; }
	BaseUniversalInventoryStorageComponent GetStorage() { return m_Storage; }
	
	protected bool m_HasBeenInteractedWith = false;
	protected int m_RespawnLootAfterTime = -1;	
	
	bool HasBeenInteractedWith() { return m_HasBeenInteractedWith; }
	
	//! Player has interacted with storage container AND respawn timer has elapsed
	bool CanRespawnLoot()
	{
		return m_HasBeenInteractedWith && s_GameMode.GetElapsedTime() >= m_RespawnLootAfterTime && m_RespawnLootAfterTime > 0;
	}
	
	static SCR_BaseGameMode s_GameMode;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode()) return;
		
		s_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		if(!rpl)
			return;
		
		if(!rpl.IsMaster())
			return;
		
		TW_LootManager.RegisterLootableContainer(this);
		
		m_StorageManager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		m_Storage = BaseUniversalInventoryStorageComponent.Cast(owner.FindComponent(BaseUniversalInventoryStorageComponent));		
	}
	
	event override protected void OnDelete(IEntity owner)
	{
		TW_LootManager.UnregisterLootableContainer(this);
	}
	
	bool InsertItem(TW_LootConfigItem item)
	{
		if(!item) return false;
		
		Resource prefabResource = Resource.Load(item.resourceName);
		
		if(!prefabResource.IsValid())
		{
			PrintFormat("TrainWreckLooting: Invalid Loot Resource: %1. Cannot spawn", item.resourceName, LogLevel.ERROR);
			return false;
		}
		
		EntitySpawnParams params = EntitySpawnParams();
		GetOwner().GetTransform(params.Transform);
		
		IEntity spawnedItem = GetGame().SpawnEntityPrefab(prefabResource, GetGame().GetWorld(), params);
		
		if(!spawnedItem)
		{
			PrintFormat("TrainWreckLooting: Was unable to spawn %1", item.resourceName, LogLevel.ERROR);
			return false;
		}
		
		BaseWeaponComponent weapon = BaseWeaponComponent.Cast(spawnedItem.FindComponent(BaseWeaponComponent));
		
		if(weapon)
		{
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			
			if(magazine)
			{
				if(TW_LootManager.ShouldSpawnMagazine)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(magazine.GetOwner());
				}
				else
				{
					int maxAmmo = magazine.GetMaxAmmoCount();
					int newCount = Math.RandomIntInclusive(0, maxAmmo);
					magazine.SetAmmoCount(newCount);
				}
			}
		}
		else
		{
			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(spawnedItem.FindComponent(BaseMagazineComponent));
			
			if(magazine)
			{
				int maxAmmo = magazine.GetMaxAmmoCount();
				float percent = TW_LootManager.GetRandomAmmoPercent();
				int ammo = Math.RandomIntInclusive(1, maxAmmo * percent);
				magazine.SetAmmoCount(Math.ClampInt(ammo, 0, maxAmmo));
			}
		}
		
		if(!m_StorageManager || !spawnedItem || !m_Storage)
			return false;
		
		bool success = m_StorageManager.TryInsertItemInStorage(spawnedItem, m_Storage);
		
		if(!success)
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
		
		return success;
	}
};
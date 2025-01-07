class TW_LootableInventoryComponentClass : ScriptComponentClass {};

class TW_LootableInventoryComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "Item Pool Types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	private SCR_EArsenalItemType m_LootItemTypes;
	
	[Attribute("0", UIWidgets.Flags, "Item Mode Types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	private SCR_EArsenalItemMode m_LootItemModes;
	
	private InventoryStorageManagerComponent m_StorageManager;
	private BaseUniversalInventoryStorageComponent m_Storage;
	private RplComponent m_Rpl;
	
	SCR_EArsenalItemType GetTypeFlags() { return m_LootItemTypes; }
	SCR_EArsenalItemMode GetModeFlags() { return m_LootItemModes; }
	InventoryStorageManagerComponent GetStorageManager() { return m_StorageManager; }
	BaseUniversalInventoryStorageComponent GetStorage() { return m_Storage; }
	
	[RplProp()]
	protected bool m_HasBeenInteractedWith = false;
	
	protected int m_RespawnLootAfterTime = -1;
	
	protected ref ScriptInvoker<bool> m_OnLootReset = new ScriptInvoker<bool>();
	
	ScriptInvoker<bool> GetOnLootReset() { return m_OnLootReset; }
	
	bool HasBeenInteractedWith() { return m_HasBeenInteractedWith; }
	
	//! Player has interacted with storage container AND respawn timer has elapsed
	bool CanRespawnLoot()
	{
		return m_HasBeenInteractedWith && GetGameMode().GetElapsedTime() >= m_RespawnLootAfterTime && m_RespawnLootAfterTime > 0;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	private void RPCAsk_Broadcast_InteractionUpdate(RplId containerId, bool value)
	{
		IEntity entity = TW_Global.GetEntityByRplId(containerId);
		
		if(!entity) return;
		
		TW_LootableInventoryComponent container = TW<TW_LootableInventoryComponent>.Find(entity);
		
		if(!container) return;
		
		container.SetInteractedWith(value);
	}
	
	void SetInteractedWith(bool value) 
	{ 	
		bool old = m_HasBeenInteractedWith;
		m_HasBeenInteractedWith = value;
		
		if(!m_Rpl.IsMaster() || !m_Rpl.IsOwner())
		{
			GetOnLootReset().Invoke(value);
			return;
		}
		
		if(value)
		{
			// We will not reregister the object if it's been interacted with already
			if(!old)
			{
				TW_LootManager.RegisterInteractedContainer(this);
				TW_LootManager.GetInstance().TrickleSpawnLootInContainer(this, TW_LootManager.GetInstance().GetRespawnLootItemThreshold());
			}
			
			// If we've interacted with we'll reset the timer
			float elapsed = GetGameMode().GetElapsedTime();
			m_RespawnLootAfterTime = elapsed + (TW_LootManager.GetInstance().GetRespawnAfterLastInteractionInMinutes() * 60);
			GetOnLootReset().Invoke(true);
			Rpc(RPCAsk_Broadcast_InteractionUpdate, m_Rpl.Id(), true);
		}
		else
		{
			/*
				RespawnLootProcessor will call this method with FALSE
				When resetting state to "Not interacted with" we need to
				delete all the items that were currently in the container
			
				That way when player returns to loot the container they have
				to do a "long press" to search, and items will trickle spawn in
			*/
			ref array<IEntity> items = {};
			GetStorageManager().GetItems(items);
			
			foreach(IEntity item : items)
				GetStorageManager().TryDeleteItem(item);
			
			GetOnLootReset().Invoke(false);
			Rpc(RPCAsk_Broadcast_InteractionUpdate, m_Rpl.Id(), false);
		}
	}
	
	static SCR_BaseGameMode s_GameMode;
	static SCR_BaseGameMode GetGameMode()
	{
		if(s_GameMode)
			return s_GameMode;
		
		s_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		return s_GameMode;
	}
		
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode()) return;
		
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		if(!m_Rpl)
			return;
		
		if(!m_Rpl.IsMaster())
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
				if(TW_LootManager.GetInstance().ShouldSpawnMagazine())
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
				float percent = TW_LootManager.GetInstance().GetRandomAmmoPercent();
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
modded class SCR_PlayerController
{
	void OnOpenLootableStorageContainer(TW_LootableInventoryComponent component)
	{		
		if(Replication.IsClient())
			Rpc(RplAsk_Server_OpenStorageContainer, TW_Global.GetEntityRplId(component.GetOwner()));
		else
			SetContainerInteraction(component);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void RplAsk_Server_OpenStorageContainer(RplId containerRplId)
	{
		IEntity containerEntity = TW_Global.GetEntityByRplId(containerRplId);
		
		if(!containerEntity)
		{
			PrintFormat("TrainWreckLooting: Unable to start spawning loot in container ID '%1' not found", containerRplId, LogLevel.ERROR);
			return;
		}
		
		TW_LootableInventoryComponent container = TW<TW_LootableInventoryComponent>.Find(containerEntity);
		
		if(!container)
		{
			PrintFormat("TrainWreckLooting: Unable to start spawning loot in container ID '%1' - Does not have lootable component", containerRplId, LogLevel.ERROR);
			return;
		}
		
		SetContainerInteraction(container);
	}
	
	private void SetContainerInteraction(TW_LootableInventoryComponent container)
	{
		TW_LootManager lootManager = TW_LootManager.GetInstance();
		
		if(lootManager)
		{
			lootManager.PrintSettings();
			
			if(lootManager.IsDebug())
			{
				Print("TrainWreck: Setting container interacted with to true");
			}
		}
		
		container.SetInteractedWith(true);
	}
};
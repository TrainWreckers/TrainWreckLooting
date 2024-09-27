modded class SCR_InventoryStorageManagerComponent
{
	override void SetStorageToOpen(IEntity storage)
	{
		super.SetStorageToOpen(storage);
		
		TW_LootableInventoryComponent loot = TW_LootableInventoryComponent.Cast(storage.FindComponent(TW_LootableInventoryComponent));
		
		if(loot)
			loot.SetInteractedWith(true);
	}
};
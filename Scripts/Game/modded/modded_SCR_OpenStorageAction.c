//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
modded class SCR_OpenStorageAction
{	
	protected TW_LootableInventoryComponent m_Container;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);				
		m_Container = TW_LootableInventoryComponent.Cast(pOwnerEntity.FindComponent(TW_LootableInventoryComponent));
		
		if(!m_Container)
			return;
		
		m_Container.GetOnLootReset().Insert(OnLootReset);
		SetActionDuration(TW_LootManager.GetNotSearchedActionDuration());
		
		if(GetUIInfo())	
			GetUIInfo().SetName("Search");
	}
	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		auto vicinity = CharacterVicinityComponent.Cast( pUserEntity .FindComponent( CharacterVicinityComponent ));
		if ( !vicinity )
			return;
		
		if(m_Container && !m_Container.HasBeenInteractedWith())
		{
			m_Container.SetInteractedWith(true);
			if(GetUIInfo())
				GetUIInfo().SetName("Open");
		}
			
		manager.SetStorageToOpen(pOwnerEntity);
		manager.OpenInventory();		
	}
	
	protected void OnLootReset(float timer)
	{
		SetActionDuration(timer);
		if(GetUIInfo())
			GetUIInfo().SetName("Search");
	}
};
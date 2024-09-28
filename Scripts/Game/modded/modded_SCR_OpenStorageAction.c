//------------------------------------------------------------------------------------------------
//! modded version for to be used with the inventory 2.0 
modded class SCR_OpenStorageAction
{	
	protected TW_LootableInventoryComponent m_Container;
	
	protected float m_SearchedTime, m_UnsearchedTime;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);				
		m_Container = TW_LootableInventoryComponent.Cast(pOwnerEntity.FindComponent(TW_LootableInventoryComponent));
		
		if(!m_Container)
			return;
		
		m_Container.GetOnLootReset().Insert(OnLootReset);
		
		vector size = SCR_EntityHelper.GetEntitySize(pOwnerEntity);
		float x = size[0];
		float y = size[1];
		float z = size[2];
		
		float unsearchRatio = TW_LootManager.GetUnlootedTimeRatio();
		float searchRatio = TW_LootManager.GetSearchedTimeRatio();
		
		m_UnsearchedTime= (x * unsearchRatio) + (y * unsearchRatio) + (z * unsearchRatio);
		m_SearchedTime = (x * searchRatio) + (y * searchRatio) + (z * searchRatio);
		
		SetActionDuration(m_UnsearchedTime);
		
		if(GetUIInfo())	
			GetUIInfo().SetName("Search");
	}
	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		auto vicinity = CharacterVicinityComponent.Cast( pUserEntity .FindComponent( CharacterVicinityComponent ));
		if ( !vicinity )
			return;
		
		if(m_Container)
			m_Container.SetInteractedWith(true);
		manager.SetStorageToOpen(pOwnerEntity);
		manager.OpenInventory();		
	}
	
	protected void OnLootReset(bool hasBeenSearched)
	{		
		string text = "Search";
		
		if(hasBeenSearched)
		{
			text = "Open";
			SetActionDuration(m_SearchedTime);
		}
		else 
			SetActionDuration(m_UnsearchedTime);
		
		if(GetUIInfo())		
			GetUIInfo().SetName(text);
	}
};
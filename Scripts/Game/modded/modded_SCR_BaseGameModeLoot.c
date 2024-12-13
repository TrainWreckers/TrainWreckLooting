modded class SCR_BaseGameMode
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!GetGame().InPlayMode())
			return;
		
		if(!m_RplComponent || !m_RplComponent.IsMaster())
			return;
		
		Event_OnGameInitializePlugins.Insert(InitializeLootManager);		
	}
	
	private void InitializeLootManager()
	{
		Print("TrainWreck: Initializing Loot Manager");
		TW_LootManager.Initialize();
	}
};
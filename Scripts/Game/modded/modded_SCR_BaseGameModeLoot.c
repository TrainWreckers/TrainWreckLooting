modded class SCR_BaseGameMode
{
	ref TW_LootManager m_LootManager;
	
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
		m_LootManager = new TW_LootManager();
		m_LootManager.Initialize();
	}
};
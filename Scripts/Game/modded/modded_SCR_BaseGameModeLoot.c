modded class SCR_BaseGameMode
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!GetGame().InPlayMode())
			return;
		
		if(!m_RplComponent || !m_RplComponent.IsMaster())
			return;
		
		// This shall automatically initialize all the things for us
		TW_LootManager.Initialize();
	}
};
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
	
	private void DelayBroadcast()
	{
		Rpc(Rpc_Broadcast_LootManagerSettings, TW_Util.ToJson(TW_LootManager.GetInstance().GetLootSettings(), true));
	}
	
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		if(!Replication.IsServer()) return;
		
		PrintFormat("TrainWreck: Player %1 joined. Broadcasting lootmap settings", playerId);
		GetGame().GetCallqueue().CallLater(DelayBroadcast, 1000, false);
	}
	
	private void InitializeLootManager()
	{
		Print("TrainWreck: Initializing Loot Manager");
		m_LootManager = new TW_LootManager();
		m_LootManager.Initialize();
	}
	
	void UpdateLootSettings(LootManagerSettings settings)
	{
		if(Replication.IsClient())
		{
			Print("TrainWreck: Unable to update loot settings - Invoked from client");
			return;
		}
		
		Print("TrainWreck: Updating Loot settings");
		
		TW_LootManager.GetInstance().InitializeLootTable(settings);
		Rpc(Rpc_Broadcast_LootManagerSettings, TW_Util.ToJson(TW_LootManager.GetInstance().GetLootSettings(), true));
	}
	
	void ResetLootSettings()
	{
		if(Replication.IsClient())
		{
			Print("TrainWreck: Unable to reset loot settings - Invoked from client");
			return;
		}
		
		Print("TrainWreck: Resetting loot settings");
		TW_LootManager.GetInstance().InitializeLootTable(resetSettings: true);
		Rpc(Rpc_Broadcast_LootManagerSettings, TW_Util.ToJson(TW_LootManager.GetInstance().GetLootSettings(), true));
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	private void Rpc_Broadcast_LootManagerSettings(string settings)
	{
		PrintFormat("TrainWreck: Broadcast Received for loot settings");
		ref LootManagerSettings lootSettings = LootManagerSettings.LoadFromFile(settings);
		ref TW_LootSettingsProxyInterface<LootManagerSettings> manager = LootSettingsManager.GetInstance().GetInterface();
		manager.Initialize(lootSettings);
	}
};
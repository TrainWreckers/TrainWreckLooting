class TW_LootSettings_Menu: MenuBase
{
	const string s_ItemChanceLayout = "{95C8F2B268AEF55E}UI/ItemChance.layout";
	
	private ref LootManagerSettings _settings;
	private Widget _contentArea;
	
	private SCR_InputButtonComponent saveButton;
	
	protected override void OnMenuOpen()
	{
		Widget rootWidget = GetRootWidget();
		
		if(Replication.IsServer())
		{
			_settings = LootManagerSettings.LoadFromFile();
		}
		else
		{
			ref TW_LootSettingsProxyInterface<LootManagerSettings> interface = TW_SettingsManager<ref TW_LootSettingsProxyInterface<LootManagerSettings>>.GetInstance().GetInterface();
			_settings = interface.GetLootSettings();
		}
		
		if(!rootWidget) return;
		_contentArea = rootWidget.FindAnyWidget("ContentArea");
		
		saveButton = SCR_InputButtonComponent.Cast(rootWidget.FindAnyWidget("SaveButton").FindHandler(SCR_InputButtonComponent));
		
		AddListeners();
		Initialize();
	}
	
	protected void Initialize()
	{		
		if(!_settings)
		{
			Print("TrainWreck: Unable to initalize loot settings menu -> Null reference to loot settings", LogLevel.WARNING);
			return;
		}
		
		if(!_settings.LootTable || _settings.LootTable.IsEmpty())
		{
			Print("TrainWreck: Unable to initialize loot settings menu -> No loot detected", LogLevel.WARNING);
			return;
		}
		
		foreach(string category, ref array<ref TW_LootConfigItem>> items : _settings.LootTable)
		{
			
			foreach(ref TW_LootConfigItem item : items)
			{
				Widget chanceWidget = GetGame().GetWorkspace().CreateWidgets(s_ItemChanceLayout, _contentArea);
				TW_LootItemChanceHandler handler = TW_LootItemChanceHandler.Cast(chanceWidget.FindHandler(TW_LootItemChanceHandler));
				handler.LinkTo(item);
			}
		}
	}
	
	private void Save()
	{
		if(Replication.IsServer())
		{
			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			gameMode.UpdateLootSettings(_settings);	
		}
		else
		{
			ref TW_LootSettingsProxyInterface<LootManagerSettings> interface = TW_SettingsManager<ref TW_LootSettingsProxyInterface<LootManagerSettings>>.GetInstance().GetInterface();
			interface.Initialize(_settings);
			interface.SaveSettings();
		}
		
		Close();
	}
	
	private void Reset()
	{
		Close();
	}
	
	protected override void OnMenuClose()
	{		
		RemoveListeners();
	}
	
	private void AddListeners()
	{
		saveButton.m_OnClicked.Insert(Save);
		
		InputManager manager = GetGame().GetInputManager();
		if(!manager) return;
		manager.ActivateContext("TrainWreckLootingSettings");
	}
	
	private void RemoveListeners()
	{
		saveButton.m_OnClicked.Remove(Save);
		InputManager manager = GetGame().GetInputManager();
		if(!manager) return;
		manager.ResetContext("TrainWreckLootingSettings");
	}
};
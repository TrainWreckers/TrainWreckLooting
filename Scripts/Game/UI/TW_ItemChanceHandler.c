class TW_LootItemChanceHandler : SCR_ScriptedWidgetComponent
{
	const string s_ChanceSlider = "ChanceSlider";
	const string s_AmountSlider = "AmountSlider";
	const string s_ItemName = "ItemName";
	const string s_ItemImage = "ItemPreview";
	const string s_EnabledCheckbox = "EnabledCheckbox";
	static protected ResourceName m_ItemPreviewManagerPrefab = "{9F18C476AB860F3B}Prefabs/World/Game/ItemPreviewManager.et";
	
	protected Widget m_Root;
	protected ref TW_LootConfigItem _loot;
	
	private static ChimeraWorld s_PreviewWorld;
	static ChimeraWorld GetPreviewWorld()
	{
		if(s_PreviewWorld)
			return s_PreviewWorld;
		
		s_PreviewWorld = GetGame().GetPlayerController().GetWorld();
		return s_PreviewWorld;
	}
	
	private static ItemPreviewManagerEntity s_PreviewManager;
	ItemPreviewManagerEntity GetPreviewManager()
	{
		if(s_PreviewManager)
			return s_PreviewManager;
		
		ChimeraWorld world = GetPreviewWorld();
		
		if(world)
		{
			s_PreviewManager = world.GetItemPreviewManager();
			
			if(!s_PreviewManager)
			{
				Resource resource = Resource.Load(m_ItemPreviewManagerPrefab);
				if(resource.IsValid())
				{
					GetGame().SpawnEntityPrefabLocal(resource, world);
				}
				
				s_PreviewManager = world.GetItemPreviewManager();
				return s_PreviewManager;
			}
			
			return s_PreviewManager;
		}
		
		return null;
	}
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_Root = w;
	}
	
	override void HandlerDeattached(Widget w)
	{
		SCR_SliderComponent amountSlider = GetSlider(s_AmountSlider);
		if(amountSlider && amountSlider.m_OnChanged)
			amountSlider.m_OnChanged.Remove(OnAmountChanged);
		
		SCR_SliderComponent chanceSlider = GetSlider(s_ChanceSlider);
		if(chanceSlider && chanceSlider.m_OnChanged)
			chanceSlider.m_OnChanged.Remove(OnChanceChanged);
		
		SCR_CheckboxComponent checkbox = GetEnabledCheckbox();
		if(checkbox && checkbox.m_OnChanged)
			checkbox.m_OnChanged.Remove(OnEnabledChanged);
	}
	
	void LinkTo(TW_LootConfigItem item)
	{
		if(!item)
		{
			return;
		}
		
		_loot = item;
		
		if(!Resource.Load(item.resourceName))
		{
			GetPreviewWidget().SetEnabled(false);
			GetNameWidget().SetText(string.Format("%1: Mod not loaded", item.resourceName));
			GetSlider(s_AmountSlider).SetEnabled(false);
			GetSlider(s_ChanceSlider).SetEnabled(false);
			return;
		}
		
		GetPreviewManager().SetPreviewItemFromPrefab(GetPreviewWidget(), item.resourceName);
		GetNameWidget().SetText(TW_Util.GetItemUIInfo(item.resourceName).GetName());
		
		SCR_CheckboxComponent checkbox = GetEnabledCheckbox();
		checkbox.SetChecked(item.isEnabled);
		checkbox.m_OnChanged.Insert(OnEnabledChanged);
		
		SCR_SliderComponent chanceSlider = GetSlider(s_ChanceSlider);
		chanceSlider.SetValue(_loot.chanceToSpawn);
		chanceSlider.SetMin(0);
		chanceSlider.SetMax(100);
		chanceSlider.SetStep(1);
		chanceSlider.SetFormatText("%1");
		chanceSlider.m_OnChanged.Insert(OnChanceChanged);
		
		SCR_SliderComponent amountSlider = GetSlider(s_AmountSlider);
		amountSlider.SetValue(_loot.randomSpawnCount);
		amountSlider.SetMin(0);
		amountSlider.SetMax(50);
		amountSlider.SetStep(1);
		amountSlider.SetFormatText("%1");
		amountSlider.m_OnChanged.Insert(OnAmountChanged);
	}
	
	private void OnEnabledChanged(SCR_CheckboxComponent comp, bool checked)
	{
		if(_loot)
			_loot.isEnabled = checked;
	}
	
	private void OnChanceChanged(SCR_SliderComponent comp, float value)
	{
		if(_loot)
			_loot.chanceToSpawn = value;
	}
	
	private void OnAmountChanged(SCR_SliderComponent comp, float value)
	{
		if(_loot)
			_loot.randomSpawnCount = value;
	}
	
	SCR_CheckboxComponent GetEnabledCheckbox()
	{
		Widget temp = m_Root.FindAnyWidget(s_EnabledCheckbox);
		if(!temp) return null;
		return SCR_CheckboxComponent.Cast(temp.FindHandler(SCR_CheckboxComponent));
	}
	
	ItemPreviewWidget GetPreviewWidget()
	{
		Widget temp = m_Root.FindAnyWidget("ItemPreview");
		if(!temp) return null;
		return ItemPreviewWidget.Cast(temp);
	}
	
	SCR_SliderComponent GetSlider(string name)
	{
		Widget temp = m_Root.FindAnyWidget(name);
		if(!temp) return null;
		return SCR_SliderComponent.Cast(temp.FindHandler(SCR_SliderComponent));
	}
	
	TextWidget GetNameWidget()
	{
		Widget temp = m_Root.FindAnyWidget(s_ItemName);
		if(!temp) return null;
		return TextWidget.Cast(temp);
	}
};
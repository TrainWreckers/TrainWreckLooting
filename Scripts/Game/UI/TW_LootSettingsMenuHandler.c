class TW_LootSettingsMenuHandler : SCR_ScriptedWidgetComponent
{
	const string s_ItemChanceLayout = "";
	const string s_CheckboxLayout = "";
	const string s_SliderLayout = "";
	
	protected Widget _root;
	protected SCR_FactionManager _factionManager;
	protected Widget _contentArea;
	
	private ref array<SCR_ChangeableComponentBase> components = {};
	
	override void HandlerAttached(Widget w)
	{
		_root = w;
		_contentArea = _root.FindAnyWidget("ContentArea");
		
	}
};
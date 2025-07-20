[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class TW_LootSettings_ToolbarAction : SCR_EditorToolbarAction
{
	override bool IsServer() { return false; }
	
	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		if(gameMode.GetState() != SCR_EGameModeState.GAME)
			return false;
		
		return true;
	}
	
	override bool CanBePerformed(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(gameMode.GetState() != SCR_EGameModeState.GAME)
			return false;
		return true;
	}
	
	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{			
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		int playerID = -1;
		
		if(playerController)
			playerID = playerController.GetPlayerId();
		
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.TW_LootSettingsMenuUI);
	}
}
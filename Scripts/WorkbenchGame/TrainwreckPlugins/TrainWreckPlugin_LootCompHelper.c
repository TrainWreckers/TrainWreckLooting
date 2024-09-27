[WorkbenchPluginAttribute(name: "TrainWreck Lootable Plugin", category: "TrainWreck Plugins", shortcut: "Ctrl+L", wbModules: {"WorldEditor"})]
class TrainWreckAddLootablePlugin : WorldEditorPlugin
{
	override void Run()
	{
		// Get World Editor module
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		
		// Get World Editor API
		WorldEditorAPI api = worldEditor.GetApi();
		
		int selectedEntitiesCount = api.GetSelectedEntitiesCount();
		
		int count = 0;
		if(selectedEntitiesCount == 0)
		{
			Print("Nothing selected to add components to", LogLevel.ERROR);
			return;
		}
		
		for(int i = 0; i < selectedEntitiesCount; i++)
		{
			IEntitySource selected = api.GetSelectedEntity(i);
			
			// Require rpl component - don't need to modify anything
			IEntityComponentSource rplCompSource = SCR_PrefabHelper.CreateEntitySourceComponentIfNeeded(selected, "RplComponent");
			
			IEntityComponentSource uniStorageSource = SCR_PrefabHelper.CreateEntitySourceComponentIfNeeded(selected, "SCR_UniversalInventoryStorageComponent");
			
			if(uniStorageSource)
			{				
				SCR_ItemAttributeCollection pAttrib = new SCR_ItemAttributeCollection();
				pAttrib.SetSlotType( ESlotID.SLOT_ANY );
				pAttrib.SetSlotSize( ESlotSize.SLOT_3x3 );
				uniStorageSource.Set("m_Attributes", pAttrib);
				uniStorageSource.Set("m_fMaxWeight", 10000);
			}
						
			SCR_PrefabHelper.CreateEntitySourceComponentIfNeeded(selected, "TW_LootableInventoryComponent");
			SCR_PrefabHelper.CreateEntitySourceComponentIfNeeded(selected, "SCR_InventoryStorageManagerComponent");
			IEntityComponentSource actionsManager = SCR_PrefabHelper.CreateEntitySourceComponentIfNeeded(selected, "ActionsManagerComponent");
			
			count++;			
		}
		
		Print(string.Format("TrainWreck: Added Components", count));
	}
}
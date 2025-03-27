[BaseContainerProps()]
class TW_LootItem
{
	private SCR_ArsenalItem m_ArsenalData;
	private SCR_EArsenalItemType m_ItemType;
	private SCR_EArsenalItemMode m_ItemMode;
	private ResourceName m_EntityPrefab;
	private string m_ItemName;
	
	void TW_LootItem(string name, SCR_EArsenalItemType itemType, SCR_EArsenalItemMode itemMode, ResourceName prefab)
	{
		m_ItemType = itemType;
		m_ItemMode = itemMode;
		m_ItemName = name;
	}
	
	ResourceName GetPrefab() { return m_EntityPrefab; }
	string GetItemName() { return m_ItemName; }
	SCR_EArsenalItemType GetItemType() { return m_ItemType; }
	SCR_EArsenalItemMode GetItemMode() { return m_ItemMode; }
};
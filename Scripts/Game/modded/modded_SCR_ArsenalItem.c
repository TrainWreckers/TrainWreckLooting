[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_ItemResourceName", true)]
modded class SCR_ArsenalItem
{
	[Attribute("25", UIWidgets.Slider, "Chance to Spawn", "0 100 1")]
	protected int m_ItemChanceToSpawn;
	
	[Attribute("1", UIWidgets.Slider, "Max # to Spawn", "1 50 1")]
	protected int m_ItemMaxSpawnCount;
	
	protected bool m_ShouldSpawn = true;
	
	protected ResourceName m_ItemPrefab;
	
	ResourceName GetItemPrefab() { return m_ItemPrefab; }
	void SetItemPrefab(ResourceName prefab) { m_ItemPrefab = prefab; }
	
	int GetItemChanceToSpawn() { return m_ItemChanceToSpawn; }
	void SetItemChanceToSpawn(int chance) { m_ItemChanceToSpawn = chance; }
	
	void SetItemMaxSpawnCount(int count) { m_ItemMaxSpawnCount = count; }
	int GetItemMaxSpawnCount() { return m_ItemMaxSpawnCount; }
	
	bool ShouldSpawn() { return m_ShouldSpawn; }
	void SetShouldSpawn(bool value) { m_ShouldSpawn = value; }
};
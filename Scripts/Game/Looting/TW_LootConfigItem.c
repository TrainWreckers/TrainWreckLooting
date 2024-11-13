/*
	This is intended to be used as a data structure for serializing and deserializing 
	TrainWreck loot tables
*/
class TW_LootConfigItem
{
	string resourceName;
	int chanceToSpawn;
	int randomSpawnCount;
	bool isEnabled;
	ref array<string> tags;
	
	void SetData(ResourceName resource, int chance, int spawnCount, array<string> itemTags = null, bool isEnabled = true)
	{
		this.resourceName = resource;
		this.chanceToSpawn = chance;
		this.randomSpawnCount = spawnCount;
		this.isEnabled = isEnabled;
		
		if(itemTags)
			this.tags = itemTags;
		else 
			this.tags = {};
	}
};
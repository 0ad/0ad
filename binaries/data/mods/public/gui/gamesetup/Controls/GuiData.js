/**
 * This class contains GUI-specific gamesetting data.
 */
class GameSettingsGuiData
{
	constructor()
	{
		this.mapFilter = new Observable();
		this.mapFilter.filter = "default";
	}

	/**
	 * Serialize for network transmission, settings persistence or convenience in other GUI files.
	 */
	Serialize()
	{
		let ret = {
			"mapFilter": this.mapFilter.filter,
		};
		return ret;
	}

	Deserialize(data)
	{
		this.mapFilter.filter = data.mapFilter;
	}
}

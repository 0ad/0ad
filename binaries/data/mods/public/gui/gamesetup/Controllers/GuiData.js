/**
 * This class contains network-synchronized data specific to GameSettingsController.
 * It's split from GameSettingsController for convenience.
 */
class GameSettingsGuiData
{
	constructor()
	{
		this.mapFilter = new Observable();
		this.mapFilter.filter = "default";
	}

	/**
	 * Serialize for network transmission & settings persistence.
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

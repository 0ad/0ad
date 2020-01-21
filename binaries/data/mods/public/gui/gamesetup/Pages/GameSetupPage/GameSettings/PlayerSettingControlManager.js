/**
 * Each property of this class is a class that inherits GameSettingControl and is
 * instantiated by the PlayerSettingControlManager.
 */
class PlayerSettingControls
{
}

/**
 * The purpose of the PlayerSettingControlManager is to own all controls that handle a property of g_GameAttributes.settings.PlayerData.
 */
class PlayerSettingControlManager
{
	constructor(...args)
	{
		this.playerSettingControls = {};

		for (let name in PlayerSettingControls)
			this.playerSettingControls[name] = new PlayerSettingControls[name](undefined, undefined, ...args);
	}

	addAutocompleteEntries(autocomplete)
	{
		for (let name in this.playerSettingControls)
			this.playerSettingControls[name].addAutocompleteEntries(name, autocomplete);
	}
}

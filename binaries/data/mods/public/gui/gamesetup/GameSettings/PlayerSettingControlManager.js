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
	constructor(playerIndex, gamesetupPage, gameSettingsControl, mapCache, mapFilters, netMessages, playerAssignmentsControl)
	{
		this.playerSettingControls = {};

		for (let name in PlayerSettingControls)
			this.playerSettingControls[name] =
				new PlayerSettingControls[name](
					undefined, undefined, playerIndex, gamesetupPage, gameSettingsControl, mapCache, mapFilters, netMessages, playerAssignmentsControl);
	}

	addAutocompleteEntries(autocomplete)
	{
		for (let name in this.playerSettingControls)
			this.playerSettingControls[name].addAutocompleteEntries(name, autocomplete);
	}
}

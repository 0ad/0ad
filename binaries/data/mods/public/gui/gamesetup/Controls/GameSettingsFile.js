/**
 * This class provides a way to save game settings to a file and load them.
 */
class GameSettingsFile
{
	constructor(GameSettingsControl)
	{
		this.filename = g_IsNetworked ?
			this.PersistedSettingsFileMultiplayer :
			this.PersistedSettingsFileSingleplayer;

		this.gameSettingsControl = GameSettingsControl;

		this.engineInfo = Engine.GetEngineInfo();
		this.enabled = Engine.ConfigDB_GetValue("user", this.ConfigName) == "true";
	}

	loadFile()
	{
		Engine.ProfileStart("loadPersistMatchSettingsFile");

		let data =
			this.enabled &&
			g_IsController &&
			Engine.FileExists(this.filename) &&
			Engine.ReadJSONFile(this.filename);

		let persistedSettings = data?.engine_info?.engine_version == this.engineInfo.engine_version &&
			hasSameMods(data?.engine_info?.mods, this.engineInfo.mods) &&
			data.attributes || {};

		Engine.ProfileStop();
		return persistedSettings;
	}

	/**
	 * Delete settings if disabled, so that players are not confronted with old settings after enabling the setting again.
	 */
	saveFile()
	{
		if (!g_IsController)
			return;

		Engine.ProfileStart("savePersistMatchSettingsFile");
		Engine.WriteJSONFile(this.filename, {
			"attributes": this.enabled ? this.gameSettingsControl.getSettings() : {},
			"engine_info": this.engineInfo
		});
		Engine.ProfileStop();
	}
}

GameSettingsFile.prototype.ConfigName =
	"persistmatchsettings";

GameSettingsFile.prototype.PersistedSettingsFileSingleplayer =
	"config/matchsettings.json";

GameSettingsFile.prototype.PersistedSettingsFileMultiplayer =
	"config/matchsettings.mp.json";

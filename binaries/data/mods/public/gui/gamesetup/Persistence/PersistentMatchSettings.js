/**
 * This class provides a way to save game settings to a file and load them.
 */
class PersistentMatchSettings
{
	constructor(isNetworked)
	{
		this.filename = isNetworked ?
			this.PersistedSettingsFileMultiplayer :
			this.PersistedSettingsFileSingleplayer;

		this.engineInfo = Engine.GetEngineInfo();
		this.enabled = Engine.ConfigDB_GetValue("user", this.ConfigName) == "true";
	}

	loadFile()
	{
		if (!this.enabled)
			return {};

		Engine.ProfileStart("loadPersistMatchSettingsFile");

		let data =
			Engine.FileExists(this.filename) &&
			Engine.ReadJSONFile(this.filename);

		let persistedSettings = data?.engine_info?.engine_version == this.engineInfo.engine_version &&
			hasSameMods(data?.engine_info?.mods, this.engineInfo.mods) &&
			data.attributes || {};

		Engine.ProfileStop();
		return persistedSettings;
	}

	/**
	 * Delete settings if disabled, so that players are not confronted
	 * with old settings after enabling the setting again.
	 */
	saveFile(settings)
	{
		Engine.ProfileStart("savePersistMatchSettingsFile");
		Engine.WriteJSONFile(this.filename, {
			"attributes": this.enabled ? settings : {},
			"engine_info": this.engineInfo
		});
		Engine.ProfileStop();
	}
}

PersistentMatchSettings.prototype.ConfigName =
	"persistmatchsettings";

PersistentMatchSettings.prototype.PersistedSettingsFileSingleplayer =
	"config/matchsettings.json";

PersistentMatchSettings.prototype.PersistedSettingsFileMultiplayer =
	"config/matchsettings.mp.json";

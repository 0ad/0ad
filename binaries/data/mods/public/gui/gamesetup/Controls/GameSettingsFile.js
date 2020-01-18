/**
 * This class provides a way to save g_GameAttributes to a file and load them.
 */
class GameSettingsFile
{
	constructor(setupWindow)
	{
		this.filename = g_IsNetworked ?
			this.GameAttributesFileMultiplayer :
			this.GameAttributesFileSingleplayer;

		this.engineInfo = Engine.GetEngineInfo();
		this.enabled = Engine.ConfigDB_GetValue("user", this.ConfigName) == "true";

		setupWindow.registerClosePageHandler(this.saveFile.bind(this));
	}

	loadFile()
	{
		Engine.ProfileStart("loadPersistMatchSettingsFile");

		let data =
			this.enabled &&
			g_IsController &&
			Engine.FileExists(this.filename) &&
			Engine.ReadJSONFile(this.filename);

		let gameAttributes =
			data &&
			data.attributes &&
			data.engine_info &&
			data.engine_info.engine_version == this.engineInfo.engine_version &&
			hasSameMods(data.engine_info.mods, this.engineInfo.mods) &&
			data.attributes || {};

		Engine.ProfileStop();
		return gameAttributes;
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
			"attributes": this.enabled ? g_GameAttributes : {},
			"engine_info": this.engineInfo
		});
		Engine.ProfileStop();
	}
}

GameSettingsFile.prototype.ConfigName =
	"persistmatchsettings";

GameSettingsFile.prototype.GameAttributesFileSingleplayer =
	"config/matchsettings.json";

GameSettingsFile.prototype.GameAttributesFileMultiplayer =
	"config/matchsettings.mp.json";

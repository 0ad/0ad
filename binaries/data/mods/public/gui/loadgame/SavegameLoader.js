/**
 * This class is responsible for display and performance of the Load button.
 */
class SavegameLoader
{
	constructor()
	{
		this.confirmButton = Engine.GetGUIObjectByName("confirmButton");
		this.confirmButton.caption = translate("Load");
		this.confirmButton.enabled = false;
	}

	onSelectionChange(gameID, metadata, label)
	{
		this.confirmButton.enabled = !!metadata;
		this.confirmButton.onPress = () => {
			this.loadGame(gameID, metadata);
		};
	}

	async loadGame(gameId, metadata)
	{
		// Check compatibility before really loading it
		let engineInfo = Engine.GetEngineInfo();
		let sameMods = hasSameMods(metadata.mods, engineInfo.mods);
		let sameEngineVersion = metadata.engine_version && metadata.engine_version == engineInfo.engine_version;

		if (sameEngineVersion && sameMods)
		{
			Engine.PopGuiPage(gameId);
			return;
		}

		// Version not compatible ... ask for confirmation
		let message = "";

		if (!sameEngineVersion)
			if (metadata.engine_version)
				message += sprintf(translate("This savegame needs 0 A.D. version %(requiredVersion)s, while you are running version %(currentVersion)s."), {
					"requiredVersion": metadata.engine_version,
					"currentVersion": engineInfo.engine_version
				}) + "\n";
			else
				message += translate("This savegame needs an older version of 0 A.D.") + "\n";

		if (!sameMods)
		{
			if (!metadata.mods)
				metadata.mods = [];

			message += translate("This savegame needs a different sequence of mods:") + "\n" +
				comparedModsString(metadata.mods, engineInfo.mods) + "\n";
		}

		message += translate("Do you still want to proceed?");

		const buttonIndex = await messageBox(
			500, 250,
			message,
			translate("Warning"),
			[translate("No"), translate("Yes")]);
		if (buttonIndex === 1)
			Engine.PopGuiPage(gameId);
	}
}

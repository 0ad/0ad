/**
 * Needed for formatPlayerInfo to show the player civs in the details.
 */
const g_CivData = loadCivData(false, false);

/**
 * This class is responsible for showing the map preview, description and other details
 * of the currently selected savegame, or showing a placeholder if no savegame is selected.
 */
class SavegameDetails
{
	constructor()
	{
		this.mapCache = new MapCache();

		this.onSelectionChange();
	}

	onSelectionChange(gameID, metadata, label)
	{
		Engine.GetGUIObjectByName("invalidGame").hidden = !!metadata;
		Engine.GetGUIObjectByName("validGame").hidden = !metadata;

		if (!metadata)
			return;

		Engine.GetGUIObjectByName("savedMapName").caption =
			this.mapCache.translateMapName(
				this.mapCache.getTranslatableMapName(metadata.initAttributes.mapType, metadata.initAttributes.map));

		Engine.GetGUIObjectByName("savedInfoPreview").sprite =
			this.mapCache.getMapPreview(metadata.initAttributes.mapType, metadata.initAttributes.map, metadata.initAttributes);

		Engine.GetGUIObjectByName("savedPlayers").caption = metadata.initAttributes.settings.PlayerData.length - 1;
		Engine.GetGUIObjectByName("savedPlayedTime").caption = timeToString(metadata.gui.timeElapsed ? metadata.gui.timeElapsed : 0);
		Engine.GetGUIObjectByName("savedMapType").caption = translateMapType(metadata.initAttributes.mapType);
		Engine.GetGUIObjectByName("savedMapSize").caption = translateMapSize(metadata.initAttributes.settings.Size || -1);
		Engine.GetGUIObjectByName("savedVictory").caption = metadata.initAttributes.settings.VictoryConditions.map(victoryConditionName => translateVictoryCondition(victoryConditionName)).join(translate(", "));

		let caption = sprintf(translate("Mods: %(mods)s"), { "mods": modsToString(metadata.mods) });
		if (!hasSameMods(metadata.mods, Engine.GetEngineInfo().mods))
			caption = coloredText(caption, "orange");
		Engine.GetGUIObjectByName("savedMods").caption = caption;

		Engine.GetGUIObjectByName("savedPlayersNames").caption = formatPlayerInfo(
			metadata.initAttributes.settings.PlayerData,
			metadata.gui.states);
	}
}

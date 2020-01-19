// TODO: There should be a dialog allowing to specify starting resources and population capacity per player
PlayerSettingControls.PlayerSettings = class extends GameSettingControl
{
	onMapChange(mapData)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		let isScenario = mapPData && g_GameAttributes.mapType == "scenario";

		if (isScenario && mapPData.Resources)
			pData.Resources = mapPData.Resources;
		else
			delete pData.Resources;

		if (isScenario && mapPData.PopulationLimit)
			pData.PopulationLimit = mapPData.PopulationLimit;
		else
			delete pData.PopulationLimit;
	}

	onGameAttributesFinalize()
	{
		// Copy map well known properties (and only well known properties)
		let mapData = this.mapCache.getMapData(g_GameAttributes.mapType, g_GameAttributes.map);

		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		if (!pData || !mapPData)
			return;

		for (let property of this.MapSettings)
			if (mapPData[property] !== undefined)
				pData[property] = mapPData[property];
	}
};

PlayerSettingControls.PlayerSettings.prototype.MapSettings = [
	"StartingTechnologies",
	"DisabledTechnologies",
	"DisabledTemplates",
	"StartingCamera"
];

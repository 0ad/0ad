/**
 * This class contains all controls modifying the AI settings of a player.
 */
class AIGameSettingControls
{
}

SetupWindowPages.AIConfigPage = class
{
	constructor(setupWindow)
	{
		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;

		this.playerIndex = undefined;
		this.row = 0;
		this.openPageHandlers = new Set();
		this.AIGameSettingControls = {};

		for (let name of this.AIGameSettingControlOrder)
			this.AIGameSettingControls[name] =
				new AIGameSettingControls[name](this, undefined, undefined, setupWindow);

		this.aiDescription = new AIDescription(this, setupWindow);

		this.aiConfigPage = Engine.GetGUIObjectByName("aiConfigPage");
		Engine.GetGUIObjectByName("aiConfigOkButton").onPress = this.closePage.bind(this);

		this.gameSettingsControl.registerGameAttributesBatchChangeHandler(
			this.onGameAttributesBatchChange.bind(this));
	}

	registerOpenPageHandler(handler)
	{
		this.openPageHandlers.add(handler);
	}

	getRow()
	{
		return this.row++;
	}

	openPage(playerIndex)
	{
		this.playerIndex = playerIndex;

		for (let handler of this.openPageHandlers)
			handler(playerIndex);

		this.aiConfigPage.hidden = false;
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			this.closePage();
	}

	closePage()
	{
		this.aiConfigPage.hidden = true;
	}
}

SetupWindowPages.AIConfigPage.prototype.AIGameSettingControlOrder = [
	"AISelection",
	"AIDifficulty",
	"AIBehavior"
];

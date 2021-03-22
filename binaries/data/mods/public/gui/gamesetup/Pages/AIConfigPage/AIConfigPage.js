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
		this.gameSettingsController = setupWindow.controls.gameSettingsController;

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

		g_GameSettings.playerAI.watch(() => this.maybeClose(), ["values"]);
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

	maybeClose()
	{
		if (!g_GameSettings.playerAI.get(this.playerIndex))
			this.closePage();
	}

	closePage()
	{
		this.aiConfigPage.hidden = true;
	}
};

SetupWindowPages.AIConfigPage.prototype.AIGameSettingControlOrder = [
	"AISelection",
	"AIDifficulty",
	"AIBehavior"
];

/**
 * The TipsPanel shows some hints to newcomers.
 * It is only shown in Singleplayer mode since the chat is shown instead in multiplayer mode.
 */
class TipsPanel
{
	constructor(gameSettingsPanel)
	{
		let available = !g_IsNetworked && Engine.ConfigDB_GetValue("user", this.Config) == "true";

		this.spTips = Engine.GetGUIObjectByName("spTips");
		this.spTips.hidden = !available;
		if (!available)
			return;

		this.displaySPTips = Engine.GetGUIObjectByName("displaySPTips");
		this.displaySPTips.onPress = this.onPress.bind(this);

		Engine.GetGUIObjectByName("aiTips").caption =
			Engine.TranslateLines(Engine.ReadFile(this.File));

		gameSettingsPanel.registerGameSettingsPanelResizeHandler(this.onGameSettingsPanelResize.bind(this));
	}

	onPress()
	{
		Engine.ConfigDB_CreateAndWriteValueToFile(
			"user",
			this.Config,
			String(this.displaySPTips.checked),
			"config/user.cfg");
	}

	onGameSettingsPanelResize(settingsPanel)
	{
		this.spTips.hidden =
			this.spTips.getComputedSize().right > settingsPanel.getComputedSize().left;
	}
}

TipsPanel.prototype.File =
	"gui/gamesetup/Pages/GameSetupPage/Panels/Tips.txt";

TipsPanel.prototype.Config =
	"gui.gamesetup.enabletips";

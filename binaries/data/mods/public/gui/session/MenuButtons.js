/**
 * This class is extended in subclasses.
 * Each subclass represents one button in the session menu.
 * All subclasses  store the button member so that mods can change it easily.
 */
class MenuButtons
{
}

MenuButtons.prototype.Manual = class
{
	constructor(button, pauseControl)
	{
		this.button = button;
		this.button.caption = translate(translate("Manual"));
		this.pauseControl = pauseControl;
	}

	onPress()
	{
		closeOpenDialogs();
		this.pauseControl.implicitPause();
		Engine.PushGuiPage("page_manual.xml", {}, resumeGame);
	}
};

MenuButtons.prototype.Chat = class
{
	constructor(button, pauseControl, playerViewControl, chat)
	{
		this.button = button;
		this.button.caption = translate("Chat");
		this.chat = chat;
		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.button.tooltip = this.chat.getOpenHotkeyTooltip().trim();
	}

	onPress()
	{
		this.chat.openPage();
	}
};

MenuButtons.prototype.Save = class
{
	constructor(button, pauseControl)
	{
		this.button = button;
		this.button.caption = translate("Save");
		this.pauseControl = pauseControl;
	}

	onPress()
	{
		closeOpenDialogs();
		this.pauseControl.implicitPause();

		Engine.PushGuiPage(
			"page_loadgame.xml",
			{ "savedGameData": getSavedGameData() },
			resumeGame);
	}
};

MenuButtons.prototype.Summary = class
{
	constructor(button, pauseControl)
	{
		this.button = button;
		this.button.caption = translate("Summary");
		this.button.hotkey = "summary";
		// TODO: Atlas should pass g_GameAttributes.settings
		this.button.enabled = !Engine.IsAtlasRunning();

		this.pauseControl = pauseControl;
		this.selectedData = undefined;

		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.button.tooltip = sprintf(translate("Press %(hotkey)s to open the summary screen."), {
			"hotkey": colorizeHotkey("%(hotkey)s", this.button.hotkey),
		});
	}

	onPress()
	{
		if (Engine.IsAtlasRunning())
			return;

		closeOpenDialogs();
		this.pauseControl.implicitPause();
		 // Allows players to see their own summary.
		// If they have shared ally vision researched, they are able to see the summary of there allies too.
		let simState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
		Engine.PushGuiPage(
			"page_summary.xml",
			{
				"sim": {
					"mapSettings": g_GameAttributes.settings,
					"playerStates": simState.players.filter((state, player) =>
						g_IsObserver || player == 0 || player == g_ViewedPlayer ||
						simState.players[g_ViewedPlayer].hasSharedLos && g_Players[player].isMutualAlly[g_ViewedPlayer]),
					"timeElapsed": simState.timeElapsed
				},
				"gui": {
					"dialog": true,
					"isInGame": true
				},
				"selectedData": this.selectedData
			},
			data =>
			{
				this.selectedData = data.summarySelectedData;
				this.pauseControl.implicitResume();
			});
	}
};

MenuButtons.prototype.Lobby = class
{
	constructor(button)
	{
		this.button = button;
		this.button.caption = translate("Lobby");
		this.button.hotkey = "lobby";
		this.button.enabled = Engine.HasXmppClient();

		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.button.tooltip = sprintf(translate("Press %(hotkey)s to open the multiplayer lobby page without leaving the game."), {
			"hotkey": colorizeHotkey("%(hotkey)s", this.button.hotkey),
		});
	}

	onPress()
	{
		if (!Engine.HasXmppClient())
			return;
		closeOpenDialogs();
		Engine.PushGuiPage("page_lobby.xml", { "dialog": true });
	}
};

MenuButtons.prototype.Options = class
{
	constructor(button, pauseControl)
	{
		this.button = button;
		this.button.caption = translate("Options");
		this.pauseControl = pauseControl;
	}

	onPress()
	{
		closeOpenDialogs();
		this.pauseControl.implicitPause();

		Engine.PushGuiPage(
			"page_options.xml",
			{},
			callbackFunctionNames => {
				for (let functionName of callbackFunctionNames)
					if (global[functionName])
						global[functionName]();

				resumeGame();
			});
	}
};

MenuButtons.prototype.Pause = class
{
	constructor(button, pauseControl, playerViewControl)
	{
		this.button = button;
		this.button.hotkey = "pause";
		this.pauseControl = pauseControl;

		registerPlayersInitHandler(this.rebuild.bind(this));
		registerPlayersFinishedHandler(this.rebuild.bind(this));
		playerViewControl.registerPlayerIDChangeHandler(this.rebuild.bind(this));
		pauseControl.registerPauseHandler(this.rebuild.bind(this));
		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.button.enabled = !g_IsObserver || !g_IsNetworked || g_IsController;
		this.button.caption = this.pauseControl.explicitPause ? translate("Resume") : translate("Pause");
		this.button.tooltip = sprintf(translate("Press %(hotkey)s to pause or resume the game."), {
			"hotkey": colorizeHotkey("%(hotkey)s", this.button.hotkey),
		});
	}

	onPress()
	{
		this.pauseControl.setPaused(!g_PauseControl.explicitPause, true);
	}
};

MenuButtons.prototype.Resign = class
{
	constructor(button, pauseControl, playerViewControl)
	{
		this.button = button;
		this.button.caption = translate("Resign");
		this.pauseControl = pauseControl;

		registerPlayersInitHandler(this.rebuild.bind(this));
		registerPlayersFinishedHandler(this.rebuild.bind(this));
		playerViewControl.registerPlayerIDChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.button.enabled = !g_IsObserver;
	}

	onPress()
	{
		closeOpenDialogs();
		this.pauseControl.implicitPause();

		messageBox(
			400, 200,
			translate("Are you sure you want to resign?"),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[
				resumeGame,
				() => {
					Engine.PostNetworkCommand({
						"type": "resign"
					});
					resumeGame();
				}
			]);
	}
};

MenuButtons.prototype.Exit = class
{
	constructor(button, pauseControl)
	{
		this.button = button;
		this.button.caption = translate("Exit");
		this.button.enabled = !Engine.IsAtlasRunning();
		this.pauseControl = pauseControl;
	}

	onPress()
	{
		closeOpenDialogs();
		this.pauseControl.implicitPause();

		let messageType = g_IsNetworked && g_IsController ? "host" :
			(g_IsNetworked && !g_IsObserver ? "client" : "singleplayer");

		messageBox(
			400, 200,
			this.Confirmation[messageType].caption(),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			this.Confirmation[messageType].buttons());
	}
};

MenuButtons.prototype.Exit.prototype.Confirmation = {
	"host": {
		"caption": () => translate("Are you sure you want to quit? Leaving will disconnect all other players."),
		"buttons": () => [resumeGame, endGame]
	},
	"client": {
		"caption": () => translate("Are you sure you want to quit?"),
		"buttons": () => [
			resumeGame,
			() => {
				messageBox(
					400, 200,
					translate("Do you want to resign or will you return soon?"),
					translate("Confirmation"),
					[translate("I will return"), translate("I resign")],
					[
						endGame,
						() => {
							Engine.PostNetworkCommand({
								"type": "resign"
							});
							resumeGame();
						}
					]);
			}
		]
	},
	"singleplayer": {
		"caption": () => translate("Are you sure you want to quit?"),
		"buttons": () => [resumeGame, endGame]
	}
};

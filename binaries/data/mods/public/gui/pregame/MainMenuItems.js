var g_MainMenuItems = [
	{
		"caption": translate("Learn to Play"),
		"tooltip": translate("Learn how to play, start the tutorial, discover the technology trees, and the history behind the civilizations."),
		"submenu": [
			{
				"caption": translate("Manual"),
				"tooltip": translate("Open the 0 A.D. Game Manual."),
				"onPress": () => {
					Engine.PushGuiPage("page_manual.xml");
				}
			},
			{
				"caption": translate("Tutorial"),
				"tooltip": translate("Start the introductory tutorial."),
				"onPress": () => {
					Engine.SwitchGuiPage("page_autostart.xml", {
						"attribs": {
							"mapType": "scenario",
							"map": "maps/tutorials/introductory_tutorial",
							"settings": {
								"CheatsEnabled": true
							},
						},
						"playerAssignments": {
							"local": {
								"player": 1,
								"name": Engine.ConfigDB_GetValue("user", "playername.singleplayer") || Engine.GetSystemUsername()
							}
						},
						"storeReplay": true
					});
				}
			},
			{
				"caption": translate("Structure Tree"),
				"tooltip": colorizeHotkey(translate("%(hotkey)s: View the structure tree of civilizations featured in 0 A.D."), "structree"),
				"hotkey": "structree",
				"onPress": pageLoop.bind(null, "page_structree.xml")
			},
			{
				"caption": translate("Civilization Overview"),
				"tooltip": colorizeHotkey(translate("%(hotkey)s: Learn about the civilizations featured in 0 A.D."), "civinfo"),
				"hotkey": "civinfo",
				"onPress": pageLoop.bind(null, "page_civinfo.xml")
			},
			{
				"caption": translate("Catafalque Overview"),
				"tooltip": translate("Compare the bonuses of catafalques featured in 0 A.D."),
				"onPress": () => {
					Engine.PushGuiPage("page_catafalque.xml");
				}
			},
			{
				"caption": translate("Map Overview"),
				"tooltip": translate("View the different maps featured in 0 A.D."),
				"onPress": () => {
					Engine.PushGuiPage("page_mapbrowser.xml");
				},
			}
		]
	},
	{
		"caption": translate("Continue Campaign"),
		"tooltip": translate("Relive history through historical military campaigns."),
		"onPress": () => {
			try
			{
				Engine.SwitchGuiPage(CampaignRun.getCurrentRun().getMenuPath());
			}
			catch(err)
			{
				error("Error opening campaign run:");
				error(err.toString());
			}
		},
		"enabled": () => CampaignRun.hasCurrentRun()
	},
	{
		"caption": translate("Single-player"),
		"tooltip": translate("Start, load, or replay a single-player game."),
		"submenu": [
			{
				"caption": translate("Matches"),
				"tooltip": translate("Start a new single-player game."),
				"onPress": () => {
					Engine.SwitchGuiPage("page_gamesetup.xml");
				}
			},
			{
				"caption": translate("Load Game"),
				"tooltip": translate("Load a saved game."),
				"onPress": async() => {
					const gameId = await Engine.PushGuiPage("page_loadgame.xml");

					if (!gameId)
						return;

					const metadata = Engine.StartSavedGame(gameId);
					if (!metadata)
					{
						error("Could not load saved game: " + gameId);
						return;
					}

					Engine.SwitchGuiPage("page_loading.xml", {
						"attribs": metadata.initAttributes,
						"playerAssignments": {
							"local": {
								"name": metadata.initAttributes.settings.
									PlayerData[metadata.playerID]?.Name ??
									singleplayerName(),
								"player": metadata.playerID
							}
						},
						"savedGUIData": metadata.gui
					});
				}
			},
			{
				"caption": translate("Continue Campaign"),
				"tooltip": translate("Relive history through historical military campaigns."),
				"onPress": () => {
					try
					{
						Engine.SwitchGuiPage(CampaignRun.getCurrentRun().getMenuPath());
					}
					catch(err)
					{
						error("Error opening campaign run:");
						error(err.toString());
					}
				},
				"enabled": () => CampaignRun.hasCurrentRun()
			},
			{
				"caption": translate("New Campaign"),
				"tooltip": translate("Relive history through historical military campaigns."),
				"onPress": () => {
					Engine.SwitchGuiPage("campaigns/setup/page.xml");
				}
			},
			{
				"caption": translate("Load Campaign"),
				"tooltip": translate("Relive history through historical military campaigns."),
				"onPress": () => {
					// Switch instead of push, otherwise the 'continue'
					// button might remain enabled.
					// TODO: find a better solution.
					Engine.SwitchGuiPage("campaigns/load_modal/page.xml");
				}
			},
			{
				"caption": translate("Replays"),
				"tooltip": translate("Playback previous games."),
				"onPress": () => {
					Engine.SwitchGuiPage("page_replaymenu.xml", {
						"replaySelectionData": {
							"filters": {
								"singleplayer": "Single-player"
							}
						}
					});
				}
			}
		]
	},
	{
		"caption": translate("Multiplayer"),
		"tooltip": translate("Fight against one or more human players in a multiplayer game."),
		"submenu": [
			{
				// Translation: Join a game by specifying the host's IP address.
				"caption": translate("Join Game"),
				"tooltip": translate("Joining an existing multiplayer game."),
				"onPress": () => {
					Engine.PushGuiPage("page_gamesetup_mp.xml", {
						"multiplayerGameType": "join"
					});
				}
			},
			{
				"caption": translate("Host Game"),
				"tooltip": translate("Host a multiplayer game."),
				"onPress": () => {
					Engine.PushGuiPage("page_gamesetup_mp.xml", {
						"multiplayerGameType": "host"
					});
				}
			},
			{
				"caption": translate("Game Lobby"),
				"tooltip":
					colorizeHotkey(translate("%(hotkey)s: Launch the multiplayer lobby to join and host publicly visible games and chat with other players."), "lobby") +
					(Engine.StartXmppClient ? "" : translate("Launch the multiplayer lobby. \\[DISABLED BY BUILD]")),
				"enabled": () => !!Engine.StartXmppClient,
				"hotkey": "lobby",
				"onPress": () => {
					 if (Engine.StartXmppClient)
						 Engine.PushGuiPage("page_prelobby_entrance.xml");
				}
			},
			{
				"caption": translate("Replays"),
				"tooltip": translate("Playback previous games."),
				"onPress": () => {
					Engine.SwitchGuiPage("page_replaymenu.xml", {
						"replaySelectionData": {
							"filters": {
								"singleplayer": "Multiplayer"
							}
						}
					});
				}
			}
		]
	},
	{
		"caption": translate("Settings"),
		"tooltip": translate("Change game options."),
		"submenu": [
			{
				"caption": translate("Options"),
				"tooltip": translate("Adjust game settings."),
				"onPress": async() => {
					fireConfigChangeHandlers(await Engine.PushGuiPage("page_options.xml"));
				}
			},
			{
				"caption": translate("Hotkeys"),
				"tooltip": translate("Adjust hotkeys."),
				"onPress": () => {
					Engine.PushGuiPage("hotkeys/page_hotkeys.xml");
				}
			},
			{
				"caption": translate("Language"),
				"tooltip": translate("Choose the language of the game."),
				"onPress": () => {
					Engine.PushGuiPage("page_locale.xml");
				}
			},
			{
				"caption": translate("Mod Selection"),
				"tooltip": translate("Select and download mods for the game."),
				"onPress": () => {
					Engine.SwitchGuiPage("page_modmod.xml");
				}
			},
			{
				"caption": translate("Welcome Screen"),
				"tooltip": translate("Show the Welcome Screen again. Useful if you hid it by mistake."),
				"onPress": () => {
					Engine.PushGuiPage("page_splashscreen.xml");
				}
			}
		]
	},
	{
		"caption": translate("Scenario Editor"),
		"tooltip": translate('Open the Atlas Scenario Editor in a new window. You can run this more reliably by starting the game with the command-line argument "-editor".'),
		"onPress": () => {
			if (Engine.AtlasIsAvailable())
				messageBox(
					400, 200,
					translate("Are you sure you want to quit 0 A.D. and open the Scenario Editor?"),
					translate("Confirmation"),
					[translate("No"), translate("Yes")],
					[null, Engine.RestartInAtlas]);
			else
				messageBox(
					400, 200,
					translate("The scenario editor is not available or failed to load. See the game logs for additional information."),
					translate("Error"));
		}
	},
	{
		"caption": translate("Credits"),
		"tooltip": translate("Show the 0 A.D. credits."),
		"onPress": () => {
			Engine.PushGuiPage("page_credits.xml");
		}
	},
	{
		"caption": translate("Exit"),
		"tooltip": translate("Exit the game."),
		"onPress": () => {
			messageBox(
				400, 200,
				translate("Are you sure you want to quit 0 A.D.?"),
				translate("Confirmation"),
				[translate("No"), translate("Yes")],
				[null, Engine.Exit]);
		}
	}
];

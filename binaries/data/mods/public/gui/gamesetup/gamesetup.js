const g_MatchSettings_SP = "config/matchsettings.json";
const g_MatchSettings_MP = "config/matchsettings.mp.json";

const g_Ceasefire = prepareForDropdown(g_Settings && g_Settings.Ceasefire);
const g_GameSpeeds = prepareForDropdown(g_Settings && g_Settings.GameSpeeds.filter(speed => !speed.ReplayOnly));
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);
const g_MapTypes = prepareForDropdown(g_Settings && g_Settings.MapTypes);
const g_PopulationCapacities = prepareForDropdown(g_Settings && g_Settings.PopulationCapacities);
const g_StartingResources = prepareForDropdown(g_Settings && g_Settings.StartingResources);
const g_VictoryConditions = prepareForDropdown(g_Settings && g_Settings.VictoryConditions);
const g_VictoryDurations = prepareForDropdown(g_Settings && g_Settings.VictoryDurations);

/**
 * Highlight the "random" dropdownlist item.
 */
var g_ColorRandom = "orange";

/**
 * Color for regular dropdownlist items.
 */
var g_ColorRegular = "white";

/**
 * Color for "Unassigned"-placeholder item in the dropdownlist.
 */
var g_PlayerAssignmentColors = {
	"player": g_ColorRegular,
	"observer": "170 170 250",
	"unassigned": "140 140 140",
	"AI": "70 150 70"
};

/**
 * Used for highlighting the sender of chat messages.
 */
var g_SenderFont = "sans-bold-13";

/**
 * This yields [1, 2, ..., MaxPlayers].
 */
var g_NumPlayersList = Array(g_MaxPlayers).fill(0).map((v, i) => i + 1);

/**
 * Used for generating the botnames.
 */
var g_RomanNumbers = [undefined, "I", "II", "III", "IV", "V", "VI", "VII", "VIII"];

var g_PlayerTeamList = prepareForDropdown([{
		"label": translateWithContext("team", "None"),
		"id": -1
	}].concat(
		Array(g_MaxTeams).fill(0).map((v, i) => ({
			"label": i + 1,
			"id": i
		}))
	)
);

/**
 * Offer users to select playable civs only.
 * Load unselectable civs as they could appear in scenario maps.
 */
var g_CivData = loadCivData();

/**
 * Number of relics: [1, ..., NumCivs]
 */
var g_RelicCountList = Object.keys(g_CivData).map((civ, i) => i + 1);

var g_PlayerCivList = g_CivData && prepareForDropdown([{
		"name": translateWithContext("civilization", "Random"),
		"tooltip": translate("Picks one civilization at random when the game starts."),
		"color": g_ColorRandom,
		"code": "random"
	}].concat(
		Object.keys(g_CivData).filter(
			civ => g_CivData[civ].SelectableInGameSetup
		).map(civ => ({
			"name": g_CivData[civ].Name,
			"tooltip": g_CivData[civ].History,
			"color": g_ColorRegular,
			"code": civ
		})).sort(sortNameIgnoreCase)
	)
);

/**
 * All selectable playercolors except gaia.
 */
var g_PlayerColorPickerList = g_Settings && g_Settings.PlayerDefaults.slice(1).map(pData => pData.Color);

/**
 * Directory containing all maps of the given type.
 */
var g_MapPath = {
	"random": "maps/random/",
	"scenario": "maps/scenarios/",
	"skirmish": "maps/skirmishes/"
};

/**
 * Containing the colors to highlight the ready status of players,
 * the chat ready messages and
 * the tooltips and captions for the ready button
 */

var g_ReadyData = [
	{
		"color": g_ColorRegular,
		"chat": translate("* %(username)s is not ready."),
		"caption": translate("I'm ready"),
		"tooltip": translate("State that you are ready to play.")
	},
	{
		"color": "green",
		"chat": translate("* %(username)s is ready!"),
		"caption": translate("Stay ready"),
		"tooltip": translate("Stay ready even when the game settings change.")
	},
	{
		"color": "150 150 250",
		"chat": "",
		"caption": translate("I'm not ready!"),
		"tooltip": translate("State that you are not ready to play.")
	}
];

/**
 * Processes a CNetMessage (see NetMessage.h, NetMessages.h) sent by the CNetServer.
 */
var g_NetMessageTypes = {
	"netstatus": msg => handleNetStatusMessage(msg),
	"netwarn": msg => addNetworkWarning(msg),
	"gamesetup": msg => handleGamesetupMessage(msg),
	"players": msg => handlePlayerAssignmentMessage(msg),
	"ready": msg => handleReadyMessage(msg),
	"start": msg => handleGamestartMessage(msg),
	"kicked": msg => addChatMessage({
		"type": msg.banned ? "banned" : "kicked",
		"username": msg.username
	}),
	"chat": msg => addChatMessage({ "type": "chat", "guid": msg.guid, "text": msg.text }),
};

var g_FormatChatMessage = {
	"system": (msg, user) => systemMessage(msg.text),
	"settings": (msg, user) => systemMessage(translate('Game settings have been changed')),
	"connect": (msg, user) => systemMessage(sprintf(translate("%(username)s has joined"), { "username": user })),
	"disconnect": (msg, user) => systemMessage(sprintf(translate("%(username)s has left"), { "username": user })),
	"kicked": (msg, user) => systemMessage(sprintf(translate("%(username)s has been kicked"), { "username": user })),
	"banned": (msg, user) => systemMessage(sprintf(translate("%(username)s has been banned"), { "username": user })),
	"chat": (msg, user) => sprintf(translate("%(username)s %(message)s"), {
		"username": senderFont(sprintf(translate("<%(username)s>"), { "username": user })),
		"message": escapeText(msg.text || "")
	}),
	"ready": (msg, user) => sprintf(g_ReadyData[msg.status].chat, { "username": user }),
	"clientlist": (msg, user) => getUsernameList(),
};

var g_MapFilterList = prepareForDropdown([
	{
		"id": "default",
		"name": translateWithContext("map filter", "Default"),
		"tooltip": translateWithContext("map filter", "All maps except naval and demo maps."),
		"filter": mapKeywords => mapKeywords.every(keyword => ["naval", "demo", "hidden"].indexOf(keyword) == -1),
		"Default": true
	},
	{
		"id": "naval",
		"name": translate("Naval Maps"),
		"tooltip": translateWithContext("map filter", "Maps where ships are needed to reach the enemy."),
		"filter": mapKeywords => mapKeywords.indexOf("naval") != -1
	},
	{
		"id": "demo",
		"name": translate("Demo Maps"),
		"tooltip": translateWithContext("map filter", "These maps are not playable but for demonstration purposes only."),
		"filter": mapKeywords => mapKeywords.indexOf("demo") != -1
	},
	{
		"id": "new",
		"name": translate("New Maps"),
		"tooltip": translateWithContext("map filter", "Maps that are brand new in this release of the game."),
		"filter": mapKeywords => mapKeywords.indexOf("new") != -1
	},
	{
		"id": "trigger",
		"name": translate("Trigger Maps"),
		"tooltip": translateWithContext("map filter", "Maps that come with scripted events and potentially spawn enemy units."),
		"filter": mapKeywords => mapKeywords.indexOf("trigger") != -1
	},
	{
		"id": "all",
		"name": translate("All Maps"),
		"tooltip": translateWithContext("map filter", "Every map of the chosen maptype."),
		"filter": mapKeywords => true
	},
]);

/**
 * Whether this is a single- or multiplayer match.
 */
var g_IsNetworked;

/**
 * Is this user in control of game settings (i.e. singleplayer or host of a multiplayergame).
 */
var g_IsController;

/**
 * Whether this is a tutorial.
 */
var g_IsTutorial;

/**
 * To report the game to the lobby bot.
 */
var g_ServerName;
var g_ServerPort;

/**
 * IP address and port of the STUN endpoint.
 */
var g_StunEndpoint;

/**
 * Current username. Cannot contain whitespace.
 */
var g_Username = Engine.LobbyGetNick();

/**
 * States whether the GUI is currently updated in response to network messages instead of user input
 * and therefore shouldn't send further messages to the network.
 */
var g_IsInGuiUpdate = false;

/**
 * Whether the current player is ready to start the game.
 * 0 - not ready
 * 1 - ready
 * 2 - stay ready
 */
var g_IsReady = 0;

/**
 * Ignore duplicate ready commands on init.
 */
var g_ReadyInit = true;

/**
 * If noone has changed the ready status, we have no need to spam the settings changed message.
 *
 * <=0 - Suppressed settings message
 * 1 - Will show settings message
 * 2 - Host's initial ready, suppressed settings message
 */
var g_ReadyChanged = 2;

/**
 * Used to prevent calling resetReadyData when starting a game.
 */
var g_GameStarted = false;

/**
 * Selectable options (player, AI, unassigned) in the player assignment dropdowns and
 * their colorized, textual representation.
 */
var g_PlayerAssignmentList = {};

/**
 * Remembers which clients are assigned to which player slots and whether they are ready.
 * The keys are guids or "local" in Singleplayer.
 */
var g_PlayerAssignments = {};

var g_DefaultPlayerData = [];

var g_GameAttributes = { "settings": {} };

/**
 * List of translated words that can be used to autocomplete titles of settings
 * and their values (for example playernames).
 */
var g_Autocomplete = [];

/**
 * Array of strings formatted as displayed, including playername.
 */
var g_ChatMessages = [];

/**
 * Filename and translated title of all maps, given the currently selected
 * maptype and filter. Sorted by title, shown in the dropdown.
 */
var g_MapSelectionList = [];

/**
 * Cache containing the mapsettings. Just-in-time loading.
 */
var g_MapData = {};

/**
 * Wait one tick before initializing the GUI objects and
 * don't process netmessages prior to that.
 */
var g_LoadingState = 0;

/**
 * Send the current gamesettings to the lobby bot if the settings didn't change for this number of seconds.
 */
var g_GameStanzaTimeout = 2;

/**
 * Index of the GUI timer.
 */
var g_GameStanzaTimer = undefined;

/**
 * Only send a lobby update if something actually changed.
 */
var g_LastGameStanza;

/**
 * Remembers if the current player viewed the AI settings of some playerslot.
 */
var g_LastViewedAIPlayer = -1;

/**
 * Total number of units that the engine can run with smoothly.
 * It means a 4v4 with 150 population can still run nicely, but more than that might "lag".
 */
var g_PopulationCapacityRecommendation = 1200;

/**
 * Order in which the GUI elements will be shown.
 * All valid options are required to appear here.
 * The ones under "map" are shown in the map selection panel,
 * the others in the "more options" dialog.
 */
var g_OptionOrderGUI = {
	"map": {
		"Dropdown": [
			"mapType",
			"mapFilter",
			"mapSelection",
			"numPlayers",
			"mapSize",
		],
		"Checkbox": [
		],
	},
	"more": {
		"Dropdown": [
			"gameSpeed",
			"victoryCondition",
			"relicCount",
			"victoryDuration",
			"populationCap",
			"startingResources",
			"ceasefire",
		],
		"Checkbox": [
			"regicideGarrison",
			"exploreMap",
			"revealMap",
			"disableTreasures",
			"disableSpies",
			"lockTeams",
			"lastManStanding",
			"enableCheats",
			"enableRating",
		]
	}
};

/**
 * These options must be initialized first, in the given order.
 */
var g_OptionOrderInit = {
	"dropdowns": [
		"mapType",
		"mapFilter",
		"mapSelection"
	],
	"checkboxes": [
	]
};

/**
 * Contains the logic of all multiple-choice gamesettings.
 *
 * Logic
 * ids     - Array of identifier strings that indicate the selected value.
 * default - Returns the index of the default value (not the value itself).
 * defined - Whether a value for the setting is actually specified.
 * get     - The identifier of the currently selected value.
 * select  - Saves and processes the value of the selected index of the ids array.
 *
 * GUI
 * title        - The caption shown in the label.
 * tooltip      - A description shown when hovering the dropdown or a specific item.
 * labels       - Array of translated strings selectable for this dropdown.
 * colors       - Optional array of colors to tint the according dropdown items with.
 * hidden       - If hidden, both the label and dropdown won't be visible. (default: false)
 * enabled      - Only the label will be shown if the setting is disabled. (default: true)
 * autocomplete - Whether to autocomplete translated values of the string. (default: false)
 *                If disabled, still autocompletes the translated title of the setting.
 */
var g_Dropdowns = {
	"mapType": {
		"title": () => translate("Map Type"),
		"tooltip": (hoverIdx) => g_MapTypes.Description[hoverIdx] || translate("Select a map type."),
		"labels": () => g_MapTypes.Title,
		"ids": () => g_MapTypes.Name,
		"default": () => g_MapTypes.Default,
		"defined": () => g_GameAttributes.mapType !== undefined,
		"get": () => g_GameAttributes.mapType,
		"select": (idx) => {
			g_MapData = {};
			g_GameAttributes.mapType = g_MapTypes.Name[idx];
			g_GameAttributes.mapPath = g_MapPath[g_GameAttributes.mapType];
			delete g_GameAttributes.map;

			if (g_GameAttributes.mapType != "scenario")
				g_GameAttributes.settings = {
					"PlayerData": g_DefaultPlayerData.slice(0, 4)
				};

			reloadMapList();
		},
		"autocomplete": true,
	},
	"mapFilter": {
		"title": () => translate("Map Filter"),
		"tooltip": (hoverIdx) => g_MapFilterList.tooltip[hoverIdx] || translate("Select a map filter."),
		"labels": () => g_MapFilterList.name,
		"ids": () => g_MapFilterList.id,
		"default": () => g_MapFilterList.Default,
		"defined": () => g_GameAttributes.mapFilter !== undefined,
		"get": () => g_GameAttributes.mapFilter,
		"select": (idx) => {
			g_GameAttributes.mapFilter = g_MapFilterList.id[idx];
			delete g_GameAttributes.map;
			reloadMapList();
		},
		"autocomplete": true,
	},
	"mapSelection": {
		"title": () => translate("Select Map"),
		"tooltip": (hoverIdx) => g_MapSelectionList.description[hoverIdx] || translate("Select a map to play on."),
		"labels": () => g_MapSelectionList.name,
		"colors": () => g_MapSelectionList.color,
		"ids": () => g_MapSelectionList.file,
		"default": () => 0,
		"defined": () => g_GameAttributes.map !== undefined,
		"get": () => g_GameAttributes.map,
		"select": (idx) => {
			selectMap(g_MapSelectionList.file[idx]);
		},
		"autocomplete": true,
	},
	"mapSize": {
		"title": () => translate("Map Size"),
		"tooltip": (hoverIdx) => g_MapSizes.Tooltip[hoverIdx] || translate("Select map size. (Larger sizes may reduce performance.)"),
		"labels": () => g_MapSizes.Name,
		"ids": () => g_MapSizes.Tiles,
		"default": () => g_MapSizes.Default,
		"defined": () => g_GameAttributes.settings.Size !== undefined,
		"get": () => g_GameAttributes.settings.Size,
		"select": (idx) => {
			g_GameAttributes.settings.Size = g_MapSizes.Tiles[idx];
		},
		"hidden": () => g_GameAttributes.mapType != "random",
		"autocomplete": true,
	},
	"numPlayers": {
		"title": () => translate("Number of Players"),
		"tooltip": (hoverIdx) => translate("Select number of players."),
		"labels": () => g_NumPlayersList,
		"ids": () => g_NumPlayersList,
		"default": () => g_MaxPlayers - 1,
		"defined": () => g_GameAttributes.settings.PlayerData !== undefined,
		"get": () => g_GameAttributes.settings.PlayerData.length,
		"enabled": () => g_GameAttributes.mapType == "random",
		"select": (idx) => {
			let num = idx + 1;
			let pData = g_GameAttributes.settings.PlayerData;
			g_GameAttributes.settings.PlayerData =
				num > pData.length ?
					pData.concat(g_DefaultPlayerData.slice(pData.length, num)) :
					pData.slice(0, num);
			unassignInvalidPlayers(num);
			sanitizePlayerData(g_GameAttributes.settings.PlayerData);
		},
	},
	"populationCap": {
		"title": () => translate("Population Cap"),
		"tooltip": (hoverIdx) => {

			let popCap = g_PopulationCapacities.Population[hoverIdx];
			let players = g_GameAttributes.settings.PlayerData.length;

			if (hoverIdx == -1 || popCap * players <= g_PopulationCapacityRecommendation)
				return translate("Select population limit.");

			return '[color="orange"]' +
				sprintf(translate("Warning: There might be performance issues if all %(players)s players reach %(popCap)s population."), {
					"players": players,
					"popCap": popCap
				}) + '[/color]';
		},
		"labels": () => g_PopulationCapacities.Title,
		"ids": () => g_PopulationCapacities.Population,
		"default": () => g_PopulationCapacities.Default,
		"defined": () => g_GameAttributes.settings.PopulationCap !== undefined,
		"get": () => g_GameAttributes.settings.PopulationCap,
		"select": (idx) => {
			g_GameAttributes.settings.PopulationCap = g_PopulationCapacities.Population[idx];
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"startingResources": {
		"title": () => translate("Starting Resources"),
		"tooltip": (hoverIdx) => {
			return hoverIdx >= 0 ?
				sprintf(translate("Initial amount of each resource: %(resources)s."), {
					"resources": g_StartingResources.Resources[hoverIdx]
				}) :
				translate("Select the game's starting resources.");
		},
		"labels": () => g_StartingResources.Title,
		"ids": () => g_StartingResources.Resources,
		"default": () => g_StartingResources.Default,
		"defined": () => g_GameAttributes.settings.StartingResources !== undefined,
		"get": () => g_GameAttributes.settings.StartingResources,
		"select": (idx) => {
			g_GameAttributes.settings.StartingResources = g_StartingResources.Resources[idx];
		},
		"hidden": () => g_GameAttributes.mapType == "scenario",
		"autocomplete": true,
	},
	"ceasefire": {
		"title": () => translate("Ceasefire"),
		"tooltip": (hoverIdx) => translate("Set time where no attacks are possible."),
		"labels": () => g_Ceasefire.Title,
		"ids": () => g_Ceasefire.Duration,
		"default": () => g_Ceasefire.Default,
		"defined": () => g_GameAttributes.settings.Ceasefire !== undefined,
		"get": () => g_GameAttributes.settings.Ceasefire,
		"select": (idx) => {
			g_GameAttributes.settings.Ceasefire = g_Ceasefire.Duration[idx];
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"victoryCondition": {
		"title": () => translate("Victory Condition"),
		"tooltip": (hoverIdx) => g_VictoryConditions.Description[hoverIdx] || translate("Select victory condition."),
		"labels": () => g_VictoryConditions.Title,
		"ids": () => g_VictoryConditions.Name,
		"default": () => g_VictoryConditions.Default,
		"defined": () => g_GameAttributes.settings.GameType !== undefined,
		"get": () => g_GameAttributes.settings.GameType,
		"select": (idx) => {
			g_GameAttributes.settings.GameType = g_VictoryConditions.Name[idx];
			g_GameAttributes.settings.VictoryScripts = g_VictoryConditions.Scripts[idx];
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
		"autocomplete": true,
	},
	"relicCount": {
		"title": () => translate("Relic Count"),
		"tooltip": (hoverIdx) => translate("Total number of relics spawned on the map."),
		"labels": () => g_RelicCountList,
		"ids": () => g_RelicCountList,
		"default": () => g_RelicCountList.indexOf(5),
		"defined": () => g_GameAttributes.settings.RelicCount !== undefined,
		"get": () => g_GameAttributes.settings.RelicCount,
		"select": (idx) => {
			g_GameAttributes.settings.RelicCount = g_RelicCountList[idx];
		},
		"hidden": () => g_GameAttributes.settings.GameType != "capture_the_relic",
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"victoryDuration": {
		"title": () => translate("Victory Duration"),
		"tooltip": (hoverIdx) => translate("Number of minutes until the player has won."),
		"labels": () => g_VictoryDurations.Title,
		"ids": () => g_VictoryDurations.Duration,
		"default": () => g_VictoryDurations.Default,
		"defined": () => g_GameAttributes.settings.VictoryDuration !== undefined,
		"get": () => g_GameAttributes.settings.VictoryDuration,
		"select": (idx) => {
			g_GameAttributes.settings.VictoryDuration = g_VictoryDurations.Duration[idx];
		},
		"hidden": () =>
			g_GameAttributes.settings.GameType != "wonder" &&
			g_GameAttributes.settings.GameType != "capture_the_relic",
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"gameSpeed": {
		"title": () => translate("Game Speed"),
		"tooltip": (hoverIdx) => translate("Select game speed."),
		"labels": () => g_GameSpeeds.Title,
		"ids": () => g_GameSpeeds.Speed,
		"default": () => g_GameSpeeds.Default,
		"defined": () => g_GameAttributes.gameSpeed !== undefined,
		"get": () => g_GameAttributes.gameSpeed,
		"select": (idx) => {
			g_GameAttributes.gameSpeed = g_GameSpeeds.Speed[idx];
		},
	},
};

/**
 * These dropdowns provide a setting that is repeated once for each player
 * (where idx is the playerID starting from 0 for player 1).
 */
var g_PlayerDropdowns = {
	"playerAssignment": {
		"labels": (idx) => g_PlayerAssignmentList.Name || [],
		"colors": (idx) => g_PlayerAssignmentList.Color || [],
		"ids": (idx) => g_PlayerAssignmentList.Choice || [],
		"default": (idx) => "ai:petra",
		"defined": (idx) => idx < g_GameAttributes.settings.PlayerData.length,
		"get": (idx) => {
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player == idx + 1)
					return "guid:" + guid;

			for (let ai of g_Settings.AIDescriptions)
				if (g_GameAttributes.settings.PlayerData[idx].AI == ai.id)
					return "ai:" + ai.id;

			return "unassigned";
		},
		"select": (selectedIdx, idx) => {

			let choice = g_PlayerAssignmentList.Choice[selectedIdx];
			if (choice == "unassigned" || choice.startsWith("ai:"))
			{
				if (g_IsNetworked)
					Engine.AssignNetworkPlayer(idx+1, "");
				else if (g_PlayerAssignments.local.player == idx+1)
					g_PlayerAssignments.local.player = -1;

				g_GameAttributes.settings.PlayerData[idx].AI = choice.startsWith("ai:") ? choice.substr(3) : "";
			}
			else
				swapPlayers(choice.substr("guid:".length), idx);
		},
		"autocomplete": true,
	},
	"playerTeam": {
		"labels": (idx) => g_PlayerTeamList.label,
		"ids": (idx) => g_PlayerTeamList.id,
		"default": (idx) => 0,
		"defined": (idx) => g_GameAttributes.settings.PlayerData[idx].Team !== undefined,
		"get": (idx) => g_GameAttributes.settings.PlayerData[idx].Team,
		"select": (selectedIdx, idx) => {
			g_GameAttributes.settings.PlayerData[idx].Team = selectedIdx - 1;
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"playerCiv": {
		"tooltip": (hoverIdx, idx) => g_PlayerCivList.tooltip[hoverIdx] || translate("Chose the civilization for this player"),
		"labels": (idx) => g_PlayerCivList.name,
		"colors": (idx) => g_PlayerCivList.color,
		"ids": (idx) => g_PlayerCivList.code,
		"default": (idx) => 0,
		"defined": (idx) => g_GameAttributes.settings.PlayerData[idx].Civ !== undefined,
		"get": (idx) => g_GameAttributes.settings.PlayerData[idx].Civ,
		"select": (selectedIdx, idx) => {
			g_GameAttributes.settings.PlayerData[idx].Civ = g_PlayerCivList.code[selectedIdx];
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
		"autocomplete": true,
	},
	"playerColorPicker": {
		"labels": (idx) => g_PlayerColorPickerList.map(color => "■"),
		"colors": (idx) => g_PlayerColorPickerList.map(color => rgbToGuiColor(color)),
		"ids": (idx) => g_PlayerColorPickerList.map((color, index) => index),
		"default": (idx) => idx,
		"defined": (idx) => g_GameAttributes.settings.PlayerData[idx].Color !== undefined,
		"get": (idx) => g_GameAttributes.settings.PlayerData[idx].Color,
		"select": (selectedIdx, idx) => {
			let playerData = g_GameAttributes.settings.PlayerData;

			// If someone else has that color, give that player the old color
			let pData = playerData.find(pData => sameColor(g_PlayerColorPickerList[selectedIdx], pData.Color));
			if (pData)
				pData.Color = playerData[idx].Color;

			playerData[idx].Color = g_PlayerColorPickerList[selectedIdx];
			ensureUniquePlayerColors(playerData);
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
};

/**
 * Contains the logic of all boolean gamesettings.
 */
var g_Checkboxes = {
	"regicideGarrison": {
		"title": () => translate("Hero Garrison"),
		"tooltip": () => translate("Toggle whether heroes can be garrisoned."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.RegicideGarrison !== undefined,
		"get": () => g_GameAttributes.settings.RegicideGarrison,
		"set": checked => {
			g_GameAttributes.settings.RegicideGarrison = checked;
		},
		"hidden": () => g_GameAttributes.settings.GameType != "regicide",
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"revealMap": {
		"title": () =>
			// Translation: Make sure to differentiate between the revealed map and explored map options!
			translate("Revealed Map"),
		"tooltip":
			// Translation: Make sure to differentiate between the revealed map and explored map options!
			() => translate("Toggle revealed map (see everything)."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.RevealMap !== undefined,
		"get": () => g_GameAttributes.settings.RevealMap,
		"set": checked => {
			g_GameAttributes.settings.RevealMap = checked;

			if (checked)
				g_Checkboxes.exploreMap.set(true);
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"exploreMap": {
		"title":
			// Translation: Make sure to differentiate between the revealed map and explored map options!
			() => translate("Explored Map"),
		"tooltip":
			// Translation: Make sure to differentiate between the revealed map and explored map options!
			() => translate("Toggle explored map (see initial map)."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.ExploreMap !== undefined,
		"get": () => g_GameAttributes.settings.ExploreMap,
		"set": checked => {
			g_GameAttributes.settings.ExploreMap = checked;
		},
		"enabled": () => g_GameAttributes.mapType != "scenario" && !g_GameAttributes.settings.RevealMap,
	},
	"disableTreasures": {
		"title": () => translate("Disable Treasures"),
		"tooltip": () => translate("Disable all treasures on the map."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.DisableTreasures !== undefined,
		"get": () => g_GameAttributes.settings.DisableTreasures,
		"set": checked => {
			g_GameAttributes.settings.DisableTreasures = checked;
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"disableSpies": {
		"title": () => translate("Disable Spies"),
		"tooltip": () => translate("Disable spies during the game."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.DisableSpies !== undefined,
		"get": () => g_GameAttributes.settings.DisableSpies,
		"set": checked => {
			g_GameAttributes.settings.DisableSpies = checked;
		},
		"enabled": () => g_GameAttributes.mapType != "scenario",
	},
	"lockTeams":  {
		"title": () => translate("Teams Locked"),
		"tooltip": () => translate("Toggle locked teams."),
		"default": () => Engine.HasXmppClient(),
		"defined": () => g_GameAttributes.settings.LockTeams !== undefined,
		"get": () => g_GameAttributes.settings.LockTeams,
		"set": checked => {
			g_GameAttributes.settings.LockTeams = checked;
			g_GameAttributes.settings.LastManStanding = false;
		},
		"enabled": () =>
			g_GameAttributes.mapType != "scenario" &&
			!g_GameAttributes.settings.RatingEnabled,
	},
	"lastManStanding":  {
		"title": () => translate("Last Man Standing"),
		"tooltip": () => translate("Toggle whether the last remaining player or the last remaining set of allies wins."),
		"default": () => false,
		"defined": () => g_GameAttributes.settings.LastManStanding !== undefined,
		"get": () => g_GameAttributes.settings.LastManStanding,
		"set": checked => {
			g_GameAttributes.settings.LastManStanding = checked;
		},
		"enabled": () =>
			g_GameAttributes.mapType != "scenario" &&
			!g_GameAttributes.settings.LockTeams,
	},
	"enableCheats":  {
		"title": () => translate("Cheats"),
		"tooltip": () => translate("Toggle the usability of cheats."),
		"default": () => !g_IsNetworked,
		"hidden": () => !g_IsNetworked,
		"defined": () => g_GameAttributes.settings.CheatsEnabled !== undefined,
		"get": () => g_GameAttributes.settings.CheatsEnabled,
		"set": checked => {
			g_GameAttributes.settings.CheatsEnabled = !g_IsNetworked ||
				checked && !g_GameAttributes.settings.RatingEnabled;
		},
		"enabled": () => !g_GameAttributes.settings.RatingEnabled,
	},
	"enableRating": {
		"title": () => translate("Rated Game"),
		"tooltip": () => translate("Toggle if this game will be rated for the leaderboard."),
		"default": () => Engine.HasXmppClient(),
		"hidden": () => !Engine.HasXmppClient(),
		"defined": () => g_GameAttributes.settings.RatingEnabled !== undefined,
		"get": () => !!g_GameAttributes.settings.RatingEnabled,
		"set": checked => {
			g_GameAttributes.settings.RatingEnabled = Engine.HasXmppClient() ? checked : undefined;
			Engine.SetRankedGame(!!g_GameAttributes.settings.RatingEnabled);
			if (checked)
			{
				g_Checkboxes.lockTeams.set(true);
				g_Checkboxes.enableCheats.set(false);
			}
		},
	},
};

/**
 * For setting up arbitrary GUI objects.
 */
var g_MiscControls = {
	"chatPanel": {
		"hidden": () => !g_IsNetworked,
	},
	"chatInput": {
		"tooltip": () => colorizeAutocompleteHotkey(translate("Press %(hotkey)s to autocomplete playernames or settings.")),
	},
	"cheatWarningText": {
		"hidden": () => !g_IsNetworked || !g_GameAttributes.settings.CheatsEnabled,
	},
	"cancelGame": {
		"tooltip": () =>
			Engine.HasXmppClient() ?
				translate("Return to the lobby.") :
				translate("Return to the main menu."),
	},
	"startGame": {
		"caption": () =>
			g_IsController ? translate("Start game!") : g_ReadyData[g_IsReady].caption,
		"tooltip": (hoverIdx) =>
			!g_IsController ?
				g_ReadyData[g_IsReady].tooltip :
				!g_IsNetworked || Object.keys(g_PlayerAssignments).every(guid =>
						g_PlayerAssignments[guid].status || g_PlayerAssignments[guid].player == -1) ?
					translate("Start a new game with the current settings.") :
					translate("Start a new game with the current settings (disabled until all players are ready)"),
		"enabled": () => !g_IsController ||
		                 Object.keys(g_PlayerAssignments).every(guid => g_PlayerAssignments[guid].status ||
		                                                                g_PlayerAssignments[guid].player == -1 ||
		                                                                guid == Engine.GetPlayerGUID() && g_IsController),
		"hidden": () =>
			!g_PlayerAssignments[Engine.GetPlayerGUID()] ||
			g_PlayerAssignments[Engine.GetPlayerGUID()].player == -1 && !g_IsController,
	},
	"civResetButton": {
		"hidden": () => g_GameAttributes.mapType == "scenario" || !g_IsController,
	},
	"teamResetButton": {
		"hidden": () => g_GameAttributes.mapType == "scenario" || !g_IsController,
	},
	// Display these after having hidden every GUI object in the "More Options" dialog
	"moreOptionsLabel": {
		"hidden": () => false,
	},
	"hideMoreOptions": {
		"hidden": () => false,
	},
};

/**
 * Contains options that are repeated for every player.
 */
var g_PlayerMiscControls = {
	"playerBox": {
		"size": (idx) => ["0", 32 * idx, "100%", 32 * (idx + 1)].join(" "),
	},
	"playerName": {
		"caption": (idx) => {
			let pData = g_GameAttributes.settings.PlayerData[idx];

			let assignedGUID = Object.keys(g_PlayerAssignments).find(
				guid => g_PlayerAssignments[guid].player == idx + 1);

			let name = translate(pData.Name || g_DefaultPlayerData[idx].Name);

			if (g_IsNetworked)
				name =
					'[color="' +
					g_ReadyData[assignedGUID ? g_PlayerAssignments[assignedGUID].status : 2].color +
					'"]' +  name + '[/color]';

			return name;
		},
	},
	"playerColor": {
		"sprite": (idx) => "color:" + rgbToGuiColor(g_GameAttributes.settings.PlayerData[idx].Color) + " 100",
	},
	"playerConfig": {
		"hidden": (idx) => !g_GameAttributes.settings.PlayerData[idx].AI,
		"onPress": (idx) => function() {
			openAIConfig(idx);
		},
	},
};

/**
 * Initializes some globals without touching the GUI.
 *
 * @param {Object} attribs - context data sent by the lobby / mainmenu
 */
function init(attribs)
{
	if (!g_Settings)
	{
		cancelSetup();
		return;
	}

	if (["offline", "server", "client"].indexOf(attribs.type) == -1)
	{
		error("Unexpected 'type' in gamesetup init: " + attribs.type);
		cancelSetup();
		return;
	}

	g_IsNetworked = attribs.type != "offline";
	g_IsController = attribs.type != "client";
	g_IsTutorial = attribs.tutorial &&  attribs.tutorial == true;
	g_ServerName = attribs.serverName;
	g_ServerPort = attribs.serverPort;
	g_StunEndpoint = attribs.stunEndpoint;

	if (!g_IsNetworked)
		g_PlayerAssignments = {
			"local": {
				"name": singleplayerName(),
				"player": 1
			}
		};

	// Replace empty playername when entering a singleplayermatch for the first time
	if (!g_IsNetworked)
	{
		Engine.ConfigDB_CreateValue("user", "playername.singleplayer", singleplayerName());
		Engine.ConfigDB_WriteValueToFile("user", "playername.singleplayer", singleplayerName(), "config/user.cfg");
	}

	initDefaults();
	supplementDefaults();

	setTimeout(displayGamestateNotifications, 1000);
}

function initDefaults()
{
	// Remove gaia from both arrays
	g_DefaultPlayerData = g_Settings.PlayerDefaults;
	g_DefaultPlayerData.shift();

	// Don't change the underlying defaults file, as Atlas uses that file too
	for (let i in g_DefaultPlayerData)
	{
		g_DefaultPlayerData[i].Civ = "random";
		g_DefaultPlayerData[i].Team = -1;
	}
}

/**
 * Sets default values for all g_GameAttribute settings which don't have a value set.
 */
function supplementDefaults()
{
	for (let dropdown in g_Dropdowns)
		if (!g_Dropdowns[dropdown].defined())
			g_Dropdowns[dropdown].select(g_Dropdowns[dropdown].default());

	for (let checkbox in g_Checkboxes)
		if (!g_Checkboxes[checkbox].defined())
			g_Checkboxes[checkbox].set(g_Checkboxes[checkbox].default());

	for (let dropdown in g_PlayerDropdowns)
		for (let i = 0; i < g_GameAttributes.settings.PlayerData.length; ++i)
			if (!isControlArrayElementHidden(i) && !g_PlayerDropdowns[dropdown].defined(i))
				g_PlayerDropdowns[dropdown].select(g_PlayerDropdowns[dropdown].default(i), i);
}

/**
 * Called after the first tick.
 */
function initGUIObjects()
{
	for (let dropdown of g_OptionOrderInit.dropdowns)
		initDropdown(dropdown);

	for (let dropdown in g_Dropdowns)
		if (g_OptionOrderInit.dropdowns.indexOf(dropdown) == -1)
			initDropdown(dropdown);

	for (let checkbox of g_OptionOrderInit.checkboxes)
		initCheckbox(checkbox);

	for (let checkbox in g_Checkboxes)
		if (g_OptionOrderInit.checkboxes.indexOf(checkbox) == -1)
			initCheckbox(checkbox);

	for (let dropdown in g_PlayerDropdowns)
		initPlayerDropdowns(dropdown);

	resizeMoreOptionsWindow();

	initSPTips();

	loadPersistMatchSettings();
	updateGameAttributes();
	sendRegisterGameStanzaImmediate();

	if (g_IsTutorial)
	{
		launchTutorial();
		return;
	}

	Engine.GetGUIObjectByName("loadingWindow").hidden = true;
	Engine.GetGUIObjectByName("setupWindow").hidden = false;

	if (g_IsNetworked)
		Engine.GetGUIObjectByName("chatInput").focus();
}

/**
 * The main options (like map selection) and player arrays have specific names.
 * Options in the "More Options" dialog use a generic name.
 */
function getGUIObjectNameFromSetting(name)
{
	for (let panel in g_OptionOrderGUI)
		for (let type in g_OptionOrderGUI[panel])
		{
			let idx = g_OptionOrderGUI[panel][type].indexOf(name);
			if (idx != -1)
				return [panel + "Option" + type, "[" + idx + "]"];
		}

	// Assume there is a GUI object with exactly that setting name
	return [name, ""];
}

function initDropdown(name, idx)
{
	let [guiName, guiIdx] = getGUIObjectNameFromSetting(name);
	let idxName = idx === undefined ? "": "[" + idx + "]";
	let data = (idx === undefined ? g_Dropdowns : g_PlayerDropdowns)[name];

	let dropdown = Engine.GetGUIObjectByName(guiName + guiIdx + idxName);

	dropdown.list = data.labels(idx).map((label, id) =>
		data.colors && data.colors(idx) ?
			'[color="' + data.colors(idx)[id] + '"]' + label + "[/color]" :
			label);

	dropdown.list_data = data.ids(idx);

	dropdown.onSelectionChange = function() {

		if (!g_IsController ||
		    g_IsInGuiUpdate ||
		    !this.list_data[this.selected] ||
		    data.hidden && data.hidden(idx) ||
		    data.enabled && !data.enabled(idx))
			return;

		data.select(this.selected, idx);

		supplementDefaults();
		updateGameAttributes();
	};

	if (data.tooltip)
		dropdown.onHoverChange = function() {
			this.tooltip = data.tooltip(this.hovered, idx);
		};
}

function initPlayerDropdowns(name)
{
	for (let i = 0; i < g_MaxPlayers; ++i)
		initDropdown(name, i);
}

function initCheckbox(name)
{
	let [guiName, guiIdx] = getGUIObjectNameFromSetting(name);
	Engine.GetGUIObjectByName(guiName + guiIdx).onPress = function() {

		let obj = g_Checkboxes[name];

		if (!g_IsController ||
		    g_IsInGuiUpdate ||
		    obj.enabled && !obj.enabled() ||
		    obj.hidden && obj.hidden())
			return;

		obj.set(this.checked);

		supplementDefaults();
		updateGameAttributes();
	};
}

function initSPTips()
{
	if (g_IsNetworked || Engine.ConfigDB_GetValue("user", "gui.gamesetup.enabletips") !== "true")
		return;

	Engine.GetGUIObjectByName("spTips").hidden = false;
	Engine.GetGUIObjectByName("displaySPTips").checked = true;
	Engine.GetGUIObjectByName("aiTips").caption = Engine.TranslateLines(Engine.ReadFile("gui/gamesetup/ai.txt"));
}

function saveSPTipsSetting()
{
	let enabled = String(Engine.GetGUIObjectByName("displaySPTips").checked);
	Engine.ConfigDB_CreateValue("user", "gui.gamesetup.enabletips", enabled);
	Engine.ConfigDB_WriteValueToFile("user", "gui.gamesetup.enabletips", enabled, "config/user.cfg");
}

function verticallyDistributeGUIObjects(parent, objectHeight, ignore)
{
	let yPos = undefined;

	let parentObject = Engine.GetGUIObjectByName(parent);
	for (let child of parentObject.children)
	{
		if (ignore.indexOf(child.name) != -1)
			continue;

		let childSize = child.size;
		yPos = yPos || childSize.top;

		if (child.hidden)
			continue;

		childSize.top = yPos;
		childSize.bottom = yPos + objectHeight - 2;
		child.size = childSize;

		yPos += objectHeight;
	}
	return yPos;
}

/**
 * Remove empty space in case of hidden options (like cheats, rating or victory duration)
 */
function resizeMoreOptionsWindow()
{
	verticallyDistributeGUIObjects("mapOptions", 32, []);

	let yPos = verticallyDistributeGUIObjects("moreOptions", 32, ["moreOptionsLabel"]);

	// Resize the vertically centered window containing the options
	let moreOptions = Engine.GetGUIObjectByName("moreOptions");
	let mSize = moreOptions.size;
	mSize.bottom = mSize.top + yPos + 20;
	moreOptions.size = mSize;
}

/**
 * Called when the client disconnects.
 * The other cases from NetClient should never occur in the gamesetup.
 */
function handleNetStatusMessage(message)
{
	if (message.status != "disconnected")
	{
		error("Unrecognised netstatus type " + message.status);
		return;
	}

	cancelSetup();
	reportDisconnect(message.reason, true);
}

/**
 * Called whenever a client clicks on ready (or not ready).
 */
function handleReadyMessage(message)
{
	--g_ReadyChanged;

	if (g_ReadyChanged < 1 && g_PlayerAssignments[message.guid].player != -1)
		addChatMessage({
			"type": "ready",
			"status": message.status,
			"guid": message.guid
		});

	g_PlayerAssignments[message.guid].status = message.status;
	updateGUIObjects();
}

/**
 * Called after every player is ready and the host decided to finally start the game.
 */
function handleGamestartMessage(message)
{
	// Immediately inform the lobby server instead of waiting for the load to finish
	if (g_IsController && Engine.HasXmppClient())
	{
		sendRegisterGameStanzaImmediate();
		let clients = formatClientsForStanza();
		Engine.SendChangeStateGame(clients.connectedPlayers, clients.list);
	}

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": g_GameAttributes,
		"isNetworked" : g_IsNetworked,
		"playerAssignments": g_PlayerAssignments,
		"isController": g_IsController
	});
}

/**
 * Called whenever the host changed any setting.
 */
function handleGamesetupMessage(message)
{
	if (!message.data)
		return;

	g_GameAttributes = message.data;

	if (!!g_GameAttributes.settings.RatingEnabled)
	{
		g_GameAttributes.settings.CheatsEnabled = false;
		g_GameAttributes.settings.LockTeams = true;
		g_GameAttributes.settings.LastManStanding = false;
	}

	Engine.SetRankedGame(!!g_GameAttributes.settings.RatingEnabled);

	resetReadyData();

	updateGUIObjects();
}

/**
 * Called whenever a client joins/leaves or any gamesetting is changed.
 */
function handlePlayerAssignmentMessage(message)
{
	let playerChange = false;

	for (let guid in message.newAssignments)
		if (!g_PlayerAssignments[guid])
		{
			onClientJoin(guid, message.newAssignments);
			playerChange = true;
		}

	for (let guid in g_PlayerAssignments)
		if (!message.newAssignments[guid])
		{
			onClientLeave(guid);
			playerChange = true;
		}

	g_PlayerAssignments = message.newAssignments;

	sanitizePlayerData(g_GameAttributes.settings.PlayerData);
	updateGUIObjects();

	if (playerChange)
		sendRegisterGameStanzaImmediate();
	else
		sendRegisterGameStanza();
}

function onClientJoin(newGUID, newAssignments)
{
	let playername = newAssignments[newGUID].name;

	addChatMessage({
		"type": "connect",
		"guid": newGUID,
		"username": playername
	});

	let isRejoiningPlayer = newAssignments[newGUID].player != -1;

	// Assign the client (or only buddies if prefered) to an unused playerslot and rejoining players to their old slot
	if (!isRejoiningPlayer && playername != newAssignments[Engine.GetPlayerGUID()].name)
	{
		let assignOption = Engine.ConfigDB_GetValue("user", "gui.gamesetup.assignplayers");
		if (assignOption == "disabled" ||
		    assignOption == "buddies" && g_Buddies.indexOf(splitRatingFromNick(playername)[0]) == -1)
			return;
	}

	let freeSlot = g_GameAttributes.settings.PlayerData.findIndex((v,i) =>
		Object.keys(g_PlayerAssignments).every(guid => g_PlayerAssignments[guid].player != i+1)
	);

	// Client is not and cannot become assigned as player
	if (!isRejoiningPlayer && freeSlot == -1)
		return;

	// Assign the joining client to the free slot
	if (g_IsController && !isRejoiningPlayer)
		Engine.AssignNetworkPlayer(freeSlot + 1, newGUID);

	resetReadyData();
}

function onClientLeave(guid)
{
	addChatMessage({
		"type": "disconnect",
		"guid": guid
	});

	if (g_PlayerAssignments[guid].player != -1)
		resetReadyData();
}

/**
 * Doesn't translate, so that lobby clients can do that locally
 * (even if they don't have that map).
 */
function getMapDisplayName(map)
{
	if (map == "random")
		return map;

	let mapData = loadMapData(map);
	if (!mapData || !mapData.settings || !mapData.settings.Name)
		return map;

	return mapData.settings.Name;
}

function getMapPreview(map)
{
	let mapData = loadMapData(map);
	if (!mapData || !mapData.settings || !mapData.settings.Preview)
		return "nopreview.png";

	return mapData.settings.Preview;
}

/**
 * Initialize the dropdown containing all maps for the selected maptype and mapfilter.
 */
function reloadMapList()
{
	if (!g_MapPath[g_GameAttributes.mapType])
	{
		error("Unexpected map type: " + g_GameAttributes.mapType);
		return;
	}

	let mapFiles = g_GameAttributes.mapType == "random" ?
		getJSONFileList(g_GameAttributes.mapPath) :
		getXMLFileList(g_GameAttributes.mapPath);

	// Apply map filter, if any defined
	let mapList = [];

	// TODO: Should verify these are valid maps before adding to list
	for (let mapFile of mapFiles)
	{
		let file = g_GameAttributes.mapPath + mapFile;
		let mapData = loadMapData(file);
		let filterID = g_MapFilterList.id.findIndex(id => id == g_GameAttributes.mapFilter);
		let mapFilter = g_MapFilterList.filter[filterID] || undefined;

		if (!mapData.settings || mapFilter && !mapFilter(mapData.settings.Keywords || []))
			continue;

		mapList.push({
			"file": file,
			"name": translate(getMapDisplayName(file)),
			"color": g_ColorRegular,
			"description": translate(mapData.settings.Description)
		});
	}

	mapList = mapList.sort(sortNameIgnoreCase);

	if (g_GameAttributes.mapType == "random")
		mapList.unshift({
			"file": "random",
			"name": translateWithContext("map selection", "Random"),
			"color": g_ColorRandom,
			"description": "Picks one of the maps of the given maptype and filter at random."
		});

	g_MapSelectionList = prepareForDropdown(mapList);
	initDropdown("mapSelection");
}

function loadMapData(name)
{
	if (!name || !g_MapPath[g_GameAttributes.mapType])
		return undefined;

	if (name == "random")
		return { "settings": { "Name": "", "Description": "" } };

	if (!g_MapData[name])
		g_MapData[name] = g_GameAttributes.mapType == "random" ?
			Engine.ReadJSONFile(name + ".json") :
			Engine.LoadMapSettings(name);

	return g_MapData[name];
}

/**
 * Sets the gameattributes the way they were the last time the user left the gamesetup.
 */
function loadPersistMatchSettings()
{
	if (!g_IsController || Engine.ConfigDB_GetValue("user", "persistmatchsettings") != "true" || g_IsTutorial)
		return;

	let settingsFile = g_IsNetworked ? g_MatchSettings_MP : g_MatchSettings_SP;
	if (!Engine.FileExists(settingsFile))
		return;

	let attrs = Engine.ReadJSONFile(settingsFile);
	if (!attrs || !attrs.settings)
		return;

	g_IsInGuiUpdate = true;

	let mapName = attrs.map || "";
	let mapSettings = attrs.settings;

	g_GameAttributes = attrs;

	if (!g_IsNetworked)
		mapSettings.CheatsEnabled = true;

	// Replace unselectable civs with random civ
	let playerData = mapSettings.PlayerData;
	if (playerData && g_GameAttributes.mapType != "scenario")
		for (let i in playerData)
			if (!g_CivData[playerData[i].Civ] || !g_CivData[playerData[i].Civ].SelectableInGameSetup)
				playerData[i].Civ = "random";

	// Apply map settings
	let newMapData = loadMapData(mapName);
	if (newMapData && newMapData.settings)
	{
		for (let prop in newMapData.settings)
			mapSettings[prop] = newMapData.settings[prop];

		if (playerData)
			mapSettings.PlayerData = playerData;
	}

	if (mapSettings.PlayerData)
		sanitizePlayerData(mapSettings.PlayerData);

	// Reload, as the maptype or mapfilter might have changed
	reloadMapList();

	g_GameAttributes.settings.RatingEnabled = Engine.HasXmppClient();
	Engine.SetRankedGame(g_GameAttributes.settings.RatingEnabled);

	supplementDefaults();

	g_IsInGuiUpdate = false;
}

function savePersistMatchSettings()
{
	if (g_IsTutorial)
		return;
	let attributes = Engine.ConfigDB_GetValue("user", "persistmatchsettings") == "true" ? g_GameAttributes : {};
	Engine.WriteJSONFile(g_IsNetworked ? g_MatchSettings_MP : g_MatchSettings_SP, attributes);
}

function sanitizePlayerData(playerData)
{
	// Remove gaia
	if (playerData.length && !playerData[0])
		playerData.shift();

	playerData.forEach((pData, index) => {

		// Use defaults if the map doesn't specify a value
		for (let prop in g_DefaultPlayerData[index])
			if (!(prop in pData))
				pData[prop] = g_DefaultPlayerData[index][prop];

		// Replace colors with the best matching color of PlayerDefaults
		if (g_GameAttributes.mapType != "scenario")
		{
			let colorDistances = g_PlayerColorPickerList.map(color => colorDistance(color, pData.Color));
			let smallestDistance = colorDistances.find(distance => colorDistances.every(distance2 => (distance2 >= distance)));
			pData.Color = g_PlayerColorPickerList.find(color => colorDistance(color, pData.Color) == smallestDistance);
		}

		// If there is a player in that slot, then there can't be an AI
		if (Object.keys(g_PlayerAssignments).some(guid => g_PlayerAssignments[guid].player == index + 1))
			pData.AI = "";
	});

	ensureUniquePlayerColors(playerData);
}

function cancelSetup()
{
	if (g_IsController)
		savePersistMatchSettings();

	Engine.DisconnectNetworkGame();

	if (Engine.HasXmppClient())
	{
		Engine.LobbySetPlayerPresence("available");

		if (g_IsController)
			Engine.SendUnregisterGame();

		Engine.SwitchGuiPage("page_lobby.xml");
	}
	else
		Engine.SwitchGuiPage("page_pregame.xml");
}

/**
 * Can't init the GUI before the first tick.
 * Process netmessages afterwards.
 */
function onTick()
{
	if (!g_Settings)
		return;

	// First tick happens before first render, so don't load yet
	if (g_LoadingState == 0)
		++g_LoadingState;
	else if (g_LoadingState == 1)
	{
		initGUIObjects();
		++g_LoadingState;
	}
	else if (g_LoadingState == 2)
		handleNetMessages();

	updateTimers();
}

/**
 * Handles all pending messages sent by the net client.
 */
function handleNetMessages()
{
	while (g_IsNetworked)
	{
		let message = Engine.PollNetworkClient();
		if (!message)
			break;

		log("Net message: " + uneval(message));

		if (g_NetMessageTypes[message.type])
			g_NetMessageTypes[message.type](message);
		else
			error("Unrecognised net message type " + message.type);
	}
}

/**
 * Called when the map or the number of players changes.
 */
function unassignInvalidPlayers(maxPlayers)
{
	if (g_IsNetworked)
	{
		// Remove invalid playerIDs from the servers playerassignments copy
		for (let playerID = +maxPlayers + 1; playerID <= g_MaxPlayers; ++playerID)
			Engine.AssignNetworkPlayer(playerID, "");
	}
	else if (g_PlayerAssignments.local.player > maxPlayers)
		g_PlayerAssignments.local.player = -1;
}

function ensureUniquePlayerColors(playerData)
{
	for (let i = playerData.length - 1; i >= 0; --i)
		// If someone else has that color, assign an unused color
		if (playerData.some((pData, j) => i != j && sameColor(playerData[i].Color, pData.Color)))
			playerData[i].Color = g_PlayerColorPickerList.find(color => playerData.every(pData => !sameColor(color, pData.Color)));
}

function selectMap(name)
{
	// Reset some map specific properties which are not necessarily redefined on each map
	for (let prop of ["TriggerScripts", "CircularMap", "Garrison", "DisabledTemplates"])
		g_GameAttributes.settings[prop] = undefined;

	let mapData = loadMapData(name);
	let mapSettings = mapData && mapData.settings ? deepcopy(mapData.settings) : {};

	// Reset victory conditions
	if (g_GameAttributes.mapType != "random")
	{
		let victoryIdx = g_VictoryConditions.Name.indexOf(mapSettings.GameType || "") != -1 ? g_VictoryConditions.Name.indexOf(mapSettings.GameType) : g_VictoryConditions.Default;
		g_GameAttributes.settings.GameType = g_VictoryConditions.Name[victoryIdx];
		g_GameAttributes.settings.VictoryScripts = g_VictoryConditions.Scripts[victoryIdx];
	}

	if (g_GameAttributes.mapType == "scenario")
	{
		delete g_GameAttributes.settings.VictoryDuration;
		delete g_GameAttributes.settings.LastManStanding;
		delete g_GameAttributes.settings.RegicideGarrison;
	}

	if (mapSettings.PlayerData)
		sanitizePlayerData(mapSettings.PlayerData);

	// Copy any new settings
	g_GameAttributes.map = name;
	g_GameAttributes.script = mapSettings.Script;
	if (g_GameAttributes.map !== "random")
		for (let prop in mapSettings)
			g_GameAttributes.settings[prop] = mapSettings[prop];

	unassignInvalidPlayers(g_GameAttributes.settings.PlayerData.length);
	supplementDefaults();
}

function isControlArrayElementHidden(idx)
{
	return idx !== undefined && idx >= g_GameAttributes.settings.PlayerData.length;
}

/**
 * @param idx - Only specified for dropdown arrays.
 */
function updateGUIDropdown(name, idx = undefined)
{
	let [guiName, guiIdx] = getGUIObjectNameFromSetting(name);
	let idxName = idx === undefined ? "": "[" + idx + "]";

	let dropdown = Engine.GetGUIObjectByName(guiName + guiIdx + idxName);
	let label = Engine.GetGUIObjectByName(guiName + "Text" + guiIdx + idxName);
	let frame = Engine.GetGUIObjectByName(guiName + "Frame" + guiIdx + idxName);
	let title = Engine.GetGUIObjectByName(guiName + "Title" + guiIdx + idxName);

	let indexHidden = isControlArrayElementHidden(idx);
	let obj = (idx === undefined ? g_Dropdowns : g_PlayerDropdowns)[name];

	let selected = indexHidden ? -1 : dropdown.list_data.indexOf(String(obj.get(idx)));
	let enabled = !indexHidden && (!obj.enabled || obj.enabled(idx));
	let hidden = indexHidden || obj.hidden && obj.hidden(idx);

	dropdown.hidden = !g_IsController || !enabled || hidden;
	dropdown.selected = indexHidden ? -1 : selected;
	dropdown.tooltip = !indexHidden && obj.tooltip ? obj.tooltip(-1, idx) : "";

	if (frame)
		frame.hidden = hidden;

	if (title && obj.title && !indexHidden)
		title.caption = sprintf(translate("%(option)s:"), { "option": obj.title(idx) });

	if (label && !indexHidden)
	{
		label.hidden = g_IsController && enabled || hidden;
		label.caption = selected == -1 ? translateWithContext("option value", "Unknown") : dropdown.list[selected];
	}
}

/**
 * Not used for the player assignments, so playerCheckboxes are not implemented,
 * hence no index.
 */
function updateGUICheckbox(name)
{
	let obj = g_Checkboxes[name];

	let checked = obj.get();
	let hidden = obj.hidden && obj.hidden();
	let enabled = !obj.enabled || obj.enabled();

	let [guiName, guiIdx] = getGUIObjectNameFromSetting(name);
	let checkbox = Engine.GetGUIObjectByName(guiName + guiIdx);
	let label = Engine.GetGUIObjectByName(guiName + "Text" + guiIdx);
	let frame = Engine.GetGUIObjectByName(guiName + "Frame" + guiIdx);
	let title = Engine.GetGUIObjectByName(guiName + "Title" + guiIdx);

	checkbox.checked = checked;
	checkbox.enabled = enabled;
	checkbox.hidden = hidden || !g_IsController;
	checkbox.tooltip = obj.tooltip ? obj.tooltip() : "";

	label.caption = checked ? translate("Yes") : translate("No");
	label.hidden = hidden || g_IsController;

	if (frame)
		frame.hidden = hidden;

	if (title && obj.title)
		title.caption = sprintf(translate("%(option)s:"), { "option": obj.title() });
}

function updateGUIMiscControl(name, idx)
{
	let idxName = idx === undefined ? "": "[" + idx + "]";
	let obj = (idx === undefined ? g_MiscControls : g_PlayerMiscControls)[name];

	let control = Engine.GetGUIObjectByName(name + idxName);
	if (!control)
		warn("No GUI object with name '" + name + "'");

	let hide = isControlArrayElementHidden(idx);
	control.hidden = hide;

	if (hide)
		return;

	for (let property in obj)
		control[property] = obj[property](idx);
}

function launchGame()
{
	if (!g_IsController)
	{
		error("Only host can start game");
		return;
	}

	if (!g_GameAttributes.map)
		return;

	savePersistMatchSettings();

	// Select random map
	if (g_GameAttributes.map == "random")
	{
		let victoryScriptsSelected = g_GameAttributes.settings.VictoryScripts;
		let gameTypeSelected = g_GameAttributes.settings.GameType;
		selectMap(pickRandom(g_Dropdowns.mapSelection.ids().slice(1)));
		g_GameAttributes.settings.VictoryScripts = victoryScriptsSelected;
		g_GameAttributes.settings.GameType = gameTypeSelected;
	}

	g_GameAttributes.settings.TriggerScripts = g_GameAttributes.settings.VictoryScripts.concat(g_GameAttributes.settings.TriggerScripts || []);

	// Prevent reseting the readystate
	g_GameStarted = true;

	g_GameAttributes.settings.mapType = g_GameAttributes.mapType;

	// Get a unique array of selectable cultures
	let cultures = Object.keys(g_CivData).filter(civ => g_CivData[civ].SelectableInGameSetup).map(civ => g_CivData[civ].Culture);
	cultures = cultures.filter((culture, index) => cultures.indexOf(culture) === index);

	// Determine random civs and botnames
	for (let i in g_GameAttributes.settings.PlayerData)
	{
		// Pick a random civ of a random culture
		let chosenCiv = g_GameAttributes.settings.PlayerData[i].Civ || "random";
		if (chosenCiv == "random")
		{
			let culture = pickRandom(cultures);
			chosenCiv = pickRandom(Object.keys(g_CivData).filter(civ => g_CivData[civ].Culture == culture));
		}
		g_GameAttributes.settings.PlayerData[i].Civ = chosenCiv;

		// Pick one of the available botnames for the chosen civ
		if (g_GameAttributes.mapType === "scenario" || !g_GameAttributes.settings.PlayerData[i].AI)
			continue;

		let chosenName = pickRandom(g_CivData[chosenCiv].AINames);

		if (!g_IsNetworked)
			chosenName = translate(chosenName);

		// Count how many players use the chosenName
		let usedName = g_GameAttributes.settings.PlayerData.filter(pData => pData.Name && pData.Name.indexOf(chosenName) !== -1).length;

		g_GameAttributes.settings.PlayerData[i].Name = !usedName ? chosenName : sprintf(translate("%(playerName)s %(romanNumber)s"), { "playerName": chosenName, "romanNumber": g_RomanNumbers[usedName+1] });
	}

	// Copy playernames for the purpose of replays
	for (let guid in g_PlayerAssignments)
	{
		let player = g_PlayerAssignments[guid];
		if (player.player > 0)	// not observer or GAIA
			g_GameAttributes.settings.PlayerData[player.player - 1].Name = player.name;
	}

	// Seed used for both map generation and simulation
	g_GameAttributes.settings.Seed = randIntExclusive(0, Math.pow(2, 32));
	g_GameAttributes.settings.AISeed = randIntExclusive(0, Math.pow(2, 32));

	// Used for identifying rated game reports for the lobby
	g_GameAttributes.matchID = Engine.GetMatchID();

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		Engine.StartNetworkGame();
	}
	else
	{
		// Find the player ID which the user has been assigned to
		let playerID = -1;
		for (let i in g_GameAttributes.settings.PlayerData)
		{
			let assignBox = Engine.GetGUIObjectByName("playerAssignment["+i+"]");
			if (assignBox.list_data[assignBox.selected] == "guid:local")
				playerID = +i+1;
		}

		Engine.StartGame(g_GameAttributes, playerID);
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes,
			"isNetworked": g_IsNetworked,
			"playerAssignments": g_PlayerAssignments
		});
	}
}

function launchTutorial()
{
	g_GameAttributes.mapType = "scenario";
	selectMap("maps/tutorials/starting_economy_walkthrough");
	launchGame();
}

/**
 * Don't set any attributes here, just show the changes in the GUI.
 *
 * Unless the mapsettings don't specify a property and the user didn't set it in g_GameAttributes previously.
 */
function updateGUIObjects()
{
	g_IsInGuiUpdate = true;

	reloadMapList();
	updatePlayerAssignmentChoices();

	// Hide exceeding dropdowns and checkboxes
	for (let panel in g_OptionOrderGUI)
		for (let child of Engine.GetGUIObjectByName(panel + "Options").children)
			child.hidden = true;

	// Show the relevant ones
	for (let name in g_Dropdowns)
		updateGUIDropdown(name);

	for (let name in g_Checkboxes)
		updateGUICheckbox(name);

	for (let i = 0; i < g_MaxPlayers; ++i)
	{
		for (let name in g_PlayerDropdowns)
			updateGUIDropdown(name, i);

		for (let name in g_PlayerMiscControls)
			updateGUIMiscControl(name, i);
	}

	for (let name in g_MiscControls)
		updateGUIMiscControl(name);

	updateGameDescription();
	resizeMoreOptionsWindow();
	rightAlignCancelButton();
	updateAutocompleteEntries();

	g_IsInGuiUpdate = false;

	// Refresh AI config page
	if (g_LastViewedAIPlayer != -1)
	{
		Engine.PopGuiPage();
		openAIConfig(g_LastViewedAIPlayer);
	}
}

function rightAlignCancelButton()
{
	const offset = 10;

	let startGame = Engine.GetGUIObjectByName("startGame");
	let right = startGame.hidden ? startGame.size.right : startGame.size.left - offset;

	let cancelGame = Engine.GetGUIObjectByName("cancelGame");
	let cancelGameSize = cancelGame.size;
	let buttonWidth = cancelGameSize.right - cancelGameSize.left;
	cancelGameSize.right = right;
	right -= buttonWidth;

	for (let element of ["cheatWarningText", "onscreenToolTip"])
	{
		let elementSize = Engine.GetGUIObjectByName(element).size;
		elementSize.right = right - (cancelGameSize.left - elementSize.right);
		Engine.GetGUIObjectByName(element).size = elementSize;
	}

	cancelGameSize.left = right;
	cancelGame.size = cancelGameSize;
}

function updateGameDescription()
{
	setMapPreviewImage("mapPreview", getMapPreview(g_GameAttributes.map));

	Engine.GetGUIObjectByName("mapInfoName").caption =
		translateMapTitle(getMapDisplayName(g_GameAttributes.map));

	Engine.GetGUIObjectByName("mapInfoDescription").caption = getGameDescription();
}

/**
 * Broadcast the changed settings to all clients and the lobbybot.
 */
function updateGameAttributes()
{
	if (g_IsInGuiUpdate || !g_IsController)
		return;

	if (g_IsNetworked)
	{
		Engine.SetNetworkGameAttributes(g_GameAttributes);
		if (g_LoadingState >= 2)
			sendRegisterGameStanza();
		resetReadyData();
	}
	else
		updateGUIObjects();
}

function openAIConfig(playerSlot)
{
	g_LastViewedAIPlayer = playerSlot;

	Engine.PushGuiPage("page_aiconfig.xml", {
		"callback": "AIConfigCallback",
		"isController": g_IsController,
		"playerSlot": playerSlot,
		"id": g_GameAttributes.settings.PlayerData[playerSlot].AI,
		"difficulty": g_GameAttributes.settings.PlayerData[playerSlot].AIDiff
	});
}

/**
 * Called after closing the dialog.
 */
function AIConfigCallback(ai)
{
	g_LastViewedAIPlayer = -1;

	if (!ai.save || !g_IsController)
		return;

	g_GameAttributes.settings.PlayerData[ai.playerSlot].AI = ai.id;
	g_GameAttributes.settings.PlayerData[ai.playerSlot].AIDiff = ai.difficulty;

	updateGameAttributes();
}

function updatePlayerAssignmentChoices()
{
	let playerChoices = sortGUIDsByPlayerID().map(guid => ({
		"Choice": "guid:" + guid,
		"Color": g_PlayerAssignments[guid].player == -1 ? g_PlayerAssignmentColors.observer : g_PlayerAssignmentColors.player,
		"Name": g_PlayerAssignments[guid].name
	}));

	// Only display hidden AIs if the map preselects them
	let aiChoices = g_Settings.AIDescriptions
		.filter(ai => !ai.data.hidden || g_GameAttributes.settings.PlayerData.some(pData => pData.AI == ai.id))
		.map(ai => ({
			"Choice": "ai:" + ai.id,
			"Name": sprintf(translate("AI: %(ai)s"), {
				"ai": translate(ai.data.name)
			}),
			"Color": g_PlayerAssignmentColors.AI
	}));

	let unassignedSlot = [{
		"Choice": "unassigned",
		"Name": translate("Unassigned"),
		"Color": g_PlayerAssignmentColors.unassigned
	}];
	g_PlayerAssignmentList = prepareForDropdown(playerChoices.concat(aiChoices).concat(unassignedSlot));

	initPlayerDropdowns("playerAssignment");
}

function swapPlayers(guid, newSlot)
{
	// Player slots are indexed from 0 as Gaia is omitted.
	let newPlayerID = newSlot + 1;
	let playerID = g_PlayerAssignments[guid].player;

	// Attempt to swap the player or AI occupying the target slot,
	// if any, into the slot this player is currently in.
	if (playerID != -1)
	{
		for (let guid in g_PlayerAssignments)
		{
			// Move the player in the destination slot into the current slot.
			if (g_PlayerAssignments[guid].player != newPlayerID)
				continue;

			if (g_IsNetworked)
				Engine.AssignNetworkPlayer(playerID, guid);
			else
				g_PlayerAssignments[guid].player = playerID;
			break;
		}

		// Transfer the AI from the target slot to the current slot.
		g_GameAttributes.settings.PlayerData[playerID - 1].AI = g_GameAttributes.settings.PlayerData[newSlot].AI;
		g_GameAttributes.settings.PlayerData[playerID - 1].AIDiff = g_GameAttributes.settings.PlayerData[newSlot].AIDiff;

		// Swap civilizations and colors if they aren't fixed
		if (g_GameAttributes.mapType != "scenario")
		{
			[g_GameAttributes.settings.PlayerData[playerID - 1].Civ, g_GameAttributes.settings.PlayerData[newSlot].Civ] =
				[g_GameAttributes.settings.PlayerData[newSlot].Civ, g_GameAttributes.settings.PlayerData[playerID - 1].Civ];
			[g_GameAttributes.settings.PlayerData[playerID - 1].Color, g_GameAttributes.settings.PlayerData[newSlot].Color] =
				[g_GameAttributes.settings.PlayerData[newSlot].Color, g_GameAttributes.settings.PlayerData[playerID - 1].Color];
		}
	}

	if (g_IsNetworked)
		Engine.AssignNetworkPlayer(newPlayerID, guid);
	else
		g_PlayerAssignments[guid].player = newPlayerID;

	g_GameAttributes.settings.PlayerData[newSlot].AI = "";
}

function submitChatInput()
{
	let input = Engine.GetGUIObjectByName("chatInput");
	let text = input.caption;
	if (!text.length)
		return;

	input.caption = "";

	if (executeNetworkCommand(text))
		return;

	Engine.SendNetworkChat(text);
}

function senderFont(text)
{
	return '[font="' + g_SenderFont + '"]' + text + '[/font]';
}

function systemMessage(message)
{
	return senderFont(sprintf(translate("== %(message)s"), { "message": message }));
}

function colorizePlayernameByGUID(guid, username = "")
{
	// TODO: Maybe the host should have the moderator-prefix?
	if (!username)
		username = g_PlayerAssignments[guid] ? escapeText(g_PlayerAssignments[guid].name) : translate("Unknown Player");
	let playerID = g_PlayerAssignments[guid] ? g_PlayerAssignments[guid].player : -1;

	let color = g_ColorRegular;
	if (playerID > 0)
	{
		color = g_GameAttributes.settings.PlayerData[playerID - 1].Color;

		// Enlighten playercolor to improve readability
		let [h, s, l] = rgbToHsl(color.r, color.g, color.b);
		let [r, g, b] = hslToRgb(h, s, Math.max(0.6, l));

		color = rgbToGuiColor({ "r": r, "g": g, "b": b });
	}

	return '[color="'+ color +'"]' + username + '[/color]';
}

function addChatMessage(msg)
{
	if (!g_FormatChatMessage[msg.type])
		return;

	if (msg.type == "chat")
	{
		let userName = g_PlayerAssignments[Engine.GetPlayerGUID()].name;

		if (userName != g_PlayerAssignments[msg.guid].name)
			notifyUser(userName, msg.text);
	}

	let user = colorizePlayernameByGUID(msg.guid || -1, msg.username || "");

	let text = g_FormatChatMessage[msg.type](msg, user);

	if (!text)
		return;

	if (Engine.ConfigDB_GetValue("user", "chat.timestamp") == "true")
		text = sprintf(translate("%(time)s %(message)s"), {
			"time": sprintf(translate("\\[%(time)s]"), {
				"time": Engine.FormatMillisecondsIntoDateStringLocal(new Date().getTime(), translate("HH:mm"))
			}),
			"message": text
		});

	g_ChatMessages.push(text);

	Engine.GetGUIObjectByName("chatText").caption = g_ChatMessages.join("\n");
}

function showMoreOptions(show)
{
	Engine.GetGUIObjectByName("moreOptionsFade").hidden = !show;
	Engine.GetGUIObjectByName("moreOptions").hidden = !show;
}

function resetCivilizations()
{
	for (let i in g_GameAttributes.settings.PlayerData)
		g_GameAttributes.settings.PlayerData[i].Civ = "random";

	updateGameAttributes();
}

function resetTeams()
{
	for (let i in g_GameAttributes.settings.PlayerData)
		g_GameAttributes.settings.PlayerData[i].Team = -1;

	updateGameAttributes();
}

function toggleReady()
{
	setReady((g_IsReady + 1) % 3, true);
}

function setReady(ready, sendMessage)
{
	g_IsReady = ready;

	if (sendMessage)
		Engine.SendNetworkReady(g_IsReady);

	updateGUIObjects();
}

function resetReadyData()
{
	if (g_GameStarted)
		return;

	if (g_ReadyChanged < 1)
		addChatMessage({ "type": "settings" });
	else if (g_ReadyChanged == 2 && !g_ReadyInit)
		return; // duplicate calls on init
	else
		g_ReadyInit = false;

	g_ReadyChanged = 2;
	if (!g_IsNetworked)
		g_IsReady = 2;
	else if (g_IsController)
	{
		Engine.ClearAllPlayerReady();
		setReady(2, true);
	}
	else if (g_IsReady != 2)
		setReady(0, false);
}

/**
 * Send a list of playernames and distinct between players and observers.
 * Don't send teams, AIs or anything else until the game was started.
 * The playerData format from g_GameAttributes is kept to reuse the GUI function presenting the data.
 */
function formatClientsForStanza()
{
	let connectedPlayers = 0;
	let playerData = [];

	for (let guid in g_PlayerAssignments)
	{
		let pData = { "Name": g_PlayerAssignments[guid].name };

		if (g_GameAttributes.settings.PlayerData[g_PlayerAssignments[guid].player - 1])
			++connectedPlayers;
		else
			pData.Team = "observer";

		playerData.push(pData);
	}

	return {
		"list": playerDataToStringifiedTeamList(playerData),
		"connectedPlayers": connectedPlayers
	};
}

/**
 * Send the relevant gamesettings to the lobbybot immediately.
 */
function sendRegisterGameStanzaImmediate()
{
	if (!g_IsController || !Engine.HasXmppClient())
		return;

	if (g_GameStanzaTimer != undefined)
	{
		clearTimeout(g_GameStanzaTimer);
		g_GameStanzaTimer = undefined;
	}

	let clients = formatClientsForStanza();

	let stanza = {
		"name": g_ServerName,
		"port": g_ServerPort,
		"hostUsername": g_Username,
		"mapName": g_GameAttributes.map,
		"niceMapName": getMapDisplayName(g_GameAttributes.map),
		"mapSize": g_GameAttributes.mapType == "random" ? g_GameAttributes.settings.Size : "Default",
		"mapType": g_GameAttributes.mapType,
		"victoryCondition": g_GameAttributes.settings.GameType,
		"nbp": clients.connectedPlayers,
		"maxnbp": g_GameAttributes.settings.PlayerData.length,
		"players": clients.list,
		"stunIP": g_StunEndpoint ? g_StunEndpoint.ip : "",
		"stunPort": g_StunEndpoint ? g_StunEndpoint.port : "",
	};

	// Only send the stanza if the relevant settings actually changed
	if (g_LastGameStanza && Object.keys(stanza).every(prop => g_LastGameStanza[prop] == stanza[prop]))
		return;

	g_LastGameStanza = stanza;
	Engine.SendRegisterGame(stanza);
}

/**
 * Send the relevant gamesettings to the lobbybot in a deferred manner.
 */
function sendRegisterGameStanza()
{
	if (!g_IsController || !Engine.HasXmppClient())
		return;

	if (g_GameStanzaTimer != undefined)
		clearTimeout(g_GameStanzaTimer);

	g_GameStanzaTimer = setTimeout(sendRegisterGameStanzaImmediate, g_GameStanzaTimeout * 1000);
}

function updateAutocompleteEntries()
{
	g_Autocomplete = [];

	for (let control of [g_Dropdowns, g_Checkboxes])
		for (let name in control)
			g_Autocomplete = g_Autocomplete.concat(control[name].title());

	for (let dropdown of [g_Dropdowns, g_PlayerDropdowns])
		for (let name in dropdown)
			if (dropdown[name].autocomplete)
				g_Autocomplete = g_Autocomplete.concat(dropdown[name].labels());
}

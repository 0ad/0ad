const MAX_HEADINGTITLE = 8;

// const for filtering long collective headings
const LONG_HEADING_WIDTH = 250;
// Vertical size of player box
const PLAYER_BOX_Y_SIZE = 30;
// Gap between players boxes
const PLAYER_BOX_GAP = 2;
// Alpha for player box
const PLAYER_BOX_ALPHA = " 32";
// Alpha for player color box
const PLAYER_COLOR_BOX_ALPHA = " 255";
// yStart value for spacing teams boxes (and noTeamsBox)
const TEAMS_BOX_Y_START = 65;
// Colors used for units and buildings
const TRAINED_COLOR = '[color="201 255 200"]';
const LOST_COLOR = '[color="255 213 213"]';
const KILLED_COLOR = '[color="196 198 255"]';

const BUILDINGS_TYPES = [ "total", "House", "Economic", "Outpost", "Military", "Fortress", "CivCentre", "Wonder" ];
const UNITS_TYPES = [ "total", "Infantry", "Worker", "Cavalry", "Champion", "Hero", "Ship", "Trader" ];
const RESOURCES_TYPES = [ "food", "wood", "stone", "metal" ];

// Colors used for gathered and traded resources
const INCOME_COLOR = '[color="201 255 200"]';
const OUTCOME_COLOR = '[color="255 213 213"]';

const DEFAULT_DECIMAL = "0.00";
const INFINITE_SYMBOL = "\u221E";
// Load data
var g_CivData = loadCivData();
var g_Teams = [];
// TODO set g_PlayerCount as playerCounters.length
var g_PlayerCount = 0;
// Count players without team (or all if teams are not displayed)
var g_WithoutTeam = 0;
var g_GameData;

/**
 * Select active panel
 * @param panelNumber Number of panel, which should get active state (integer)
 */
function selectPanel(panelNumber)
{
	var panelNames = [ 'scorePanel', 'buildingsPanel', 'unitsPanel', 'resourcesPanel', 'marketPanel', 'miscPanel' ];

	function adjustTabDividers(tabSize)
	{
		var leftSpacer = Engine.GetGUIObjectByName("tabDividerLeft");
		var rightSpacer = Engine.GetGUIObjectByName("tabDividerRight");
		leftSpacer.size = "20 " + leftSpacer.size.top + " " + (tabSize.left + 2) + " " + leftSpacer.size.bottom;
		rightSpacer.size = (tabSize.right - 2) + " " + rightSpacer.size.top + " 100%-20 " + rightSpacer.size.bottom;
	}

	for (var i = 0; i < panelNames.length; ++i)
	{
		Engine.GetGUIObjectByName(panelNames[i] + 'Button').sprite = "BackgroundTab";
	}

	Engine.GetGUIObjectByName(panelNames[panelNumber] + 'Button').sprite = "ForegroundTab";
	adjustTabDividers(Engine.GetGUIObjectByName(panelNames[panelNumber] + 'Button').size);

	updatePanelData(panelsData[panelNumber]);
}

function updatePanelData(panelInfo)
{
	resetGeneralPanel();
	resetDataHelpers();
	updateGeneralPanelHeadings(panelInfo.headings);
	updateGeneralPanelTitles(panelInfo.titleHeadings);
	var rowPlayerObjectWidth = updateGeneralPanelCounter(panelInfo.counters);
	updateGeneralPanelTeams();

	var playerBoxesCounts = [ ];
	for (var i = 0; i < g_PlayerCount; ++i)
	{
		var playerState = g_GameData.playerStates[i+1];

		if (!playerBoxesCounts[playerState.team+1])
			playerBoxesCounts[playerState.team+1] = 1;
		else
			playerBoxesCounts[playerState.team+1] += 1;

		var positionObject = playerBoxesCounts[playerState.team+1] - 1;
		var rowPlayer = "playerBox[" + positionObject + "]";
		var playerNameColumn = "playerName[" + positionObject + "]";
		var playerColorBoxColumn = "playerColorBox[" + positionObject + "]";
		var playerCivicBoxColumn = "civIcon[" + positionObject + "]";
		var playerCounterValue = "valueData[" + positionObject + "]";
		if (playerState.team != -1)
		{
			rowPlayer = "playerBoxt[" + playerState.team + "][" + positionObject + "]";
			playerNameColumn = "playerNamet[" + playerState.team + "][" + positionObject + "]";
			playerColorBoxColumn = "playerColorBoxt[" + playerState.team + "][" + positionObject + "]";
			playerCivicBoxColumn = "civIcont[" + playerState.team + "][" + positionObject + "]";
			playerCounterValue = "valueDataTeam[" + playerState.team + "][" + positionObject + "]";
		}

		var colorString = "color: "
				+ Math.floor(playerState.color.r * 255) + " "
				+ Math.floor(playerState.color.g * 255) + " "
				+ Math.floor(playerState.color.b * 255);

		var rowPlayerObject = Engine.GetGUIObjectByName(rowPlayer);
		rowPlayerObject.hidden = false;
		rowPlayerObject.sprite = colorString + PLAYER_BOX_ALPHA;
		var boxSize = rowPlayerObject.size;
		boxSize.right = rowPlayerObjectWidth;
		rowPlayerObject.size = boxSize;

		var playerColorBox = Engine.GetGUIObjectByName(playerColorBoxColumn);
		playerColorBox.sprite = colorString + PLAYER_COLOR_BOX_ALPHA;

		Engine.GetGUIObjectByName(playerNameColumn).caption = g_GameData.players[i+1].name;

		var civIcon = Engine.GetGUIObjectByName(playerCivicBoxColumn);
		civIcon.sprite = "stretched:"+g_CivData[playerState.civ].Emblem;
		civIcon.tooltip = g_CivData[playerState.civ].Name;

		// Update counters
		updateCountersPlayer(playerState, panelInfo.counters, playerCounterValue);
	}
	// Update team counters
	var teamCounterFn = panelInfo.teamCounterFn;
	if (g_Teams && teamCounterFn)
		teamCounterFn(panelInfo.counters);
}

function init(data)
{
	updateObjectPlayerPosition();
	g_GameData = data;

	var mapSize = data.mapSettings.Size ? g_Settings.MapSizes.find(size => size.Tiles == data.mapSettings.Size) : undefined;
	var mapType = g_Settings.MapTypes.find(mapType => mapType.Name == data.mapSettings.mapType);
	Engine.GetGUIObjectByName("timeElapsed").caption = sprintf(translate("Game time elapsed: %(time)s"), { "time": timeToString(data.timeElapsed) });
	Engine.GetGUIObjectByName("summaryText").caption = data.gameResult;
	Engine.GetGUIObjectByName("mapName").caption = sprintf(translate("%(mapName)s - %(mapType)s"), { "mapName": translate(data.mapSettings.Name), "mapType": mapSize ? mapSize.LongName : (mapType ? mapType.Title : "") });

	// Panels
	g_PlayerCount = data.playerStates.length - 1;

	if (data.mapSettings.LockTeams)	// teams ARE locked
	{
		// Count teams
		for(var t = 0; t < g_PlayerCount; ++t)
		{
			let playerTeam = data.playerStates[t+1].team;
			if (g_Teams[playerTeam])
				g_Teams[playerTeam]++;
			else
				g_Teams[playerTeam] = 1;
		}

		if (g_Teams.length == g_PlayerCount)
			g_Teams = false;	// Each player has his own team. Displaying teams makes no sense.
	}
	else				// teams are NOT locked
		g_Teams = false;

	// Erase teams data if teams are not displayed
	if (!g_Teams)
	{
		for(var p = 0; p < g_PlayerCount; ++p)
			data.playerStates[p+1].team = -1;
	}

	g_WithoutTeam = g_PlayerCount;
	if (g_Teams)
	{
		// Count players without team (or all if teams are not displayed)
		for (var i = 0; i < g_Teams.length; ++i)
			g_WithoutTeam -= g_Teams[i] ? g_Teams[i] : 0;
	}

	selectPanel(0);
}

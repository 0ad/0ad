const g_CivData = loadCivData(false, false);

var g_ScorePanelsData;

var g_MaxHeadingTitle = 9;
var g_LongHeadingWidth = 250;
var g_PlayerBoxYSize = 40;
var g_PlayerBoxGap = 2;
var g_PlayerBoxAlpha = 50;
var g_TeamsBoxYStart = 40;

var g_TypeColors = {
	"blue": "196 198 255",
	"green": "201 255 200",
	"red": "255 213 213",
	"yellow": "255 255 157"
};

/**
 * Colors, captions and format used for units, buildings, etc. types
 */
var g_SummaryTypes = {
	"percent": {
		"color": "",
		"caption": "%",
		"postfix": "%"
	},
	"trained": {
		"color": g_TypeColors.green,
		"caption": translate("Trained"),
		"postfix": " / "
	},
	"constructed": {
		"color": g_TypeColors.green,
		"caption": translate("Constructed"),
		"postfix": " / "
	},
	"gathered": {
		"color": g_TypeColors.green,
		"caption": translate("Gathered"),
		"postfix": " / "
	},
	"sent": {
		"color": g_TypeColors.green,
		"caption": translate("Sent"),
		"postfix": " / "
	},
	"bought": {
		"color": g_TypeColors.green,
		"caption": translate("Bought"),
		"postfix": " / "
	},
	"income": {
		"color": g_TypeColors.green,
		"caption": translate("Income"),
		"postfix": " / "
	},
	"captured": {
		"color": g_TypeColors.yellow,
		"caption": translate("Captured"),
		"postfix": " / "
	},
	"succeeded": {
		"color": g_TypeColors.green,
		"caption": translate("Succeeded"),
		"postfix": " / "
	},
	"destroyed": {
		"color": g_TypeColors.blue,
		"caption": translate("Destroyed"),
		"postfix": "\n"
	},
	"killed": {
		"color": g_TypeColors.blue,
		"caption": translate("Killed"),
		"postfix": "\n"
	},
	"lost": {
		"color": g_TypeColors.red,
		"caption": translate("Lost"),
		"postfix": ""
	},
	"used": {
		"color": g_TypeColors.red,
		"caption": translate("Used"),
		"postfix": ""
	},
	"received": {
		"color": g_TypeColors.red,
		"caption": translate("Received"),
		"postfix": ""
	},
	"sold": {
		"color": g_TypeColors.red,
		"caption": translate("Sold"),
		"postfix": ""
	},
	"outcome": {
		"color": g_TypeColors.red,
		"caption": translate("Outcome"),
		"postfix": ""
	},
	"failed": {
		"color": g_TypeColors.red,
		"caption": translate("Failed"),
		"postfix": ""
	}
};

// Translation: Unicode encoded infinity symbol indicating a division by zero in the summary screen.
var g_InfinitySymbol = translate("\u221E");

var g_Teams = [];

// TODO set g_PlayerCount as playerCounters.length
var g_PlayerCount = 0;

var g_GameData;
var g_ResourceData = new Resources();

/**
 * Selected chart indexes.
 */
var g_SelectedChart = {
	"category": [0, 0],
	"value": [0, 1],
	"type": [0, 0]
};

/**
 * Array of the panel button names.
 */
var g_PanelButtons = [];

/**
 * Remember the name of the currently opened view panel.
 */
var g_SelectedPanel = "";

function init(data)
{
	// Fill globals
	g_GameData = data;
	g_ScorePanelsData = getScorePanelsData();
	g_PanelButtons = Object.keys(g_ScorePanelsData).concat(["charts"]).map(panel => panel + "PanelButton");

	g_SelectedPanel = g_PanelButtons[0];
	if (data && data.selectedData)
	{
		g_SelectedPanel = data.selectedData.panel;
		g_SelectedChart = data.selectedData.charts;
	}

	initTeamData();
	calculateTeamCounterDataHelper();

	// Output globals
	initGUIWindow();
	initPlayerBoxPositions();
	initGUICharts();
	initGUILabels();
	initGUIButtons();

	selectPanel(Engine.GetGUIObjectByName(g_SelectedPanel));
	for (let button of g_PanelButtons)
	{
		let tab = Engine.GetGUIObjectByName(button);
		tab.onMouseWheelUp = () => selectNextTab(1);
		tab.onMouseWheelDown = () => selectNextTab(-1);
	}
}

/**
 * Sets the style and title of the page.
 */
function initGUIWindow()
{
	let summaryWindow = Engine.GetGUIObjectByName("summaryWindow");
	summaryWindow.sprite = g_GameData.gui.dialog ? "ModernDialog" : "ModernWindow";
	summaryWindow.size = g_GameData.gui.dialog ? "16 24 100%-16 100%-24" : "0 0 100% 100%";
	Engine.GetGUIObjectByName("summaryWindowTitle").size = g_GameData.gui.dialog ? "50%-128 -16 50%+128 16" : "50%-128 4 50%+128 36";
}

/**
 * Show next/previous panel.
 * @param direction - 1/-1 forward, backward panel.
 */
function selectNextTab(direction)
{
	selectPanel(Engine.GetGUIObjectByName(g_PanelButtons[
		(g_PanelButtons.indexOf(g_SelectedPanel) + direction + g_PanelButtons.length) % g_PanelButtons.length]));
}

function selectPanel(panel)
{
	// TODO: move panel buttons to a custom parent object

	for (let button of Engine.GetGUIObjectByName("summaryWindow").children)
		if (button.name.endsWith("PanelButton"))
			button.sprite = "ModernTabHorizontalBackground";

	panel.sprite = "ModernTabHorizontalForeground";

	adjustTabDividers(panel.size);

	let generalPanel = Engine.GetGUIObjectByName("generalPanel");
	let chartsPanel = Engine.GetGUIObjectByName("chartsPanel");
	let chartsHidden = panel.name != "chartsPanelButton";
	generalPanel.hidden = !chartsHidden;
	chartsPanel.hidden = chartsHidden;
	if (chartsHidden)
		updatePanelData(g_ScorePanelsData[panel.name.substr(0, panel.name.length - "PanelButton".length)]);
	else
		[0, 1].forEach(updateCategoryDropdown);

	g_SelectedPanel = panel.name;
}

function initGUICharts()
{
	let player_colors = [];
	for (let i = 1; i <= g_PlayerCount; ++i)
	{
		let playerState = g_GameData.sim.playerStates[i];
		player_colors.push(
			Math.floor(playerState.color.r * 255) + " " +
			Math.floor(playerState.color.g * 255) + " " +
			Math.floor(playerState.color.b * 255)
		);
	}

	for (let i = 0; i < 2; ++i)
		Engine.GetGUIObjectByName("chart[" + i + "]").series_color = player_colors;

	let chartLegend = Engine.GetGUIObjectByName("chartLegend");
	chartLegend.caption = g_GameData.sim.playerStates.slice(1).map(
		(state, index) => coloredText("■", player_colors[index]) + " " + state.name
	).join("   ");

	let chart1Part = Engine.GetGUIObjectByName("chart[1]Part");
	let chart1PartSize = chart1Part.size;
	chart1PartSize.rright += 50;
	chart1PartSize.rleft += 50;
	chart1PartSize.right -= 5;
	chart1PartSize.left -= 5;
	chart1Part.size = chart1PartSize;
}

function resizeDropdown(dropdown)
{
	let size = dropdown.size;
	size.bottom = dropdown.size.top +
		(Engine.GetTextWidth(dropdown.font, dropdown.list[dropdown.selected]) >
			dropdown.size.right - dropdown.size.left - 32 ? 42 : 27);
	dropdown.size = size;
}

function updateCategoryDropdown(number)
{
	let chartCategory = Engine.GetGUIObjectByName("chart[" + number + "]CategorySelection");
	chartCategory.list_data = Object.keys(g_ScorePanelsData);
	chartCategory.list = Object.keys(g_ScorePanelsData).map(panel => g_ScorePanelsData[panel].caption);
	chartCategory.onSelectionChange = function() {
		if (!this.list_data[this.selected])
			return;
		if (g_SelectedChart.category[number] != this.selected)
		{
			g_SelectedChart.category[number] = this.selected;
			g_SelectedChart.value[number] = 0;
			g_SelectedChart.type[number] = 0;
		}

		resizeDropdown(this);
		updateValueDropdown(number, this.list_data[this.selected]);
	};
	chartCategory.selected = g_SelectedChart.category[number];
}

function updateValueDropdown(number, category)
{
	let chartValue = Engine.GetGUIObjectByName("chart[" + number + "]ValueSelection");
	let list = g_ScorePanelsData[category].headings.map(heading => heading.caption);
	list.shift();
	chartValue.list = list;
	let list_data = g_ScorePanelsData[category].headings.map(heading => heading.identifier);
	list_data.shift();
	chartValue.list_data = list_data;
	chartValue.onSelectionChange = function() {
		if (!this.list_data[this.selected])
			return;
		if (g_SelectedChart.value[number] != this.selected)
		{
			g_SelectedChart.value[number] = this.selected;
			g_SelectedChart.type[number] = 0;
		}

		resizeDropdown(this);
		updateTypeDropdown(number, category, this.list_data[this.selected], this.selected);
	};
	chartValue.selected = g_SelectedChart.value[number];
}

function updateTypeDropdown(number, category, item, itemNumber)
{
	let testValue = g_ScorePanelsData[category].counters[itemNumber].fn(g_GameData.sim.playerStates[1], 0, item);
	let hide = !g_ScorePanelsData[category].counters[itemNumber].fn ||
		typeof testValue != "object" || Object.keys(testValue).length < 2;
	Engine.GetGUIObjectByName("chart[" + number + "]TypeLabel").hidden = hide;
	let chartType = Engine.GetGUIObjectByName("chart[" + number + "]TypeSelection");
	chartType.hidden = hide;
	if (hide)
	{
		updateChart(number, category, item, itemNumber, Object.keys(testValue)[0] || undefined);
		return;
	}

	chartType.list = Object.keys(testValue).map(type => g_SummaryTypes[type].caption);
	chartType.list_data = Object.keys(testValue);
	chartType.onSelectionChange = function() {
		if (!this.list_data[this.selected])
			return;
		g_SelectedChart.type[number] = this.selected;
		resizeDropdown(this);
		updateChart(number, category, item, itemNumber, this.list_data[this.selected]);
	};
	chartType.selected = g_SelectedChart.type[number];
}

function updateChart(number, category, item, itemNumber, type)
{
	if (!g_ScorePanelsData[category].counters[itemNumber].fn)
		return;
	let chart = Engine.GetGUIObjectByName("chart[" + number + "]");
	chart.format_y = g_ScorePanelsData[category].headings[itemNumber + 1].format || "INTEGER";
	Engine.GetGUIObjectByName("chart[" + number + "]XAxisLabel").caption = translate("Time elapsed");
	let series = [];
	for (let j = 1; j <= g_PlayerCount; ++j)
	{
		let playerState = g_GameData.sim.playerStates[j];
		let data = [];
		for (let index in playerState.sequences.time)
		{
			let value = g_ScorePanelsData[category].counters[itemNumber].fn(playerState, index, item);
			if (type)
				value = value[type];
			data.push([playerState.sequences.time[index], value]);
		}
		series.push(data);
	}
	chart.series = series;
}

function adjustTabDividers(tabSize)
{
	let leftSpacer = Engine.GetGUIObjectByName("tabDividerLeft");
	let rightSpacer = Engine.GetGUIObjectByName("tabDividerRight");

	leftSpacer.size = [
		20,
		leftSpacer.size.top,
		tabSize.left + 2,
		leftSpacer.size.bottom
	].join(" ");

	rightSpacer.size = [
		tabSize.right - 2,
		rightSpacer.size.top,
		"100%-20",
		rightSpacer.size.bottom
	].join(" ");
}

function updatePanelData(panelInfo)
{
	resetGeneralPanel();
	updateGeneralPanelHeadings(panelInfo.headings);
	updateGeneralPanelTitles(panelInfo.titleHeadings);
	let rowPlayerObjectWidth = updateGeneralPanelCounter(panelInfo.counters);
	updateGeneralPanelTeams();

	let index = g_GameData.sim.playerStates[1].sequences.time.length - 1;
	let playerBoxesCounts = [];
	for (let i = 0; i < g_PlayerCount; ++i)
	{
		let playerState = g_GameData.sim.playerStates[i + 1];

		if (!playerBoxesCounts[playerState.team + 1])
			playerBoxesCounts[playerState.team + 1] = 1;
		else
			playerBoxesCounts[playerState.team + 1] += 1;

		let positionObject = playerBoxesCounts[playerState.team + 1] - 1;
		let rowPlayer = "playerBox[" + positionObject + "]";
		let playerOutcome = "playerOutcome[" + positionObject + "]";
		let playerNameColumn = "playerName[" + positionObject + "]";
		let playerCivicBoxColumn = "civIcon[" + positionObject + "]";
		let playerCounterValue = "valueData[" + positionObject + "]";

		if (playerState.team != -1)
		{
			rowPlayer = "playerBoxt[" + playerState.team + "][" + positionObject + "]";
			playerOutcome = "playerOutcomet[" + playerState.team + "][" + positionObject + "]";
			playerNameColumn = "playerNamet[" + playerState.team + "][" + positionObject + "]";
			playerCivicBoxColumn = "civIcont[" + playerState.team + "][" + positionObject + "]";
			playerCounterValue = "valueDataTeam[" + playerState.team + "][" + positionObject + "]";
		}

		let colorString = "color: " +
			Math.floor(playerState.color.r * 255) + " " +
			Math.floor(playerState.color.g * 255) + " " +
			Math.floor(playerState.color.b * 255);

		let rowPlayerObject = Engine.GetGUIObjectByName(rowPlayer);
		rowPlayerObject.hidden = false;
		rowPlayerObject.sprite = colorString + " " + g_PlayerBoxAlpha;

		let boxSize = rowPlayerObject.size;
		boxSize.right = rowPlayerObjectWidth;
		rowPlayerObject.size = boxSize;

		setOutcomeIcon(playerState.state, playerOutcome);

		playerNameColumn = Engine.GetGUIObjectByName(playerNameColumn);
		playerNameColumn.caption = g_GameData.sim.playerStates[i + 1].name;
		playerNameColumn.tooltip = translateAISettings(g_GameData.sim.mapSettings.PlayerData[i + 1]);

		let civIcon = Engine.GetGUIObjectByName(playerCivicBoxColumn);
		civIcon.sprite = "stretched:" + g_CivData[playerState.civ].Emblem;
		civIcon.tooltip = g_CivData[playerState.civ].Name;

		updateCountersPlayer(playerState, panelInfo.counters, panelInfo.headings, playerCounterValue, index);
	}

	let teamCounterFn = panelInfo.teamCounterFn;
	if (g_Teams && teamCounterFn)
		updateCountersTeam(teamCounterFn, panelInfo.counters, panelInfo.headings, index);
}

function continueButton()
{
	let summarySelectedData = {
		"panel": g_SelectedPanel,
		"charts": g_SelectedChart
	};
	if (g_GameData.gui.isInGame)
		Engine.PopGuiPageCB({
			"explicitResume": 0,
			"summarySelectedData": summarySelectedData
		});
	else if (g_GameData.gui.dialog)
		Engine.PopGuiPage();
	else if (Engine.HasXmppClient())
		Engine.SwitchGuiPage("page_lobby.xml", { "dialog": false });
	else if (g_GameData.gui.isReplay)
		Engine.SwitchGuiPage("page_replaymenu.xml", {
			"replaySelectionData": g_GameData.gui.replaySelectionData,
			"summarySelectedData": summarySelectedData
		});
	else
		Engine.SwitchGuiPage("page_pregame.xml");
}

function startReplay()
{
	if (!Engine.StartVisualReplay(g_GameData.gui.replayDirectory))
	{
		warn("Replay file not found!");
		return;
	}

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": Engine.GetReplayAttributes(g_GameData.gui.replayDirectory),
		"playerAssignments": {
			"local": {
				"name": singleplayerName(),
				"player": -1
			}
		},
		"savedGUIData": "",
		"isReplay": true,
		"replaySelectionData": g_GameData.gui.replaySelectionData
	});
}

function initGUILabels()
{
	let assignedState = g_GameData.sim.playerStates[g_GameData.gui.assignedPlayer || -1];

	Engine.GetGUIObjectByName("summaryText").caption =
		g_GameData.gui.isInGame ?
			translate("Current Scores") :
		g_GameData.gui.isReplay ?
			translate("Scores at the end of the game.") :
		g_GameData.gui.disconnected ?
			translate("You have been disconnected.") :
		!assignedState ?
			translate("You have left the game.") :
		assignedState.state == "won" ?
			translate("You have won the battle!") :
		assignedState.state == "defeated" ?
			translate("You have been defeated…") :
			translate("You have abandoned the game.");

	Engine.GetGUIObjectByName("timeElapsed").caption = sprintf(
		translate("Game time elapsed: %(time)s"), {
			"time": timeToString(g_GameData.sim.timeElapsed)
	});

	let mapType = g_Settings.MapTypes.find(type => type.Name == g_GameData.sim.mapSettings.mapType);
	let mapSize = g_Settings.MapSizes.find(size => size.Tiles == g_GameData.sim.mapSettings.Size || 0);

	Engine.GetGUIObjectByName("mapName").caption = sprintf(
		translate("%(mapName)s - %(mapType)s"), {
			"mapName": translate(g_GameData.sim.mapSettings.Name),
			"mapType": mapSize ? mapSize.Name : (mapType ? mapType.Title : "")
		});
}

function initGUIButtons()
{
	let replayButton = Engine.GetGUIObjectByName("replayButton");
	replayButton.hidden = g_GameData.gui.isInGame || !g_GameData.gui.replayDirectory;

	let lobbyButton = Engine.GetGUIObjectByName("lobbyButton");
	lobbyButton.tooltip = colorizeHotkey(translate("%(hotkey)s: Toggle the multiplayer lobby in a dialog window."), "lobby");
	lobbyButton.hidden = g_GameData.gui.isInGame || !Engine.HasXmppClient();

	// Right-align lobby button
	let lobbyButtonSize = lobbyButton.size;
	let lobbyButtonWidth = lobbyButtonSize.right - lobbyButtonSize.left;
	lobbyButtonSize.right = (replayButton.hidden ? Engine.GetGUIObjectByName("continueButton").size.left : replayButton.size.left) - 10;
	lobbyButtonSize.left = lobbyButtonSize.right - lobbyButtonWidth;
	lobbyButton.size = lobbyButtonSize;
}

function initTeamData()
{
	// Panels
	g_PlayerCount = g_GameData.sim.playerStates.length - 1;

	if (g_GameData.sim.mapSettings.LockTeams)
	{
		// Count teams
		for (let player = 1; player <= g_PlayerCount; ++player)
		{
			let playerTeam = g_GameData.sim.playerStates[player].team;
			if (!g_Teams[playerTeam])
				g_Teams[playerTeam] = [];
			g_Teams[playerTeam].push(player);
		}

		if (g_Teams.every(team => team && team.length < 2))
			g_Teams = false;	// Each player has his own team. Displaying teams makes no sense.
	}
	else
		g_Teams = false;

	// Erase teams data if teams are not displayed
	if (!g_Teams)
		for (let p = 0; p < g_PlayerCount; ++p)
			g_GameData.sim.playerStates[p+1].team = -1;
}

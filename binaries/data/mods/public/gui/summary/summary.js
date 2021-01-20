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
 * Colors, captions and format used for units, structures, etc. types
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
	"count": {
		"caption": translate("Count"),
		"hideInSummary": true
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
	"population": {
		"color": g_TypeColors.red,
		"caption": translate("Population"),
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

var g_PlayerCount;

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

function init(data)
{
	initSummaryData(data);
	initGUISummary();
}

function initSummaryData(data)
{
	g_GameData = data;
	g_ScorePanelsData = getScorePanelsData();

	let teamCharts = false;
	if (data && data.gui && data.gui.summarySelection)
	{
		g_TabCategorySelected = data.gui.summarySelection.panel;
		g_SelectedChart = data.gui.summarySelection.charts;
		teamCharts = data.gui.summarySelection.teamCharts;
	}
	Engine.GetGUIObjectByName("toggleTeamBox").checked = g_Teams && teamCharts;

	initTeamData();
	calculateTeamCounterDataHelper();
}

function initGUISummary()
{
	initGUIWindow();
	initPlayerBoxPositions();
	initGUICharts();
	initGUILabels();
	initGUIButtons();
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

function selectPanelGUI(panel)
{
	adjustTabDividers(Engine.GetGUIObjectByName("tabButton[" + panel + "]").size);

	let generalPanel = Engine.GetGUIObjectByName("generalPanel");
	let chartsPanel = Engine.GetGUIObjectByName("chartsPanel");

	// We assume all scorePanels come before the charts.
	let chartsHidden = panel < g_ScorePanelsData.length;
	generalPanel.hidden = !chartsHidden;
	chartsPanel.hidden = chartsHidden;
	if (chartsHidden)
		updatePanelData(g_ScorePanelsData[panel]);
	else
		[0, 1].forEach(updateCategoryDropdown);
}

function constructPlayersWithColor(color, playerListing)
{
	return sprintf(translateWithContext("Player listing with color indicator",
		"%(colorIndicator)s %(playerListing)s"),
	{
		"colorIndicator": setStringTags(translateWithContext(
			"Charts player color indicator", "■"), { "color": color }),
		"playerListing": playerListing
	});
}

function updateChartColorAndLegend()
{
	let playerColors = [];
	for (let i = 1; i <= g_PlayerCount; ++i)
	{
		let playerState = g_GameData.sim.playerStates[i];
		playerColors.push(
			Math.floor(playerState.color.r * 255) + " " +
			Math.floor(playerState.color.g * 255) + " " +
			Math.floor(playerState.color.b * 255)
		);
	}

	for (let i = 0; i < 2; ++i)
		Engine.GetGUIObjectByName("chart[" + i + "]").series_color =
			Engine.GetGUIObjectByName("toggleTeamBox").checked ?
				g_Teams.filter(el => el !== null).map(players => playerColors[players[0] - 1]) :
				playerColors;

	let chartLegend = Engine.GetGUIObjectByName("chartLegend");
	chartLegend.caption = (Engine.GetGUIObjectByName("toggleTeamBox").checked ?
		g_Teams.filter(el => el !== null).map(players =>
			constructPlayersWithColor(playerColors[players[0] - 1],	players.map(player =>
				g_GameData.sim.playerStates[player].name
			).join(translateWithContext("Player listing", ", ")))
		) :
		g_GameData.sim.playerStates.slice(1).map((state, index) =>
			constructPlayersWithColor(playerColors[index], state.name))
	).join("   ");
}

function initGUICharts()
{
	updateChartColorAndLegend();
	let chart1Part = Engine.GetGUIObjectByName("chart[1]Part");
	let chart1PartSize = chart1Part.size;
	chart1PartSize.rright += 50;
	chart1PartSize.rleft += 50;
	chart1PartSize.right -= 5;
	chart1PartSize.left -= 5;
	chart1Part.size = chart1PartSize;
	Engine.GetGUIObjectByName("toggleTeam").hidden = !g_Teams;
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
	chartCategory.list_data = g_ScorePanelsData.map((panel, idx) => idx);
	chartCategory.list = g_ScorePanelsData.map(panel => panel.label);
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
	if (Engine.GetGUIObjectByName("toggleTeamBox").checked)
		for (let team in g_Teams)
		{
			let data = [];
			for (let index in g_GameData.sim.playerStates[1].sequences.time)
			{
				let value = g_ScorePanelsData[category].teamCounterFn(team, index, item,
					g_ScorePanelsData[category].counters, g_ScorePanelsData[category].headings);
				if (type)
					value = value[type];
				data.push([g_GameData.sim.playerStates[1].sequences.time[index], value]);
			}
			series.push(data);
		}
	else
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
	let tabButtonsLeft = Engine.GetGUIObjectByName("tabButtonsFrame").size.left;

	let leftSpacer = Engine.GetGUIObjectByName("tabDividerLeft");
	let leftSpacerSize = leftSpacer.size;
	leftSpacerSize.right = tabSize.left + tabButtonsLeft + 2;
	leftSpacer.size = leftSpacerSize;

	let rightSpacer = Engine.GetGUIObjectByName("tabDividerRight");
	let rightSpacerSize = rightSpacer.size;
	rightSpacerSize.left = tabSize.right + tabButtonsLeft - 2;
	rightSpacer.size = rightSpacerSize;
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

		setOutcomeIcon(playerState.state, Engine.GetGUIObjectByName(playerOutcome));

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
	let summarySelection = {
		"panel": g_TabCategorySelected,
		"charts": g_SelectedChart,
		"teamCharts": Engine.GetGUIObjectByName("toggleTeamBox").checked
	};
	if (g_GameData.gui.isInGame)
		Engine.PopGuiPage({
			"summarySelection": summarySelection
		});
	else if (g_GameData.gui.dialog)
		Engine.PopGuiPage();
	else if (Engine.HasXmppClient())
		Engine.SwitchGuiPage("page_lobby.xml", { "dialog": false });
	else if (g_GameData.gui.isReplay)
		Engine.SwitchGuiPage("page_replaymenu.xml", {
			"replaySelectionData": g_GameData.gui.replaySelectionData,
			"summarySelection": summarySelection
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

	let allPanelsData = g_ScorePanelsData.concat(g_ChartPanelsData);
	for (let tab in allPanelsData)
		allPanelsData[tab].tooltip =
			sprintf(translate("Toggle the %(name)s summary tab."), { "name": allPanelsData[tab].label }) +
			colorizeHotkey("\n" + translate("Use %(hotkey)s to move a summary tab right."), "tab.next") +
			colorizeHotkey("\n" + translate("Use %(hotkey)s to move a summary tab left."), "tab.prev");

	placeTabButtons(
		allPanelsData,
		true,
		g_TabButtonWidth,
		g_TabButtonDist,
		selectPanel,
		selectPanelGUI);
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

var g_ScorePanelsData = {
	"score": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Economy score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Military score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Exploration score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Total score"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [],
		"counters": [
			{ "width": 100, "fn": calculateEconomyScore, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateMilitaryScore, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateExplorationScore, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateScoreTotal, "verticalOffset": 12 }
		],
		"teamCounterFn": calculateScoreTeam
	},
	"buildings": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Total"), "yStart": 34, "width": 105 },
			{ "caption": translate("Houses"), "yStart": 34, "width": 85 },
			{ "caption": translate("Economic"), "yStart": 34, "width": 85 },
			{ "caption": translate("Outposts"), "yStart": 34, "width": 85 },
			{ "caption": translate("Military"), "yStart": 34, "width": 85 },
			{ "caption": translate("Fortresses"), "yStart": 34, "width": 85 },
			{ "caption": translate("Civ centers"), "yStart": 34, "width": 85 },
			{ "caption": translate("Wonders"), "yStart": 34, "width": 85 }
		],
		"titleHeadings": [
			{
				"caption": sprintf(translate("Buildings Statistics (%(constructed)s / %(destroyed)s / %(captured)s / %(lost)s)"),
					{
						"constructed": g_TrainedColor + translate("Constructed") + '[/color]',
						"destroyed": g_KilledColor + translate("Destroyed") + '[/color]',
						"captured": g_CapturedColor + translate("Captured") + '[/color]',
						"lost": g_LostColor + translate("Lost") + '[/color]'
					}),
				"yStart": 16,
				"width": (85 * 7 + 105)
			},	// width = 700
		],
		"counters": [
			{ "width": 105, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateBuildings, "verticalOffset": 3 }
		],
		"teamCounterFn": calculateBuildingsTeam
	},
	"units": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Total"), "yStart": 34, "width": 105 },
			{ "caption": translate("Infantry"), "yStart": 34, "width": 85 },
			{ "caption": translate("Worker"), "yStart": 34, "width": 85 },
			{ "caption": translate("Cavalry"), "yStart": 34, "width": 85 },
			{ "caption": translate("Champion"), "yStart": 34, "width": 85 },
			{ "caption": translate("Heroes"), "yStart": 34, "width": 85 },
			{ "caption": translate("Siege"), "yStart": 34, "width": 85 },
			{ "caption": translate("Navy"), "yStart": 34, "width": 85 },
			{ "caption": translate("Traders"), "yStart": 34, "width": 85 }
		],
		"titleHeadings": [
			{
				"caption": sprintf(translate("Units Statistics (%(trained)s / %(killed)s / %(captured)s / %(lost)s)"),
					{
						"trained": g_TrainedColor + translate("Trained") + '[/color]',
						"killed": g_KilledColor + translate("Killed") + '[/color]',
						"captured": g_CapturedColor + translate("Captured") + '[/color]',
						"lost": g_LostColor + translate("Lost") + '[/color]'
					}),
				"yStart": 16,
				"width": (100 * 7 + 120)
			},	// width = 820
		],
		"counters": [
			{ "width": 105, "fn": calculateUnitsWithCaptured, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 105, "fn": calculateUnitsWithCaptured, "verticalOffset": 3 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 },
			{ "width": 85, "fn": calculateUnits, "verticalOffset": 12 }
		],
		"teamCounterFn": calculateUnitsTeam
	},
	"resources": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			...g_ResourceData.GetResources().map(res => ({
				"caption": translateWithContext("firstWord", res.name),
				"yStart": 34,
				"width": 100
			})),
			{ "caption": translate("Total"), "yStart": 34, "width": 110 },
			{
				"caption": sprintf(translate("Tributes \n(%(sent)s / %(received)s)"),
					{
						"sent": g_IncomeColor + translate("Sent") + '[/color]',
						"received": g_OutcomeColor + translate("Received") + '[/color]'
					}),
				"yStart": 16,
				"width": 121
			},
			{ "caption": translate("Treasures collected"), "yStart": 16, "width": 100 },
			{ "caption": translate("Loot"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [
			{
				"caption": sprintf(translate("Resource Statistics (%(gathered)s / %(used)s)"),
					{
						"gathered": g_IncomeColor + translate("Gathered") + '[/color]',
						"used": g_OutcomeColor + translate("Used") + '[/color]'
					}),
				"yStart": 16,
				"width": 100 * g_ResourceData.GetCodes().length + 110
			},
		],
		"counters": [
			...g_ResourceData.GetCodes().map(code => ({
				"fn": calculateResources,
				"verticalOffset": 12,
				"width": 100
			})),
			{ "width": 110, "fn": calculateTotalResources, "verticalOffset": 12 },
			{ "width": 121, "fn": calculateTributeSent, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateTreasureCollected, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateLootCollected, "verticalOffset": 12 }
		],
		"teamCounterFn": calculateResourcesTeam
	},
	"market": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			...g_ResourceData.GetResources().map(res => {
				return {
					"caption":
						// Translation: use %(resourceWithinSentence)s if needed
						sprintf(translate("%(resourceFirstWord)s exchanged"), {
							"resourceFirstWord": translateWithContext("firstWord", res.name),
							"resourceWithinSentence": translateWithContext("withinSentence", res.name)
						}),
					"yStart": 16,
					"width": 100
				};
			}),
			{ "caption": translate("Barter efficiency"), "yStart": 16, "width": 100 },
			{ "caption": translate("Trade income"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [],
		"counters": [
			...g_ResourceData.GetCodes().map(code => ({
				"width": 100,
				"fn": calculateResourceExchanged,
				"verticalOffset": 12
			})),
			{ "width": 100, "fn": calculateBarterEfficiency, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateTradeIncome, "verticalOffset": 12 }
		],
		"teamCounterFn": calculateMarketTeam
	},
	"misc": {
		"headings": [
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Vegetarian\nratio"), "yStart": 16, "width": 100 },
			{ "caption": translate("Feminization"), "yStart": 16, "width": 100 },
			{ "caption": translate("Kill / Death\nratio"), "yStart": 16, "width": 100 },
			{ "caption": translate("Map\nexploration"), "yStart": 16, "width": 100 },
			{ "caption": translate("At peak"), "yStart": 34, "width": 100 },
			{ "caption": translate("At finish"), "yStart": 34, "width": 100 }
		],
		"titleHeadings": [
			{ "caption": translate("Map control"), "xOffset": 400, "yStart": 16, "width": 200 }
		],
		"counters": [
			{ "width": 100, "fn": calculateVegetarianRatio, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateFeminization, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateKillDeathRatio, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateMapExploration, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateMapPeakControl, "verticalOffset": 12 },
			{ "width": 100, "fn": calculateMapFinalControl, "verticalOffset": 12 }
		],
		"teamCounterFn": calculateMiscellaneous
	}
};

function resetGeneralPanel()
{
	for (let h = 0; h < g_MaxHeadingTitle; ++h)
	{
		Engine.GetGUIObjectByName("titleHeading["+ h +"]").hidden = true;
		Engine.GetGUIObjectByName("Heading[" + h + "]").hidden = true;
		for (let p = 0; p < g_MaxPlayers; ++p)
		{
			Engine.GetGUIObjectByName("valueData[" + p + "][" + h + "]").hidden = true;
			for (let t = 0; t < g_MaxTeams; ++t)
			{
				Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + h + "]").hidden = true;
				Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + h + "]").hidden = true;
			}
		}
	}
}

function updateGeneralPanelHeadings(headings)
{
	let left = 50;
	for (let h in headings)
	{
		let headerGUIName = "playerNameHeading";
		if (h > 0)
			headerGUIName = "Heading[" + (h - 1) + "]";

		let headerGUI = Engine.GetGUIObjectByName(headerGUIName);
		headerGUI.caption = headings[h].caption;
		headerGUI.size = left + " " + headings[h].yStart + " " + (left + headings[h].width) + " 100%";
		headerGUI.hidden = false;

		if (headings[h].width < g_LongHeadingWidth)
			left += headings[h].width;
	}
}

function updateGeneralPanelTitles(titleHeadings)
{
	let left = 250;
	for (let th in titleHeadings)
	{
		if (th >= g_MaxHeadingTitle)
			break;

		if (titleHeadings[th].xOffset)
			left += titleHeadings[th].xOffset;

		let headerGUI = Engine.GetGUIObjectByName("titleHeading["+ th +"]");
		headerGUI.caption = titleHeadings[th].caption;
		headerGUI.size = left + " " + titleHeadings[th].yStart + " " + (left + titleHeadings[th].width) + " 100%";
		headerGUI.hidden = false;

		if (titleHeadings[th].width < g_LongHeadingWidth)
			left += titleHeadings[th].width;
	}
}

function updateGeneralPanelCounter(counters)
{
	let rowPlayerObjectWidth = 0;
	let left = 0;

	for (let p = 0; p < g_MaxPlayers; ++p)
	{
		left = 240;
		let counterObject;

		for (let w in counters)
		{
			counterObject = Engine.GetGUIObjectByName("valueData[" + p + "][" + w + "]");
			counterObject.size = left + " " + counters[w].verticalOffset + " " + (left + counters[w].width) + " 100%";
			counterObject.hidden = false;
			left += counters[w].width;
		}

		if (rowPlayerObjectWidth == 0)
			rowPlayerObjectWidth = left;

		let counterTotalObject;
		for (let t = 0; t < g_MaxTeams; ++t)
		{
			left = 240;
			for (let w in counters)
			{
				counterObject = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]");
				counterObject.size = left + " " + counters[w].verticalOffset + " " + (left + counters[w].width) + " 100%";
				counterObject.hidden = false;

				if (g_Teams[t])
				{
					let yStart = 25 + g_Teams[t] * (g_PlayerBoxYSize + g_PlayerBoxGap) + 3 + counters[w].verticalOffset;
					counterTotalObject = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]");
					counterTotalObject.size = (left + 20) + " " + yStart + " " + (left + counters[w].width) + " 100%";
					counterTotalObject.hidden = false;
				}

				left += counters[w].width;
			}
		}
	}
	return rowPlayerObjectWidth;
}

function updateGeneralPanelTeams()
{
	if (!g_Teams || g_WithoutTeam > 0)
		Engine.GetGUIObjectByName("noTeamsBox").hidden = false;

	if (!g_Teams)
		return;

	let yStart = g_TeamsBoxYStart + g_WithoutTeam * (g_PlayerBoxYSize + g_PlayerBoxGap);
	for (let i = 0; i < g_Teams.length; ++i)
	{
		if (!g_Teams[i])
			continue;

		let teamBox = Engine.GetGUIObjectByName("teamBoxt["+i+"]");
		teamBox.hidden = false;
		let teamBoxSize = teamBox.size;
		teamBoxSize.top = yStart;
		teamBox.size = teamBoxSize;

		yStart += 30 + g_Teams[i] * (g_PlayerBoxYSize + g_PlayerBoxGap) + 32;

		Engine.GetGUIObjectByName("teamNameHeadingt["+i+"]").caption = "Team "+(i+1);

		let teamHeading = Engine.GetGUIObjectByName("teamHeadingt["+i+"]");
		let yStartTotal = 30 + g_Teams[i] * (g_PlayerBoxYSize + g_PlayerBoxGap) + 10;
		teamHeading.size = "50 " + yStartTotal + " 100% " + (yStartTotal + 20);
		teamHeading.caption = translate("Team total");
	}

	// If there are no players without team, hide "player name" heading
	if (!g_WithoutTeam)
		Engine.GetGUIObjectByName("playerNameHeading").caption = "";
}

function initPlayerBoxPositions()
{
	for (let h = 0; h < g_MaxPlayers; ++h)
	{
		let playerBox = Engine.GetGUIObjectByName("playerBox[" + h + "]");
		let boxSize = playerBox.size;
		boxSize.top += h * (g_PlayerBoxYSize + g_PlayerBoxGap);
		boxSize.bottom = boxSize.top + g_PlayerBoxYSize;
		playerBox.size = boxSize;

		for (let i = 0; i < g_MaxTeams; ++i)
		{
			let playerBoxt = Engine.GetGUIObjectByName("playerBoxt[" + i + "][" + h + "]");
			boxSize = playerBoxt.size;
			boxSize.top += h * (g_PlayerBoxYSize + g_PlayerBoxGap);
			boxSize.bottom = boxSize.top + g_PlayerBoxYSize;
			playerBoxt.size = boxSize;
		}
	}
}

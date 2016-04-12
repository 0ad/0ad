var g_ScorePanelsData = [
	{	// Scores panel
		"headings": [	// headings on score panel
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Economy score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Military score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Exploration score"), "yStart": 16, "width": 100 },
			{ "caption": translate("Total score"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [],
		"counters": [	// counters on score panel
			{ "width": 100, "fn": calculateEconomyScore },
			{ "width": 100, "fn": calculateMilitaryScore },
			{ "width": 100, "fn": calculateExplorationScore },
			{ "width": 100, "fn": calculateScoreTotal}
		],
		"teamCounterFn": calculateScoreTeam
	},
	{	// buildings panel
		"headings": [	// headings on buildings panel
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
			{ "caption": translate("Buildings Statistics (Constructed / Lost / Destroyed)"), "yStart": 16, "width": (85 * 7 + 105) },	// width = 700
		],
		"counters": [	// counters on buildings panel
			{ "width": 105, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings },
			{ "width": 85, "fn": calculateBuildings }
		],
		"teamCounterFn": calculateColorsTeam
	},
	{	// units panel
		"headings": [	// headings on units panel
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Total"), "yStart": 34, "width": 120 },
			{ "caption": translate("Infantry"), "yStart": 34, "width": 100 },
			{ "caption": translate("Worker"), "yStart": 34, "width": 100 },
			{ "caption": translate("Cavalry"), "yStart": 34, "width": 100 },
			{ "caption": translate("Champion"), "yStart": 34, "width": 100 },
			{ "caption": translate("Heroes"), "yStart": 34, "width": 100 },
			{ "caption": translate("Navy"), "yStart": 34, "width": 100 },
			{ "caption": translate("Traders"), "yStart": 34, "width": 100 }
		],
		"titleHeadings": [
			{ "caption": translate("Units Statistics (Trained / Lost / Killed)"), "yStart": 16, "width": (100 * 7 + 120) },	// width = 820
		],
		"counters": [	// counters on units panel
			{ "width": 120, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits },
			{ "width": 100, "fn": calculateUnits }
		],
		"teamCounterFn": calculateColorsTeam
	},
	{	// resources panel
		"headings": [	// headings on resources panel
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Food"), "yStart": 34, "width": 100 },
			{ "caption": translate("Wood"), "yStart": 34, "width": 100 },
			{ "caption": translate("Stone"), "yStart": 34, "width": 100 },
			{ "caption": translate("Metal"), "yStart": 34, "width": 100 },
			{ "caption": translate("Total"), "yStart": 34, "width": 110 },
			{ "caption": translate("Tributes (Sent / Received)"), "yStart": 16, "width": 121 },
			{ "caption": translate("Treasures collected"), "yStart": 16, "width": 100 },
			{ "caption": translate("Loot"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [
			{ "caption": translate("Resource Statistics (Gathered / Used)"), "yStart": 16, "width": (100 * 4 + 110) }, // width = 510
		],
		"counters": [	// counters on resources panel
			{ "width": 100, "fn": calculateResources },
			{ "width": 100, "fn": calculateResources },
			{ "width": 100, "fn": calculateResources },
			{ "width": 100, "fn": calculateResources },
			{ "width": 110, "fn": calculateTotalResources },
			{ "width": 121, "fn": calculateTributeSent },
			{ "width": 100, "fn": calculateTreasureCollected },
			{ "width": 100, "fn": calculateLootCollected }
		],
		"teamCounterFn": calculateResourcesTeam
	},
	{	// market panel
		"headings": [	// headings on market panel
			{ "caption": translate("Player name"), "yStart": 26, "width": 200 },
			{ "caption": translate("Food exchanged"), "yStart": 16, "width": 100 },
			{ "caption": translate("Wood exchanged"), "yStart": 16, "width": 100 },
			{ "caption": translate("Stone exchanged"), "yStart": 16, "width": 100 },
			{ "caption": translate("Metal exchanged"), "yStart": 16, "width": 100 },
			{ "caption": translate("Barter efficiency"), "yStart": 16, "width": 100 },
			{ "caption": translate("Trade income"), "yStart": 16, "width": 100 }
		],
		"titleHeadings": [],
		"counters": [	// counters on market panel
			{ "width": 100, "fn": calculateResourceExchanged },
			{ "width": 100, "fn": calculateResourceExchanged },
			{ "width": 100, "fn": calculateResourceExchanged },
			{ "width": 100, "fn": calculateResourceExchanged },
			{ "width": 100, "fn": calculateBarterEfficiency },
			{ "width": 100, "fn": calculateTradeIncome }
		],
		"teamCounterFn": calculateMarketTeam
	},
	{	// miscellaneous panel
		"headings": [	// headings on miscellaneous panel
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
		"counters": [	// counters on miscellaneous panel
			{ "width": 100, "fn": calculateVegetarianRatio },
			{ "width": 100, "fn": calculateFeminization },
			{ "width": 100, "fn": calculateKillDeathRatio },
			{ "width": 100, "fn": calculateMapExploration },
			{ "width": 100, "fn": calculateMapPeakControl },
			{ "width": 100, "fn": calculateMapFinalControl }
		],
		"teamCounterFn": calculateMiscellaneous
	}
];

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
			counterObject.size = left + " 6 " + (left + counters[w].width) + " 100%";
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
				counterObject.size = left + " 6 " + (left + counters[w].width) + " 100%";
				counterObject.hidden = false;

				if (g_Teams[t])
				{
					let yStart = 30 + g_Teams[t] * (g_PlayerBoxYSize + g_PlayerBoxGap) + 2;
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
		let yStartTotal = 30 + g_Teams[i] * (g_PlayerBoxYSize + g_PlayerBoxGap) + 2;
		teamHeading.size = "50 "+yStartTotal+" 100% "+(yStartTotal+20);
		teamHeading.caption = translate("Team total");
	}

	// If there are no players without team, hide "player name" heading
	if (!g_WithoutTeam)
		Engine.GetGUIObjectByName("playerNameHeading").caption = "";
}

function updateObjectPlayerPosition()
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

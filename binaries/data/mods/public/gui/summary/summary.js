// Max player slots for any map (should read from config)
const MAX_SLOTS = 8;

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
	
	for (var i = 0; i < panelNames.length; i++)
	{
		if (i != panelNumber)
		{
			Engine.GetGUIObjectByName(panelNames[i]).hidden = true;
			Engine.GetGUIObjectByName(panelNames[i] + 'Button').sprite = "BackgroundTab";
		}
		else
		{
			Engine.GetGUIObjectByName(panelNames[i]).hidden = false;
			Engine.GetGUIObjectByName(panelNames[i] + 'Button').sprite = "ForegroundTab";
			adjustTabDividers(Engine.GetGUIObjectByName(panelNames[i] + 'Button').size);
		}
	}
}

function init(data)
{
	// LOCAL CONSTS, VARIABLES & FUNCTIONS
	// const for filtering long collective headings
	const LONG_HEADING_WIDTH = 250;
	// number of panels
	const PANELS_COUNT = 6;
	// alpha for player box
	const PLAYER_BOX_ALPHA = " 32";
	// alpha for player colour box
	const PLAYER_COLOUR_BOX_ALPHA = " 255";
	// yStart value for spaceing teams boxes (and noTeamsBox)
	const TEAMS_BOX_Y_START = 65;
	// vertical size of player box
	const PLAYER_BOX_Y_SIZE = 30;
	// gap between players boxes
	const PLAYER_BOX_GAP = 2;
	
	// colours used for units and buildings
	const TRAINED_COLOR = '[color="201 255 200"]';
	const LOST_COLOR = '[color="255 213 213"]';
	const KILLED_COLOR = '[color="196 198 255"]';

	// colours used for gathered and traded resources
	const INCOME_COLOR = '[color="201 255 200"]';
	const OUTCOME_COLOR = '[color="255 213 213"]';
	
	const BUILDINGS_TYPES = [ "total", "House", "Economic", "Outpost", "Military", "Fortress", "CivCentre", "Wonder" ];
	const UNITS_TYPES = [ "total", "Infantry", "Worker", "Cavalry", "Champion", "Hero", "Ship" ];
	const RESOURCES_TYPES = [ "food", "wood", "stone", "metal" ];
	
	var panels = {
		"score": {		// score panel
			"headings": {	// headings on score panel
				"playerName0Heading":      { "yStart": 26, "width": 200 },
				"economyScoreHeading":     { "yStart": 16, "width": 100 },
				"militaryScoreHeading":    { "yStart": 16, "width": 100 },
				"explorationScoreHeading": { "yStart": 16, "width": 100 },
				"totalScoreHeading":       { "yStart": 16, "width": 100 }
			},
			"counters": {	// counters on score panel
				"economyScore":     {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"militaryScore":    {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"explorationScore": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"totalScore":       {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		},
		"buildings": {		// buildings panel
			"headings": {	// headings on buildings panel
				"playerName1Heading":        {"yStart": 26, "width": 200 },
				"buildingsHeading":          {"yStart": 16, "width": (85 * 7 + 105) },	// width = 735
				"totalBuildingsHeading":     {"yStart": 34, "width": 105 },
				"houseBuildingsHeading":     {"yStart": 34, "width": 85 },
				"economicBuildingsHeading":  {"yStart": 34, "width": 85 },
				"outpostBuildingsHeading":   {"yStart": 34, "width": 85 },
				"militaryBuildingsHeading":  {"yStart": 34, "width": 85 },
				"fortressBuildingsHeading":  {"yStart": 34, "width": 85 },
				"civCentreBuildingsHeading": {"yStart": 34, "width": 85 },
				"wonderBuildingsHeading":    {"yStart": 34, "width": 85 }
			},
			"counters": {	// counters on buildings panel
				"totalBuildings":     {"width": 105, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"houseBuildings":     {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"economicBuildings":  {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"outpostBuildings":   {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"militaryBuildings":  {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"fortressBuildings":  {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"civCentreBuildings": {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"wonderBuildings":    {"width": 85,  "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		},
		"units": {		// units panel
			"headings": {	// headings on units panel
				"playerName2Heading":   {"yStart": 26, "width": 200 },
				"unitsHeading":         {"yStart": 16, "width": (100 * 6 + 120) },	// width = 720
				"totalUnitsHeading":    {"yStart": 34, "width": 120 },
				"infantryUnitsHeading": {"yStart": 34, "width": 100 },
				"workerUnitsHeading":   {"yStart": 34, "width": 100 },
				"cavalryUnitsHeading":  {"yStart": 34, "width": 100 },
				"championUnitsHeading": {"yStart": 34, "width": 100 },
				"heroesUnitsHeading":   {"yStart": 34, "width": 100 },
				"navyUnitsHeading":     {"yStart": 34, "width": 100 }
			},
			"counters": {	// counters on units panel
				"totalUnits":    {"width": 120, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"infantryUnits": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"workerUnits":   {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"cavalryUnits":  {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"championUnits": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"heroesUnits":   {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"navyUnits":     {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		},
		"resources": {		// resources panel
			"headings": {	// headings on resources panel
				"playerName3Heading":        {"yStart": 26, "width": 200 },
				"resourceHeading":           {"yStart": 16, "width": (100 * 4 + 110) },//width = 510
				"foodGatheredHeading":       {"yStart": 34, "width": 100 },
				"woodGatheredHeading":       {"yStart": 34, "width": 100 },
				"stoneGatheredHeading":      {"yStart": 34, "width": 100 },
				"metalGatheredHeading":      {"yStart": 34, "width": 100 },
				"totalGatheredHeading":      {"yStart": 34, "width": 110 },
				"treasuresCollectedHeading": {"yStart": 16, "width": 100 },
				"resourcesTributedHeading":  {"yStart": 16, "width": 121 }
			},
			"counters": {	// counters on resources panel
				"foodGathered":       {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"woodGathered":       {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"stoneGathered":      {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"metalGathered":      {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"totalGathered":      {"width": 110, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"treasuresCollected": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"resourcesTributed":  {"width": 121, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		},
		"market": {		// market panel
			"headings": {	// headings on market panel
				"playerName4Heading":      {"yStart": 26, "width": 200 },
				"exchangedFoodHeading":    {"yStart": 16, "width": 100 },
				"exchangedWoodHeading":    {"yStart": 16, "width": 100 },
				"exchangedStoneHeading":   {"yStart": 16, "width": 100 },
				"exchangedMetalHeading":   {"yStart": 16, "width": 100 },
				"barterEfficiencyHeading": {"yStart": 16, "width": 100 },
				"tradeIncomeHeading":      {"yStart": 16, "width": 100 }
			},
			"counters": {	// counters on market panel
				"exchangedFood":    {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"exchangedWood":    {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"exchangedStone":   {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"exchangedMetal":   {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"barterEfficiency": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"tradeIncome":      {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		},
		"miscelanous": {	// miscelanous panel
			"headings": {	// headings on miscelanous panel
				"playerName5Heading":     {"yStart": 26, "width": 200 },
				"vegetarianRatioHeading": {"yStart": 16, "width": 100 },
				"feminisationHeading":    {"yStart": 26, "width": 100 },
				"killDeathRatioHeading":  {"yStart": 16, "width": 100 },
				"mapExplorationHeading":  {"yStart": 16, "width": 100 }
			},
			"counters": {	// counters on miscelanous panel
				"vegetarianRatio": {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"feminisation":    {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"killDeathRatio":  {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] },
				"mapExploration":  {"width": 100, "objects": [ ], "teamsScores": [ ], "teamsScoresCaption": [ ] }
			}
		}
	};
	
	function alignHeaders(headings)
	{		
		left = 50;
		for (var h in headings)
		{
			Engine.GetGUIObjectByName(h).size = left + " " + headings[h].yStart + " " + (left + headings[h].width) + " 100%";
			if (headings[h].width < LONG_HEADING_WIDTH)
				left += headings[h].width;			
		}
	}
	
	function alignCounters(counters, player)
	{
		left = 240;
		for each (var counter in counters)
		{
			counter.objects[player].size = left + " 2 " + (left + counter.width) + " 100%";
			left += counter.width;
		}
		
		return left;
	}
	
	// caption counters functions
	function captionEconomyScore()
	{
		var total = 0;
		for each (var res in playerState.statistics.resourcesGathered)
			total += res;
			
		return Math.round(total / 10);
	}
	
	function captionBuildings(object, type)
	{		
		object.caption = TRAINED_COLOR + playerState.statistics.buildingsConstructed[type] + '[/color] / ' +
				LOST_COLOR + playerState.statistics.buildingsLost[type] + '[/color] / ' +
				KILLED_COLOR + playerState.statistics.enemyBuildingsDestroyed[type] + '[/color]';
	}
	
	function captionUnits(object, type)
	{
		object.caption = TRAINED_COLOR + playerState.statistics.unitsTrained[type] + '[/color] / ' +
				LOST_COLOR + playerState.statistics.unitsLost[type] + '[/color] / ' +
				KILLED_COLOR + playerState.statistics.enemyUnitsKilled[type] + '[/color]';
	}
	
	function captionResourcesGathered(object, type)
	{
		object.caption = INCOME_COLOR + playerState.statistics.resourcesGathered[type] + '[/color] / ' +
				OUTCOME_COLOR + (playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type]) + '[/color]';
	}
	
	function captionTotalResourcesGathered()
	{
		var totalGathered = 0;
		var totalUsed = 0;
		
		for each (var type in RESOURCES_TYPES)
		{
			totalGathered += playerState.statistics.resourcesGathered[type];
			totalUsed += playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
		}
			
		return INCOME_COLOR + totalGathered + '[/color] / ' + OUTCOME_COLOR + totalUsed + '[/color]';
	}
	
	function captionResourcesTributed()
	{
		return INCOME_COLOR + playerState.statistics.tributesSent + "[/color] / " + OUTCOME_COLOR + playerState.statistics.tributesReceived + "[/color]";
	}
	
	function captionResourcesExchanged(object, type)
	{	
		object.caption = INCOME_COLOR + '+' + playerState.statistics.resourcesBought[type] + '[/color] ' +
				OUTCOME_COLOR + '-' + playerState.statistics.resourcesSold[type] + '[/color]';		
	}
	
	function captionBarterEfficiency()
	{
		var totalBought = 0;
		for each (var boughtAmount in playerState.statistics.resourcesBought)
			totalBought += boughtAmount;
		var totalSold = 0;
		for each (var soldAmount in playerState.statistics.resourcesSold)
			totalSold += soldAmount;
			
		return Math.floor(totalSold > 0 ? (totalBought / totalSold) * 100 : 0) + "%";
	}
	
	function captionVegetarianRatio()
	{
		if (playerState.statistics.resourcesGathered.vegetarianFood && playerState.statistics.resourcesGathered.food)
			return Math.floor((playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100) + "%";
		else
			return 0 + "%";	
	}
	
	function captionFeminisation()
	{
		if (playerState.statistics.unitsTrained.Worker && playerState.statistics.unitsTrained.Female)
			return Math.floor((playerState.statistics.unitsTrained.Female / playerState.statistics.unitsTrained.Worker) * 100) + "%";
		else
			return 0 + "%";
	}
	
	function captionKillDeathRatio()
	{
		if (!playerState.statistics.enemyUnitsKilled.total)
			return "0.00";
		if (!playerState.statistics.unitsLost.total)	// and enemyUnitsKilled.total > 0
			return "\u221E"; // infinity symbol
		return Math.round((playerState.statistics.enemyUnitsKilled.total / playerState.statistics.unitsLost.total)*100)/100;
	}
	
	function sumTeamBuildings(counter, type)
	{		
		if (counter.teamsScores[playerState.team] == 0)
		{			
			counter.teamsScores[playerState.team] = { };
			counter.teamsScores[playerState.team].buildingsConstructed = 0;
			counter.teamsScores[playerState.team].buildingsLost = 0;
			counter.teamsScores[playerState.team].enemyBuildingsDestroyed = 0;
		}

		counter.teamsScores[playerState.team].buildingsConstructed += playerState.statistics.buildingsConstructed[type];
		counter.teamsScores[playerState.team].buildingsLost += playerState.statistics.buildingsLost[type];
		counter.teamsScores[playerState.team].enemyBuildingsDestroyed += playerState.statistics.enemyBuildingsDestroyed[type];
		
		counter.teamsScoresCaption[playerState.team] = TRAINED_COLOR + counter.teamsScores[playerState.team].buildingsConstructed + '[/color] / ' +
			LOST_COLOR + counter.teamsScores[playerState.team].buildingsLost + '[/color] / ' +
			KILLED_COLOR + counter.teamsScores[playerState.team].enemyBuildingsDestroyed + '[/color]';
	}
	
	function sumTeamUnits(counter, type)
	{		
		if (counter.teamsScores[playerState.team] == 0)
		{
			counter.teamsScores[playerState.team] = { };
			counter.teamsScores[playerState.team].unitsTrained = 0;
			counter.teamsScores[playerState.team].unitsLost = 0;
			counter.teamsScores[playerState.team].enemyUnitsKilled = 0;
		}

		counter.teamsScores[playerState.team].unitsTrained += playerState.statistics.unitsTrained[type];
		counter.teamsScores[playerState.team].unitsLost += playerState.statistics.unitsLost[type];
		counter.teamsScores[playerState.team].enemyUnitsKilled += playerState.statistics.enemyUnitsKilled[type];
		
		counter.teamsScoresCaption[playerState.team] = TRAINED_COLOR + counter.teamsScores[playerState.team].unitsTrained + '[/color] / ' +
			LOST_COLOR + counter.teamsScores[playerState.team].unitsLost + '[/color] / ' +
			KILLED_COLOR + counter.teamsScores[playerState.team].enemyUnitsKilled + '[/color]';
	}
	
	function sumResourcesGathered(counter, type)
	{		
		if (counter.teamsScores[playerState.team] == 0)
		{
			counter.teamsScores[playerState.team] = { };
			counter.teamsScores[playerState.team].resourcesGathered = 0;
			counter.teamsScores[playerState.team].resourcesUsed = 0;
		}
		
		counter.teamsScores[playerState.team].resourcesGathered += playerState.statistics.resourcesGathered[type];
		counter.teamsScores[playerState.team].resourcesUsed += playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
		
		counter.teamsScoresCaption[playerState.team] = INCOME_COLOR + counter.teamsScores[playerState.team].resourcesGathered + '[/color] / ' +
			OUTCOME_COLOR + counter.teamsScores[playerState.team].resourcesUsed + '[/color]';
	}
	
	function sumTotalResourcesGathered()
	{
		if (panels.resources.counters.totalGathered.teamsScores[playerState.team] == 0)
		{
			panels.resources.counters.totalGathered.teamsScores[playerState.team] = { };
			panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesGathered = 0;
			panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesUsed = 0;
		}
		
		for each (var type in RESOURCES_TYPES)
		{
			panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesGathered += playerState.statistics.resourcesGathered[type];
			panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesUsed +=
				playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
		}
		
		panels.resources.counters.totalGathered.teamsScoresCaption[playerState.team] =
			INCOME_COLOR + panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesGathered + '[/color] / ' +
			OUTCOME_COLOR + panels.resources.counters.totalGathered.teamsScores[playerState.team].resourcesUsed + '[/color]';
	}
	
	function sumResourcesTributed()
	{
		if (panels.resources.counters.resourcesTributed.teamsScores[playerState.team] == 0)
		{
			panels.resources.counters.resourcesTributed.teamsScores[playerState.team] = { };
			panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesSent = 0;
			panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesReceived = 0;
		}
		
		panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesSent += playerState.statistics.tributesSent;
		panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesReceived += playerState.statistics.tributesReceived;
		
		panels.resources.counters.resourcesTributed.teamsScoresCaption[playerState.team] =
			INCOME_COLOR + panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesSent + "[/color] / " +
			OUTCOME_COLOR + panels.resources.counters.resourcesTributed.teamsScores[playerState.team].tributesReceived + "[/color]";
	}
	
	function sumResourcesExchanged(counter, type)
	{		
		if (counter.teamsScores[playerState.team] == 0)
		{
			counter.teamsScores[playerState.team] = { };
			counter.teamsScores[playerState.team].resourcesBought = 0;
			counter.teamsScores[playerState.team].resourcesSold = 0;
		}
		
		counter.teamsScores[playerState.team].resourcesBought += playerState.statistics.resourcesBought[type];
		counter.teamsScores[playerState.team].resourcesSold += playerState.statistics.resourcesSold[type];
		
		counter.teamsScoresCaption[playerState.team] =
			INCOME_COLOR + '+' + counter.teamsScores[playerState.team].resourcesBought + '[/color] ' +
			OUTCOME_COLOR + '-' + counter.teamsScores[playerState.team].resourcesSold + '[/color]';
	}
	
	function sumBarterEfficiency()
	{
		if (panels.market.counters.barterEfficiency.teamsScores[playerState.team] == 0)
		{
			panels.market.counters.barterEfficiency.teamsScores[playerState.team] = { };
			panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesBought = 0;
			panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesSold = 0;
		}
		
		for each (var boughtAmount in playerState.statistics.resourcesBought)
			panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesBought += boughtAmount;
		for each (var soldAmount in playerState.statistics.resourcesSold)
			panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesSold += soldAmount;
			
		panels.market.counters.barterEfficiency.teamsScoresCaption[playerState.team] =
			Math.floor(panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesSold > 0 ?
			(panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesBought /
			 panels.market.counters.barterEfficiency.teamsScores[playerState.team].resourcesSold) * 100 : 0) + "%";
	}
	
	function sumVegetarianRatio()
	{
		if (panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team] == 0)
		{
			panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team] = { };
			panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].vegetarianFood = 0;
			panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].food = 0;
		}
		
		panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].vegetarianFood += playerState.statistics.resourcesGathered.vegetarianFood;
		panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].food += playerState.statistics.resourcesGathered.food;
		
		if (panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].food &&
		    panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].vegetarianFood)
		{
			panels.miscelanous.counters.vegetarianRatio.teamsScoresCaption[playerState.team] =
				Math.floor((panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].vegetarianFood /
					   panels.miscelanous.counters.vegetarianRatio.teamsScores[playerState.team].food) * 100) + "%";
			return;
		}
		panels.miscelanous.counters.vegetarianRatio.teamsScoresCaption[playerState.team] = 0 + "%";
	}
	
	function sumFeminisation()
	{
		if (panels.miscelanous.counters.feminisation.teamsScores[playerState.team] == 0)
		{
			panels.miscelanous.counters.feminisation.teamsScores[playerState.team] = { };
			panels.miscelanous.counters.feminisation.teamsScores[playerState.team].femalesTrained = 0;
			panels.miscelanous.counters.feminisation.teamsScores[playerState.team].workersTrained = 0;
		}
		
		panels.miscelanous.counters.feminisation.teamsScores[playerState.team].femalesTrained += playerState.statistics.unitsTrained.Female;
		panels.miscelanous.counters.feminisation.teamsScores[playerState.team].workersTrained += playerState.statistics.unitsTrained.Worker;
		
		if (panels.miscelanous.counters.feminisation.teamsScores[playerState.team].femalesTrained &&
		    panels.miscelanous.counters.feminisation.teamsScores[playerState.team].workersTrained)
		{
			panels.miscelanous.counters.feminisation.teamsScoresCaption[playerState.team] =
				Math.floor((panels.miscelanous.counters.feminisation.teamsScores[playerState.team].femalesTrained /
					    panels.miscelanous.counters.feminisation.teamsScores[playerState.team].workersTrained) * 100) + "%";
				return;
		}
		panels.miscelanous.counters.feminisation.teamsScoresCaption[playerState.team] = 0 + "%";
	}
	
	function sumKillDeathRatio()
	{
		if (panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team] == 0)
		{
			panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team] = { };
			panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].enemyUnitsKilled = 0;
			panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].unitsLost = 0;
		}
		
		panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].enemyUnitsKilled += playerState.statistics.enemyUnitsKilled.total;
		panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].unitsLost += playerState.statistics.unitsLost.total;
		
		if (!panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].enemyUnitsKilled)
		{
			panels.miscelanous.counters.killDeathRatio.teamsScoresCaption[playerState.team] = "0.00";
			return;
		}
		if (!panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].unitsLost)
		{
			panels.miscelanous.counters.killDeathRatio.teamsScoresCaption[playerState.team] = "\u221E"; // infinity symbol
			return;
		}
		panels.miscelanous.counters.killDeathRatio.teamsScoresCaption[playerState.team] =
			Math.round((panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].enemyUnitsKilled /
				    panels.miscelanous.counters.killDeathRatio.teamsScores[playerState.team].unitsLost) * 100)/100;
	}
	
	// FUNCTION BODY
	// Load data
	var civData = loadCivData();
	// Map
	var mapDisplayType = translate("Scenario");

	Engine.GetGUIObjectByName("timeElapsed").caption = sprintf(translate("Time elapsed: %(time)s"), { time: timeToString(data.timeElapsed) });

	Engine.GetGUIObjectByName("summaryText").caption = data.gameResult;

	// This is only defined for random maps
	if (data.mapSettings.Size)
	{
		// load the map sizes from the JSON file
		var mapSizes = initMapSizes();

		// retrieve the index of the map size
		for (var mapSizeIndex in mapSizes.tiles)
		{
			if (mapSizes.tiles[mapSizeIndex] == data.mapSettings.Size)
			{
				mapDisplayType = mapSizes.names[mapSizeIndex];
				break;
			}
		}
	}

	Engine.GetGUIObjectByName("mapName").caption = sprintf(translate("%(mapName)s - %(mapType)s"), { mapName: data.mapSettings.Name, mapType: mapDisplayType});
	
	// Panels
	// Align headers
	var left = 50;
	for each (var panel in panels)	// for all panels
		alignHeaders(panel.headings);

	// TODO set maxPlayers as playerCounters.length
	var maxPlayers = data.playerStates.length - 1;
	var maxTeams = 0;

	var teams = [ ];
	if (data.mapSettings.LockTeams)	// teams ARE locked
	{
		// count teams
		for(var t = 0; t < maxPlayers; ++t)
		{
			if (!teams[data.playerStates[t+1].team])
			{
				teams[data.playerStates[t+1].team] = 1;
				continue;
			}
			teams[data.playerStates[t+1].team]++;
		}
		
		if (teams.length == maxPlayers)
			teams = false;	// Each player has his own team. Displaying teams makes no sense.
	}
	else				// teams are NOT locked
		teams = false;
	
	// Erase teams data if teams are not displayed
	if (!teams)
	{
		for(var p = 0; p < maxPlayers; ++p)
			data.playerStates[p+1].team = -1;
	}
	
	// Count players without team	(or all if teams are not displayed)
	var withoutTeam = maxPlayers;
	if (teams)
	{
		// count players without team	(or all if teams are not displayed)
		for (var i = 0; i < teams.length; ++i)
			withoutTeam -= teams[i];
		
		// Display teams boxes
		var p = 0;
		for each (var panel in panels)
		{
			var yStart = TEAMS_BOX_Y_START + withoutTeam * (PLAYER_BOX_Y_SIZE + PLAYER_BOX_GAP);
			for (var i = 0; i < teams.length; ++i)
			{
				var teamBox = Engine.GetGUIObjectByName("teamBox"+p+"t"+i);
				teamBox.hidden = false;
				var teamBoxSize = teamBox.size;
				teamBoxSize.top = yStart;
				teamBox.size = teamBoxSize;
				
				yStart += 30 + teams[i] * (PLAYER_BOX_Y_SIZE + PLAYER_BOX_GAP) + 32;
				
				Engine.GetGUIObjectByName("teamNameHeading"+p+"t"+i).caption = "Team "+(i+1);
				
				// Make place to store team scores
				for each (var counter in panel.counters)
				{
					counter.teamsScores[i] = 0;
					counter.teamsScoresCaption[i] = "0";
				}
			}
			
			// If there are no players without team, hide "player name" heading
			if (!withoutTeam)
				Engine.GetGUIObjectByName("playerName"+p+"Heading").caption = "";	
			
			p++;
		}
	}
	
	if (withoutTeam)
	{
		// Show boxes for no teams
		for (var b = 0; b < PANELS_COUNT; ++b)
			Engine.GetGUIObjectByName("noTeamsBox"+b).hidden = false;
	}

	var playerBoxesCounts = [ ];
	for (var i = 0; i < maxPlayers; ++i)
	{
		var tn = "";
		var playerState = data.playerStates[i+1];
		
		if (!playerBoxesCounts[playerState.team+1])
			playerBoxesCounts[playerState.team+1] = 1;
		else
			playerBoxesCounts[playerState.team+1] += 1;

		if (playerState.team != -1)
			tn = "t"+playerState.team+"p";

		var j = 0;
		for each (var panel in panels)
		{
			var playerIdentityString = tn+"["+(playerBoxesCounts[playerState.team+1]-1)+"]";
			// Display boxes for players
			var playerBox = Engine.GetGUIObjectByName("playerBox"+j+playerIdentityString); 
			playerBox.hidden = false;
			
			var boxSize = playerBox.size;
			boxSize.top += (playerBoxesCounts[playerState.team+1] - 1) * (PLAYER_BOX_Y_SIZE + PLAYER_BOX_GAP);
			boxSize.bottom = boxSize.top + PLAYER_BOX_Y_SIZE;
			playerBox.size = boxSize;
			
			var colourString = "colour: "
				+ Math.floor(playerState.colour.r * 255) + " "
				+ Math.floor(playerState.colour.g * 255) + " "
				+ Math.floor(playerState.colour.b * 255);
			
			playerBox.sprite = colourString + PLAYER_BOX_ALPHA;
			
			var playerColourBox = Engine.GetGUIObjectByName("playerColourBox"+j+playerIdentityString);
			playerColourBox.sprite = colourString + PLAYER_COLOUR_BOX_ALPHA;
			
			// Show the multiplayer name, e.g. "Foobar" rather than "Player 1".
			// TODO: Perhaps show both the multiplayer and map-specific name?
			var playerName = Engine.GetGUIObjectByName("playerName"+j+playerIdentityString);
			playerName.caption = data.players[i+1].name;

			var civIcon = Engine.GetGUIObjectByName("civIcon"+j+playerIdentityString);
			civIcon.sprite = "stretched:"+civData[playerState.civ].Emblem;
			civIcon.tooltip = civData[playerState.civ].Name;
			
			// Get counters
			for (var c in panel.counters)
			{
				panel.counters[c].objects[i] = Engine.GetGUIObjectByName(c+playerIdentityString);
			}
			
			// Align counters
			var right = alignCounters(panel.counters, i);
			boxSize.right = right;
			playerBox.size = boxSize;
			
			j++;
		}
		
		// Assign counters
		// score panel
		panels.score.counters.economyScore.objects[i].caption = captionEconomyScore();
		panels.score.counters.militaryScore.objects[i].caption = Math.round((playerState.statistics.enemyUnitsKilledValue +
			playerState.statistics.enemyBuildingsDestroyedValue) / 10);
		panels.score.counters.explorationScore.objects[i].caption = playerState.statistics.percentMapExplored * 10;
		panels.score.counters.totalScore.objects[i].caption = (+panels.score.counters.economyScore.objects[i].caption) +
			(+panels.score.counters.militaryScore.objects[i].caption) +
			(+panels.score.counters.explorationScore.objects[i].caption);
		// buildings panel
		var t = 0;
		for each (var counter in panels.buildings.counters)
		{
			captionBuildings(counter.objects[i], BUILDINGS_TYPES[t]);
			t++;
		}
		// units panel
		t = 0;
		for each (var counter in panels.units.counters)
		{
			captionUnits(counter.objects[i], UNITS_TYPES[t]);
			t++;
		}
		// resources panel
		t = 0;
		for each (var counter in panels.resources.counters)
		{
			if (t >= 4)	// only 4 first counters
				break;
			
			captionResourcesGathered(counter.objects[i], RESOURCES_TYPES[t]);
			t++;
		}
		panels.resources.counters.totalGathered.objects[i].caption = captionTotalResourcesGathered();
		panels.resources.counters.treasuresCollected.objects[i].caption = playerState.statistics.treasuresCollected;
		panels.resources.counters.resourcesTributed.objects[i].caption = captionResourcesTributed();
		// market panel
		t = 0;
		for (var c in panels.market.counters)
		{
			if (t >= 4)	// only 4 first counters
				break;
			
			captionResourcesExchanged(panels.market.counters[c].objects[i], RESOURCES_TYPES[t]);
			t++;
		}
		panels.market.counters.barterEfficiency.objects[i].caption = captionBarterEfficiency();
		panels.market.counters.tradeIncome.objects[i].caption = playerState.statistics.tradeIncome;
		// miscelanous panel
		panels.miscelanous.counters.vegetarianRatio.objects[i].caption = captionVegetarianRatio();
		panels.miscelanous.counters.feminisation.objects[i].caption = captionFeminisation();
		panels.miscelanous.counters.killDeathRatio.objects[i].caption = captionKillDeathRatio();
		panels.miscelanous.counters.mapExploration.objects[i].caption = playerState.statistics.percentMapExplored + "%";
		
		if (!teams)
			continue;
		
		if (playerState.team == -1)
			continue;
		
		// Evaluate team total score
		// score panel
		for (var c in panels.score.counters)
		{
			panels.score.counters[c].teamsScores[playerState.team] += (+panels.score.counters[c].objects[i].caption);
			panels.score.counters[c].teamsScoresCaption[playerState.team] = panels.score.counters[c].teamsScores[playerState.team];
		}
		// buildings panel
		var t = 0;
		for each (var counter in panels.buildings.counters)
		{
			sumTeamBuildings(counter, BUILDINGS_TYPES[t]);
			t++;
		}
		// units panel
		t = 0;
		for each (var counter in panels.units.counters)
		{
			sumTeamUnits(counter, UNITS_TYPES[t]);
			t++;
		}
		// resources panel
		t = 0;
		for each (var counter in panels.resources.counters)
		{
			if (t >= 4)	// only 4 first counters
				break;
			
			sumResourcesGathered(counter, RESOURCES_TYPES[t]);
			t++;
		}
		sumTotalResourcesGathered();
		panels.resources.counters.treasuresCollected.teamsScores[playerState.team] += playerState.statistics.treasuresCollected;
		panels.resources.counters.treasuresCollected.teamsScoresCaption[playerState.team] = panels.resources.counters.treasuresCollected.teamsScores[playerState.team];
		sumResourcesTributed();
		// market panel
		t = 0;
		for (var c in panels.market.counters)
		{
			if (t >= 4)	// only 4 first counters
				break;
			
			sumResourcesExchanged(panels.market.counters[c], RESOURCES_TYPES[t]);
			t++;
		}
		sumBarterEfficiency();
		panels.market.counters.tradeIncome.teamsScores[playerState.team] += playerState.statistics.tradeIncome;
		panels.market.counters.tradeIncome.teamsScoresCaption[playerState.team] = panels.market.counters.tradeIncome.teamsScores[playerState.team];
		// miscelanous panel
		sumVegetarianRatio();
		sumFeminisation();
		sumKillDeathRatio();
		// TODO: probably change from simple sum to union from range manager
		panels.miscelanous.counters.mapExploration.teamsScores[playerState.team] += playerState.statistics.percentMapExplored;
		panels.miscelanous.counters.mapExploration.teamsScoresCaption[playerState.team] = panels.miscelanous.counters.mapExploration.teamsScores[playerState.team] + "%";
	}
	
	if (!teams)
	{
		selectPanel(0);
		return;
	}
	
	// Display teams totals counters
	for (var i = 0; i < teams.length; ++i)
	{
		var pn = 0;
		for each (var panel in panels)
		{
			var teamHeading = Engine.GetGUIObjectByName("teamHeading"+pn+"t"+i);
			var yStart = 30 + teams[i] * (PLAYER_BOX_Y_SIZE + PLAYER_BOX_GAP) + 2;
			teamHeading.size = "50 "+yStart+" 100% "+(yStart+20);
			teamHeading.caption = "Team total";
			
			var left = 250;
			for (var c in panel.counters)
			{
				var counter = Engine.GetGUIObjectByName(c+"t"+i);
				counter.size = left + " " + yStart + " " + (left + panel.counters[c].width) + " " + (yStart+20);
				counter.caption = panel.counters[c].teamsScoresCaption[i];
				
				left += panel.counters[c].width;
			}
			pn++;
		}
	}

	selectPanel(0);
}

function onTick()
{
}

// Max player slots for any map (should read from config)
const MAX_SLOTS = 8;

var panelNames = [ 'scorePanel', 'unitsBuildingsPanel', 'conquestPanel', 'resourcesPanel', 'marketPanel' ];
var panelButtonNames = [ 'scorePanelButton', 'unitsBuildingsPanelButton', 'conquestPanelButton', 'resourcesPanelButton', 'marketPanelButton' ];

/**
 * Select active panel
 * @param panelNumber Number of panel, which should get active state (integer)
 */
function selectPanel(panelNumber)
{
	for (var i = 0; i < panelNames.length; i++)
	{
		if (i != panelNumber)
		{
			Engine.GetGUIObjectByName(panelNames[i]).hidden = true;
			Engine.GetGUIObjectByName(panelButtonNames[i]).sprite = "BackgroundTab";
		}
		else
		{
			Engine.GetGUIObjectByName(panelNames[i]).hidden = false;
			Engine.GetGUIObjectByName(panelButtonNames[i]).sprite = "ForegroundTab";
			adjustTabDividers(Engine.GetGUIObjectByName(panelButtonNames[i]).size);
		}
	}
}

function adjustTabDividers(tabSize)
{
	var leftSpacer = Engine.GetGUIObjectByName("tabDividerLeft");
	var rightSpacer = Engine.GetGUIObjectByName("tabDividerRight");
	leftSpacer.size = "20 " + leftSpacer.size.top + " " + (tabSize.left + 2) + " " + leftSpacer.size.bottom;
	rightSpacer.size = (tabSize.right - 2) + " " + rightSpacer.size.top + " 100%-20 " + rightSpacer.size.bottom;
}

function init(data)
{
	var civData = loadCivData();
	var mapSize = "Scenario";

	Engine.GetGUIObjectByName("timeElapsed").caption = "Time elapsed: " + timeToString(data.timeElapsed);

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
				mapSize = mapSizes.names[mapSizeIndex];
				break;
			}
		}
	}

	Engine.GetGUIObjectByName("mapName").caption = data.mapSettings.Name + " - " + mapSize;

	// Space player boxes
	var boxSpacing = 32;
	for (var i = 0; i < panelNames.length; ++i)
	{
		for (var j = 0; j < MAX_SLOTS; ++j)
		{
			var box = Engine.GetGUIObjectByName("playerBox"+i+"["+j+"]");
			var boxSize = box.size;
			var h = boxSize.bottom - boxSize.top;
			boxSize.top = j * boxSpacing;
			boxSize.bottom = j * boxSpacing + h;
			box.size = boxSize;
		}
	}

	// TODO set maxPlayers as playerCounters.length
	var maxPlayers = data.playerStates.length - 1;

	// Align headers
	var left = 50;
	var width = 100;
	var playerNameHeadingWidth = 200;
	// Special cased to make the (Sent / Received) part fit
	var tributesWidth = 121;
	Engine.GetGUIObjectByName("playerName0Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	Engine.GetGUIObjectByName("economyScoreHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("militaryScoreHeading").size = left +  " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("explorationScoreHeading").size = left +  " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("totalScoreHeading").size = left +  " 16 " + (left + width) + " 100%"; left += width;
	
	left = 50;
	Engine.GetGUIObjectByName("playerName1Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	Engine.GetGUIObjectByName("unitsTrainedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("unitsLostHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("enemyUnitsKilledHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("buildingsConstructedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("buildingsLostHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("enemyBuildingsDestroyedHeading").size = left + " 6 " + (left + width) + " 100%"; left += width;

	left = 50;
	Engine.GetGUIObjectByName("playerName2Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	Engine.GetGUIObjectByName("civCentresBuiltHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("enemyCivCentresDestroyedHeading").size = left +  " 6 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("mapExplorationHeading").size = left +  " 6 " + (left + width) + " 100%"; left += width;

	left = 50;
	Engine.GetGUIObjectByName("playerName3Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	Engine.GetGUIObjectByName("resourceHeading").size = left + " 16 " + (left + width * 4) + " 100%";
	Engine.GetGUIObjectByName("foodGatheredHeading").size = left + " 34 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("woodGatheredHeading").size = left + " 34 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("stoneGatheredHeading").size = left + " 34 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("metalGatheredHeading").size = left + " 34 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("vegetarianRatioHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("treasuresCollectedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("resourcesTributedHeading").size = left + " 16 " + (left + tributesWidth) + " 100%"; left += tributesWidth;

	left = 50;
	Engine.GetGUIObjectByName("playerName4Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	Engine.GetGUIObjectByName("exchangedFoodHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("exchangedWoodHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("exchangedStoneHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("exchangedMetalHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("barterEfficiencyHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	Engine.GetGUIObjectByName("tradeIncomeHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;

	// Show counters
	for (var i = 0; i < MAX_SLOTS; ++i)
	{
		if (i < maxPlayers)
		{
			var playerState = data.playerStates[i+1];

			for (var k = 0; k < panelNames.length; ++k)
			{
				var playerBox = Engine.GetGUIObjectByName("playerBox"+k+"["+i+"]");
				playerBox.hidden = false;

				var colourString = "colour: "
					+ Math.floor(playerState.colour.r * 255) + " "
					+ Math.floor(playerState.colour.g * 255) + " "
					+ Math.floor(playerState.colour.b * 255);
				playerBox.sprite = colourString + " 32";
				var playerColourBox = Engine.GetGUIObjectByName("playerColourBox"+k+"["+i+"]");
				playerColourBox.sprite = colourString + " 255";

				// Show the multiplayer name, e.g. "Foobar" rather than "Player 1".
				// TODO: Perhaps show both the multiplayer and map-specific name?
				var playerName = Engine.GetGUIObjectByName("playerName"+k+"["+i+"]");
				playerName.caption = data.players[i+1].name;

				Engine.GetGUIObjectByName("civIcon"+k+"["+i+"]").sprite = "stretched:"+civData[playerState.civ].Emblem;
				Engine.GetGUIObjectByName("civIcon"+k+"["+i+"]").tooltip = civData[playerState.civ].Name;
			}

			var economyScore = Engine.GetGUIObjectByName("economyScore["+i+"]");
			var militaryScore = Engine.GetGUIObjectByName("militaryScore["+i+"]");
			var explorationScore = Engine.GetGUIObjectByName("explorationScore["+i+"]");
			var totalScore = Engine.GetGUIObjectByName("totalScore["+i+"]");

			var unitsTrained = Engine.GetGUIObjectByName("unitsTrained["+i+"]");
			var unitsLost = Engine.GetGUIObjectByName("unitsLost["+i+"]");
			var enemyUnitsKilled = Engine.GetGUIObjectByName("enemyUnitsKilled["+i+"]");
			var buildingsConstructed = Engine.GetGUIObjectByName("buildingsConstructed["+i+"]");
			var buildingsLost = Engine.GetGUIObjectByName("buildingsLost["+i+"]");
			var enemyBuildingsDestroyed = Engine.GetGUIObjectByName("enemyBuildingsDestroyed["+i+"]");

			var civCentresBuilt = Engine.GetGUIObjectByName("civCentresBuilt["+i+"]");
			var enemyCivCentresDestroyed = Engine.GetGUIObjectByName("enemyCivCentresDestroyed["+i+"]");
			var mapExploration = Engine.GetGUIObjectByName("mapExploration["+i+"]");

			var foodGathered = Engine.GetGUIObjectByName("foodGathered["+i+"]");
			var woodGathered = Engine.GetGUIObjectByName("woodGathered["+i+"]");
			var stoneGathered = Engine.GetGUIObjectByName("stoneGathered["+i+"]");
			var metalGathered = Engine.GetGUIObjectByName("metalGathered["+i+"]");
			var vegetarianRatio = Engine.GetGUIObjectByName("vegetarianRatio["+i+"]");
			var treasuresCollected = Engine.GetGUIObjectByName("treasuresCollected["+i+"]");
			var resourcesTributed = Engine.GetGUIObjectByName("resourcesTributed["+i+"]");

			var exchangedFood = Engine.GetGUIObjectByName("exchangedFood["+i+"]");
			var exchangedWood = Engine.GetGUIObjectByName("exchangedWood["+i+"]");
			var exchangedStone = Engine.GetGUIObjectByName("exchangedStone["+i+"]");
			var exchangedMetal = Engine.GetGUIObjectByName("exchangedMetal["+i+"]");
			var barterEfficiency = Engine.GetGUIObjectByName("barterEfficiency["+i+"]");
			var tradeIncome = Engine.GetGUIObjectByName("tradeIncome["+i+"]");

			// align counters

			left = 240;
			width = 100;
			economyScore.size = left + " 2 " + (left + width) + " 100%"; left += width;
			militaryScore.size = left + " 2 " + (left + width) + " 100%"; left += width;
			explorationScore.size = left + " 2 " + (left + width) + " 100%"; left += width;
			totalScore.size = left + " 2 " + (left + width) + " 100%"; left += width;
			var size = Engine.GetGUIObjectByName("playerBox0["+i+"]").size;
			size.right = left + 10;
			Engine.GetGUIObjectByName("playerBox0["+i+"]").size = size;
			
			left = 240;
			unitsTrained.size = left + " 2 " + (left + width) + " 100%"; left += width;
			unitsLost.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyUnitsKilled.size = left + " 2 " + (left + width) + " 100%"; left += width;
			buildingsConstructed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			buildingsLost.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyBuildingsDestroyed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			size = Engine.GetGUIObjectByName("playerBox1["+i+"]").size;
			size.right = left + 10;
			Engine.GetGUIObjectByName("playerBox1["+i+"]").size = size;

			left = 240;
			civCentresBuilt.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyCivCentresDestroyed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			mapExploration.size = left + " 2 " + (left + width) + " 100%"; left += width;
			size = Engine.GetGUIObjectByName("playerBox2["+i+"]").size;
			size.right = left + 10;
			Engine.GetGUIObjectByName("playerBox2["+i+"]").size = size;

			left = 240;
			foodGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			woodGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			stoneGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			metalGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			vegetarianRatio.size = left + " 2 " + (left + width) + " 100%"; left += width;
			treasuresCollected.size	= left + " 2 " + (left + width) + " 100%"; left += width;
			resourcesTributed.size = left + " 2 " + (left + tributesWidth) + " 100%"; left += tributesWidth;
			size = Engine.GetGUIObjectByName("playerBox3["+i+"]").size;
			size.right = left + 10;
			Engine.GetGUIObjectByName("playerBox3["+i+"]").size = size;

			left = 240;
			exchangedFood.size = left + " 2 " + (left + width) + " 100%"; left += width;
			exchangedWood.size = left + " 2 " + (left + width) + " 100%"; left += width;
			exchangedStone.size = left + " 2 " + (left + width) + " 100%"; left += width;
			exchangedMetal.size = left + " 2 " + (left + width) + " 100%"; left += width;
			barterEfficiency.size = left + " 2 " + (left + width) + " 100%"; left += width;
			tradeIncome.size = left + " 2 " + (left + width) + " 100%"; left += width;
			size = Engine.GetGUIObjectByName("playerBox4["+i+"]").size;
			size.right = left + 10;
			Engine.GetGUIObjectByName("playerBox4["+i+"]").size = size;

			// display counters
			economyScore.caption = Math.round((playerState.statistics.resourcesGathered.food + playerState.statistics.resourcesGathered.wood + 
				playerState.statistics.resourcesGathered.stone + playerState.statistics.resourcesGathered.metal) / 10);
			militaryScore.caption = Math.round((playerState.statistics.enemyUnitsKilledValue + playerState.statistics.enemyBuildingsDestroyedValue) / 10);
			explorationScore.caption = playerState.statistics.percentMapExplored * 10;
			totalScore.caption = Number(economyScore.caption) + Number(militaryScore.caption) + Number(explorationScore.caption);
			
			unitsTrained.caption = playerState.statistics.unitsTrained;
			unitsLost.caption = playerState.statistics.unitsLost;
			enemyUnitsKilled.caption = playerState.statistics.enemyUnitsKilled;
			buildingsConstructed.caption = playerState.statistics.buildingsConstructed;
			buildingsLost.caption = playerState.statistics.buildingsLost;
			enemyBuildingsDestroyed.caption = playerState.statistics.enemyBuildingsDestroyed;

			civCentresBuilt.caption = playerState.statistics.civCentresBuilt;
			enemyCivCentresDestroyed.caption = playerState.statistics.enemyCivCentresDestroyed;
			mapExploration.caption = playerState.statistics.percentMapExplored + "%";

			const SOLD_COLOR = '[color="201 255 200"]';
			const BOUGHT_COLOR = '[color="255 213 213"]';
			foodGathered.caption = SOLD_COLOR + playerState.statistics.resourcesGathered.food + "[/color] / " +
				BOUGHT_COLOR + (playerState.statistics.resourcesUsed.food - playerState.statistics.resourcesSold.food) + "[/color]";
			woodGathered.caption = SOLD_COLOR + playerState.statistics.resourcesGathered.wood + "[/color] / " +
				BOUGHT_COLOR + (playerState.statistics.resourcesUsed.wood - playerState.statistics.resourcesSold.wood) + "[/color]";
			stoneGathered.caption = SOLD_COLOR + playerState.statistics.resourcesGathered.stone + "[/color] / " +
				BOUGHT_COLOR + (playerState.statistics.resourcesUsed.stone - playerState.statistics.resourcesSold.stone) + "[/color]";
			metalGathered.caption = SOLD_COLOR + playerState.statistics.resourcesGathered.metal + "[/color] / " +
				BOUGHT_COLOR + (playerState.statistics.resourcesUsed.metal - playerState.statistics.resourcesSold.metal) + "[/color]";
			vegetarianRatio.caption = Math.floor(playerState.statistics.resourcesGathered.food > 0 ?
				(playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100 : 0) + "%";
			treasuresCollected.caption = playerState.statistics.treasuresCollected;
			resourcesTributed.caption = SOLD_COLOR + playerState.statistics.tributesSent + "[/color] / " +
				BOUGHT_COLOR + playerState.statistics.tributesReceived + "[/color]";

			exchangedFood.caption = SOLD_COLOR + '+' + playerState.statistics.resourcesBought.food
				+ '[/color] ' + BOUGHT_COLOR + '-' + playerState.statistics.resourcesSold.food + '[/color]';
			exchangedWood.caption = SOLD_COLOR + '+' + playerState.statistics.resourcesBought.wood
				+ '[/color] ' + BOUGHT_COLOR + '-' + playerState.statistics.resourcesSold.wood + '[/color]';
			exchangedStone.caption = SOLD_COLOR + '+' + playerState.statistics.resourcesBought.stone
				+ '[/color] ' + BOUGHT_COLOR + '-' + playerState.statistics.resourcesSold.stone + '[/color]';
			exchangedMetal.caption = SOLD_COLOR + '+' + playerState.statistics.resourcesBought.metal
				+ '[/color] ' + BOUGHT_COLOR + '-' + playerState.statistics.resourcesSold.metal + '[/color]';
			var totalBought = 0;
			for each (var boughtAmount in playerState.statistics.resourcesBought)
				totalBought += boughtAmount;
			var totalSold = 0;
			for each (var soldAmount in playerState.statistics.resourcesSold)
				totalSold += soldAmount;
			barterEfficiency.caption = Math.floor(totalSold > 0 ? (totalBought / totalSold) * 100 : 0) + "%";
			tradeIncome.caption = playerState.statistics.tradeIncome;
		}
		else
		{
			// hide player boxes
			for (var k = 0; k < panelNames.length; ++k)
			{
				var playerBox = Engine.GetGUIObjectByName("playerBox"+k+"["+i+"]");
				playerBox.hidden = true;
			}
		}
	}

	selectPanel(0);
}

function onTick()
{
}

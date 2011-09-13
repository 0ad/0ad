// Max player slots for any map (should read from config)
const MAX_SLOTS = 8;

var panelNames = [ 'unitsBuildingsPanel', 'conquestPanel', 'resourcesPanel' ];
var panelButtonNames = [ 'unitsBuildingsPanelButton', 'conquestPanelButton', 'resourcesPanelButton' ];

/**
 * @param time Time period in milliseconds (integer)
 * @return String representing time period
 */
function timeToString(time)
{
	var hours   = Math.floor(time / 1000 / 60 / 60);
	var minutes = Math.floor(time / 1000 / 60) % 60;
	var seconds = Math.floor(time / 1000) % 60;
	return hours + ':' + (minutes < 10 ? '0' + minutes : minutes) + ':' + (seconds < 10 ? '0' + seconds : seconds);
}

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
			getGUIObjectByName(panelNames[i]).hidden = true;
			getGUIObjectByName(panelButtonNames[i]).sprite = "BackgroundTab";
		}
		else
		{
			getGUIObjectByName(panelNames[i]).hidden = false;
			getGUIObjectByName(panelButtonNames[i]).sprite = "ForegroundTab";
                        adjustTabDividers(getGUIObjectByName(panelButtonNames[i]).size);
		}
	}
}

function adjustTabDividers(tabSize)
{
    var leftSpacer = getGUIObjectByName("tabDividerLeft");
    var rightSpacer = getGUIObjectByName("tabDividerRight");
    leftSpacer.size = "20 " + leftSpacer.size.top + " " + (tabSize.left + 2) + " " + leftSpacer.size.bottom;
    rightSpacer.size = (tabSize.right - 2) + " " + rightSpacer.size.top + " 100%-20 " + rightSpacer.size.bottom;
}

function init(data)
{
	var civData = loadCivData();

	getGUIObjectByName("timeElapsed").caption = "Time elapsed: " + timeToString(data.timeElapsed);

	getGUIObjectByName("summaryText").caption = data.gameResult;

	// Space player boxes
	var boxSpacing = 32;
	for (var i = 0; i < panelNames.length; ++i)
        {
		for (var j = 0; j < MAX_SLOTS; ++j)
		{
			var box = getGUIObjectByName("playerBox"+i+"["+j+"]");
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
	getGUIObjectByName("playerName0Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	getGUIObjectByName("unitsTrainedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("unitsLostHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("enemyUnitsKilledHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("buildingsConstructedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("buildingsLostHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("enemyBuildingsDestroyedHeading").size = left +  " 6 " + (left + width) + " 100%"; left += width;

	left = 50;
	getGUIObjectByName("playerName1Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	getGUIObjectByName("civCentresBuiltHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("enemyCivCentresDestroyedHeading").size = left +  " 6 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("mapExplorationHeading").size = left +  " 6 " + (left + width) + " 100%"; left += width;

	left = 50;
	getGUIObjectByName("playerName2Heading").size = left + " 26 " + (left + playerNameHeadingWidth) + " 100%"; left += playerNameHeadingWidth;
	getGUIObjectByName("foodGatheredHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("vegetarianRatioHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("woodGatheredHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("metalGatheredHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("stoneGatheredHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;
	getGUIObjectByName("treasuresCollectedHeading").size = left + " 16 " + (left + width) + " 100%"; left += width;

	// Show counters
	for (var i = 0; i < MAX_SLOTS; ++i)
	{
		if (i < maxPlayers)
		{
			var playerState = data.playerStates[i+1];

			for (var k = 0; k < panelNames.length; ++k)
			{
				var playerBox = getGUIObjectByName("playerBox"+k+"["+i+"]");
				playerBox.hidden = false;

				var colourString = "colour: "
					+ Math.floor(playerState.colour.r * 255) + " "
					+ Math.floor(playerState.colour.g * 255) + " "
					+ Math.floor(playerState.colour.b * 255);
				playerBox.sprite = colourString + " 32";
				var playerColourBox = getGUIObjectByName("playerColourBox"+k+"["+i+"]");
				playerColourBox.sprite = colourString + " 255";

				var playerName = getGUIObjectByName("playerName"+k+"["+i+"]");
				playerName.caption = playerState.name;

				getGUIObjectByName("civIcon"+k+"["+i+"]").sprite = "stretched:"+civData[playerState.civ].Emblem;
			}

			var unitsTrained = getGUIObjectByName("unitsTrained["+i+"]");
			var unitsLost = getGUIObjectByName("unitsLost["+i+"]");
			var enemyUnitsKilled = getGUIObjectByName("enemyUnitsKilled["+i+"]");
			var buildingsConstructed = getGUIObjectByName("buildingsConstructed["+i+"]");
			var buildingsLost = getGUIObjectByName("buildingsLost["+i+"]");
			var enemyBuildingsDestroyed = getGUIObjectByName("enemyBuildingsDestroyed["+i+"]");

			var civCentresBuilt = getGUIObjectByName("civCentresBuilt["+i+"]");
			var enemyCivCentresDestroyed = getGUIObjectByName("enemyCivCentresDestroyed["+i+"]");
			var mapExploration = getGUIObjectByName("mapExploration["+i+"]");

			var foodGathered = getGUIObjectByName("foodGathered["+i+"]");
			var vegetarianRatio = getGUIObjectByName("vegetarianRatio["+i+"]");
			var woodGathered = getGUIObjectByName("woodGathered["+i+"]");
			var metalGathered = getGUIObjectByName("metalGathered["+i+"]");
			var stoneGathered = getGUIObjectByName("stoneGathered["+i+"]");
			var treasuresCollected = getGUIObjectByName("treasuresCollected["+i+"]");

			// align counters

			left = 240;
			width = 100;
			unitsTrained.size = left + " 2 " + (left + width) + " 100%"; left += width;
			unitsLost.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyUnitsKilled.size = left + " 2 " + (left + width) + " 100%"; left += width;
			buildingsConstructed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			buildingsLost.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyBuildingsDestroyed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			size = getGUIObjectByName("playerBox0["+i+"]").size;
			size.right = left + 10;
			getGUIObjectByName("playerBox0["+i+"]").size = size;

			left = 240;
			civCentresBuilt.size = left + " 2 " + (left + width) + " 100%"; left += width;
			enemyCivCentresDestroyed.size = left + " 2 " + (left + width) + " 100%"; left += width;
			mapExploration.size = left + " 2 " + (left + width) + " 100%"; left += width;
			size = getGUIObjectByName("playerBox1["+i+"]").size;
			size.right = left + 10;
			getGUIObjectByName("playerBox1["+i+"]").size = size;

			left = 240;
			foodGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			vegetarianRatio.size = left + " 2 " + (left + width) + " 100%"; left += width;
			woodGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			metalGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			stoneGathered.size = left + " 2 " + (left + width) + " 100%"; left += width;
			treasuresCollected.size	= left + " 2 " + (left + width) + " 100%"; left += width;
			size = getGUIObjectByName("playerBox2["+i+"]").size;
			size.right = left + 10;
			getGUIObjectByName("playerBox2["+i+"]").size = size;

			// display counters
			unitsTrained.caption = playerState.statistics.unitsTrained;
			unitsLost.caption = playerState.statistics.unitsLost;
			enemyUnitsKilled.caption = playerState.statistics.enemyUnitsKilled;
			buildingsConstructed.caption = playerState.statistics.buildingsConstructed;
			buildingsLost.caption = playerState.statistics.buildingsLost;
			enemyBuildingsDestroyed.caption = playerState.statistics.enemyBuildingsDestroyed;

			civCentresBuilt.caption = playerState.statistics.civCentresBuilt;
			enemyCivCentresDestroyed.caption = playerState.statistics.enemyCivCentresDestroyed;
			mapExploration.caption = playerState.statistics.percentMapExplored + "%";

			foodGathered.caption = playerState.statistics.resourcesGathered.food;
			vegetarianRatio.caption = Math.floor(playerState.statistics.resourcesGathered.food > 0 ?
				(playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100 : 0) + "%";
			woodGathered.caption = playerState.statistics.resourcesGathered.wood;
			metalGathered.caption = playerState.statistics.resourcesGathered.metal;
			stoneGathered.caption = playerState.statistics.resourcesGathered.stone;
			treasuresCollected.caption = playerState.statistics.treasuresCollected;
		}
		else
		{
			// hide player boxes
			for (var k = 0; k < panelNames.length; ++k)
			{
				var playerBox = getGUIObjectByName("playerBox"+k+"["+i+"]");
				playerBox.hidden = true;
			}
		}
	}

        selectPanel(0);
}

function onTick()
{
	// Update music state
	global.music.updateTimer();
}
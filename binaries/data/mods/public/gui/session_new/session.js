// Cache dev-mode settings that are frequently or widely used
var g_DevSettings = {
	controlAll: false
};

function init(initData, hotloadData)
{
	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else 
	{
		// Starting for the first time:
		startMusic();
	}

	onSimulationUpdate();
}

function leaveGame()
{
	stopMusic();
	endGame();
	Engine.SwitchGuiPage("page_pregame.xml");
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}

function changeNetStatus(status)
{
	var obj = getGUIObjectByName("netStatus");
	switch (status)
	{
	case "normal":
		obj.caption = "";
		obj.hidden = true;
		break;
	case "waiting_for_connect":
		obj.caption = "Waiting for other players to connect";
		obj.hidden = false;
		break;
	default:
		error("Unexpected net status "+status);
	}
}

function onTick()
{
	g_DevSettings.controlAll = getGUIObjectByName("devControlAll").checked;
	// TODO: at some point this controlAll needs to disable the simulation code's
	// player checks (once it has some player checks)

	updateCursor();

	// If the selection changed, we need to regenerate the sim display
	if (g_Selection.dirty)
		onSimulationUpdate();
}

function onSimulationUpdate()
{
	g_Selection.dirty = false;
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!simState)
		return;

	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails();
}

function updateDebug(simState)
{
	var debug = getGUIObjectByName("debug");

	if (getGUIObjectByName("devDisplayState").checked)
	{
		debug.hidden = false;
	}
	else
	{
		debug.hidden = true;
		return;
	}

	var text = uneval(simState);
	
	var selection = g_Selection.toList();
	if (selection.length)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[g_Selection.getPrimary()]);
		if (entState)
		{
			var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
			text += "\n\n" + uneval(entState) + "\n\n" + uneval(template);
		}
	}

	debug.caption = text;
}

function updatePlayerDisplay(simState)
{
	var playerState = simState.players[Engine.GetPlayerID()];
	
	if (!playerState)
		return;

	getGUIObjectByName("resourceFood").caption = playerState.resourceCounts.food;
	getGUIObjectByName("resourceWood").caption = playerState.resourceCounts.wood;
	getGUIObjectByName("resourceStone").caption = playerState.resourceCounts.stone;
	getGUIObjectByName("resourceMetal").caption = playerState.resourceCounts.metal;
	getGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;
}


//-------------------------------- -------------------------------- -------------------------------- 
// Utility functions
//-------------------------------- -------------------------------- -------------------------------- 

function damageTypesToTextStacked(dmg)
{
	if (!dmg)
		return "(None)";
	return dmg.hack + " Hack\n" + dmg.pierce + " Pierce\n" + dmg.crush + " Crush";
}

function damageTypesToText(dmg)
{
	if (!dmg)
		return "[font=\"serif-12\"](None)[/font]";
	
	var hackLabel = "[font=\"serif-12\"] Hack[/font]";
	var pierceLabel = "[font=\"serif-12\"] Pierce[/font]";
	var crushLabel = "[font=\"serif-12\"] Crush[/font]";
	var hackDamage = dmg.hack;
	var pierceDamage = dmg.pierce;
	var crushDamage = dmg.crush;
	
	var dmgArray = [];
	(hackDamage?  dmgArray.push(hackDamage +  hackLabel) : "");
	(pierceDamage?  dmgArray.push(pierceDamage +  pierceLabel) : "");
	(crushDamage?  dmgArray.push(crushDamage  +  crushLabel) : "");

	return dmgArray.join(", ");
}

function isUnitElite(templateName)
{
	var eliteStatus = false;
	var firstWord = templateName.substring(0, templateName.search("/"));
	var endsWith = templateName.substring(templateName.length-2, templateName.length);

	if (firstWord == "units" && endsWith == "_e")
		eliteStatus = true;

	return eliteStatus;
}

function getFullName(template)
{
		var name;
		
		if ((template.name.specific && template.name.generic) && (template.name.specific != template.name.generic))
			name = template.name.specific + " (" + template.name.generic + ")";
		else
			name = template.name.specific || template.name.generic || "???";
		
		return "[font=\"serif-bold-16\"]" + name + "[/font]";
}

function createIconTooltip(entState, template)
{
	var tooltip = "";
	tooltip = getFullName(template);
	
	var hitpointsLabel = "[font=\"serif-bold-13\"]Hitpoints: [/font]";
	tooltip += "\n" + hitpointsLabel + entState.hitpoints + "/" + entState.maxHitpoints;
	
	var attackLabel = "[font=\"serif-bold-13\"]Attack: [/font]";
	var armourLabel = "[font=\"serif-bold-13\"]Armour: [/font]";
	tooltip += "\n" + attackLabel + damageTypesToText(entState.attack) + "\n" + armourLabel + damageTypesToText(entState.armour);
	
	return tooltip;
}

//-------------------------------- -------------------------------- -------------------------------- 
//  Menu Functions
//-------------------------------- -------------------------------- -------------------------------- 

function toggleDeveloperOverlay()
{
	if (getGUIObjectByName("devCommands").hidden)
		getGUIObjectByName("devCommands").hidden = false; // show overlay
	else
		getGUIObjectByName("devCommands").hidden = true; // hide overlay
}

function toggleSettingsWindow()
{
	if (getGUIObjectByName("settingsWindow").hidden)
	{
		getGUIObjectByName("settingsWindow").hidden = false; // show settings
		setPaused(true);
	}
	else
	{
		getGUIObjectByName("settingsWindow").hidden = true; // hide settings
		setPaused(false);
	}

	getGUIObjectByName("menu").hidden = true; // Hide menu
}

function togglePause()
{
	if (getGUIObjectByName("pauseOverlay").hidden)
	{
		getGUIObjectByName("pauseOverlay").hidden = false; // pause game
		setPaused(true);
	}
	else
	{
		getGUIObjectByName("pauseOverlay").hidden = true; // unpause game
		setPaused(false);
	}

	getGUIObjectByName("menu").hidden = true; // Hide menu
}

function toggleMenu()
{
	if (getGUIObjectByName("menu").hidden)
		getGUIObjectByName("menu").hidden = false; // View menu
	else
		getGUIObjectByName("menu").hidden = true; // Hide menu
}

//-------------------------------- -------------------------------- -------------------------------- 
//  View / Hide Details Panel and Commands Panel information
//-------------------------------- -------------------------------- -------------------------------- 

// Hides Details Panel's Information
function hideSelectionDetails(booleanValue)
{
	getGUIObjectByName("selectionDetailsIcon").hidden = booleanValue;
	getGUIObjectByName("selectionDetailsHealth").hidden = booleanValue;
	getGUIObjectByName("selectionDetailsStamina").hidden = booleanValue;
	getGUIObjectByName("selectionDetailsMainText").hidden = booleanValue;
	getGUIObjectByName("selectionDetailsAttack").hidden = booleanValue;
	getGUIObjectByName("selectionDetailsArmour").hidden = booleanValue;
	getGUIObjectByName("unitSelectionPanel").hidden = booleanValue;
	getGUIObjectByName("selectionProductLogo").hidden = !booleanValue; // gets opposite of booleanValue
}

// Hides Commands Panel's Information
function hideCommands(booleanValue)
{
	getGUIObjectByName("unitConstructionPanel").hidden = booleanValue;
	getGUIObjectByName("unitStancePanel").hidden = booleanValue;
	getGUIObjectByName("unitFormationPanel").hidden = booleanValue;
	getGUIObjectByName("unitResearchPanel").hidden = booleanValue;
	getGUIObjectByName("unitTrainingPanel").hidden = booleanValue;
	getGUIObjectByName("unitQueuePanel").hidden = booleanValue;
}

//-------------------------------- -------------------------------- -------------------------------- 
//  Details Panel layout
//-------------------------------- -------------------------------- -------------------------------- 

const resourceIconCellIds = {food : 0, wood : 1, stone : 2, metal : 3};

// Multiple Selection Layout
function selectionLayoutMultiple()
{
		getGUIObjectByName("selectionDetailsMainText").size = "80 100%-70 100%-14 100%-10";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 6 100% 30";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 34 100% 100%-8";

		getGUIObjectByName("selectionDetailsIcon").size = "10 100%-74 66 100%-18";
		getGUIObjectByName("selectionDetailsHealth").size = "10 100%-16 66 100%-12";
		getGUIObjectByName("selectionDetailsStamina").size = "10 100%-10 66 100%-6";

		getGUIObjectByName("selectionDetailsAttack").hidden = true;
		getGUIObjectByName("selectionDetailsArmour").hidden = true;

		getGUIObjectByName("selectionDetailsMainText").sprite = "goldPanel";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "";
}

// Single Selection Layout
function selectionLayoutSingle()
{
		getGUIObjectByName("selectionDetailsMainText").size = "6 0 100%-6 50";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 0 100% 30";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 30 100% 50";

		getGUIObjectByName("selectionDetailsIcon").size = "10 100%-104 90 100%-22";
		getGUIObjectByName("selectionDetailsHealth").size = "10 100%-20 90 100%-14";
		getGUIObjectByName("selectionDetailsStamina").size = "10 100%-12 90 100%-6";

		getGUIObjectByName("selectionDetailsAttack").size = "104 64 100% 100%-30";
		getGUIObjectByName("selectionDetailsArmour").size = "204 64 100% 100%-30";

		getGUIObjectByName("selectionDetailsAttack").hidden = false;
		getGUIObjectByName("selectionDetailsArmour").hidden = false;

		getGUIObjectByName("selectionDetailsMainText").sprite = "";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "wheatWindowTitle";
}

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0 };

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue", "Selection"];

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var i = 0;

	for each (var item in items)
	{
		if (i > 15) // End loop early if more than 16 buttons
			break;

		// Get templates
		var entType;
		if (guiName == "Queue")
			entType = item.template;
		else
			entType = item;
			
		var template = Engine.GuiInterfaceCall("GetTemplateData", entType);
		if (!template)
			continue; // ignore attempts to use invalid templates (an error should have been reported already)

		// Name
		var name;
		if (guiName == "Selection")
			name = template.name.specific || template.name.generic || "???";
		else
			name = getFullName(template);

		// Tooltip
		var tooltip = (isUnitElite(entType)?  "Elite " + name : name ); // "Elite " is not formatted in bold, so may need custom versions of this later
		
		if (guiName == "Selection")
		{
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = 
				(g_Selection.groups.groupTypeCount[item] > 1 ? g_Selection.groups.groupTypeCount[item] : "");

			tooltip += (g_Selection.groups.groupTypeCount[item] > 1 ? " (" + g_Selection.groups.groupTypeCount[item] + ")" : "")
		}
		else if (guiName == "Queue")
		{
			var progress = Math.round(item.progress*100) + "%";
			tooltip += " - " + progress;
			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");
			getGUIObjectByName("unit"+guiName+"Progress["+i+"]").caption = (item.progress ? progress : "");
		}
		else if (guiName == "Construction" || guiName == "Training")
		{
			if (template.cost)
			{
				var costs = [];
				if (template.cost.food) costs.push("[font=\"serif-bold-13\"]Food:[/font] " + template.cost.food);
				if (template.cost.wood) costs.push("[font=\"serif-bold-13\"]Wood:[/font] " + template.cost.wood);
				if (template.cost.metal) costs.push("[font=\"serif-bold-13\"]Metal:[/font] " + template.cost.metal);
				if (template.cost.stone) costs.push("[font=\"serif-bold-13\"]Stone:[/font] " + template.cost.stone);
				if (costs.length)
					tooltip += "\n" + costs.join(", ");
			}
		
			if (guiName == "Training")
			{
				var [batchSize, batchIncrement] = getTrainingQueueBatchStatus(unitEntState.id, entType);
				tooltip += "\n[font=\"serif-13\"]";
				if (batchSize) tooltip += "Training [font=\"serif-bold-13\"]" + batchSize + "[font=\"serif-13\"] units; ";
					tooltip += "Shift-click to train [font=\"serif-bold-13\"]"+ (batchSize+batchIncrement) + "[font=\"serif-13\"] units[/font]";
			}	
		}

		// Button
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip;

		if (callback != null)
			button.onpress = (function(e) { return function() { callback(e) } })(item); // (need nested functions to get the closure right)

		icon.sprite = "snPortraitSheetHele"; // TODO
		if (typeof template.icon_cell == "undefined")
			icon.cell_id = 0;
		else
			icon.cell_id = template.icon_cell;
			
		++i;
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = ((guiName == "Selection")? 37 : 45);
	var numButtons = i;
	var j = 0; // index for second row of buttons
	
	for (i = 0; i < numButtons; ++i)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;

		if (i > 7) // Make second row
		{
			if (guiName == "Queue")
				getGUIObjectByName("unit"+guiName+"Panel").size = "0 -104 100% 100%-166"
				
			size.left = buttonSpacer*j;
			size.right = buttonSpacer*j + buttonSideLength;
			size.top = buttonSpacer;
			size.bottom = buttonSpacer + buttonSideLength;
			j++;
		}
		else // Make first row
		{
			if ((guiName == "Queue"))
				getGUIObjectByName("unit"+guiName+"Panel").size = "0 -60 100% 100%-166"
		
			size.left = buttonSpacer*i;
			size.right = buttonSpacer*i + size.bottom;
		}

		button.size = size;
	}
	
	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

// Fills out information that most entities have
function displayGeneralInfo(entState, template)
{
	var iconTooltip = "";

	// Is unit Elite?
	var eliteStatus = isUnitElite(entState.template);
	
	// Specific Name
	var name = (eliteStatus?  "Elite " + template.name.specific : template.name.specific);
	getGUIObjectByName("selectionDetailsSpecific").caption = name;
	iconTooltip += "[font=\"serif-bold-16\"]" + name + "[/font]";

	// Generic Name
	if (template.name.generic != template.name.specific)
		getGUIObjectByName("selectionDetailsSpecific").tooltip = template.name.generic;
	else
		getGUIObjectByName("selectionDetailsSpecific").tooltip = "";

	// Player Name
	getGUIObjectByName("selectionDetailsPlayer").caption = "Player " + entState.player; // TODO: get player name

	// Hitpoints
	if (entState.hitpoints != undefined)
	{
		var healthSize = getGUIObjectByName("selectionDetailsHealthBar").size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		getGUIObjectByName("selectionDetailsHealthBar").size = healthSize;
		
		var tooltipHitPoints = "[font=\"serif-bold-13\"]Hitpoints [/font]" + entState.hitpoints + "/" + entState.maxHitpoints;
		getGUIObjectByName("selectionDetailsHealth").tooltip = tooltipHitPoints;
		getGUIObjectByName("selectionDetailsHealth").hidden = false;
		iconTooltip += "\n" + tooltipHitPoints;
	}
	else
	{
		getGUIObjectByName("selectionDetailsHealth").hidden = true;
		getGUIObjectByName("selectionDetailsHealth").tooltip = "";
	}

	// Attack stats
	getGUIObjectByName("selectionDetailsAttackStats").caption = damageTypesToTextStacked(entState.attack);
	if (entState.attack)
		iconTooltip += "\n" +  "[font=\"serif-bold-13\"]Attack: [/font]" + damageTypesToText(entState.attack);		

	// Armour stats
	getGUIObjectByName("selectionDetailsArmourStats").caption = damageTypesToTextStacked(entState.armour);
	if (entState.armour)
		iconTooltip += "\n" + "[font=\"serif-bold-13\"]Armour: [/font]" + damageTypesToText(entState.armour);	

	// Is this a Gaia unit?
	var firstWord = entState.template.substring(0, entState.template.search("/"));
	if (firstWord == "gaia")
	{
		getGUIObjectByName("selectionDetailsAttack").hidden = true;
		getGUIObjectByName("selectionDetailsArmour").hidden = true;
	}

	// Resource stats
	if (entState.resourceSupply)
	{
		var resources = entState.resourceSupply.amount + "/" + entState.resourceSupply.max + " ";
		var resourceType = entState.resourceSupply.type["generic"];
		
		getGUIObjectByName("selectionDetailsResourceStats").caption =  resources;
		getGUIObjectByName("selectionDetailsResourceIcon").cell_id = resourceIconCellIds[resourceType];
		getGUIObjectByName("selectionDetailsResources").hidden = false;
		
		iconTooltip += "\n[font=\"serif-bold-13\"]Resources: [/font]" + resources + "[font=\"serif-12\"]" + resourceType + "[/font]";
	}
	else
	{
		getGUIObjectByName("selectionDetailsResources").hidden = true;
	}				

	// Icon
	getGUIObjectByName("selectionDetailsIconImage").tooltip = iconTooltip;
	getGUIObjectByName("selectionDetailsIconImage").sprite = "snPortraitSheetHele";
	getGUIObjectByName("selectionDetailsIconImage").cell_id = template.icon_cell;
}

function updateSelectionDetails()
{
	var detailsPanel = getGUIObjectByName("selectionDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	var selection = g_Selection.toList();
	if (selection.length == 0)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	/* If the unit has no data (e.g. it was killed), don't try displaying any
	 data for it. (TODO: it should probably be removed from the selection too;
	 also need to handle multi-unit selections) */
	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[g_Selection.getPrimary()]);
	if (!entState)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);

	// Different selection details are shown based on whether multiple units or a single unit is selected
	if (selection.length > 1)
		selectionLayoutMultiple();
	else
		selectionLayoutSingle();

	// Fill out general info and display it
	displayGeneralInfo(entState, template); // must come after layout functions

	// Show Panels
	detailsPanel.hidden = false;
	
	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(commandsPanel,  selection, entState);
}

//  Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(commandsPanel,  selection, entState)
{
	// Panels that are active
	var usedPanels = {};

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (entState.attack) // TODO - this should be based on some AI properties
		{
			//usedPanels["Stance"] = 1;
			//usedPanels["Formation"] = 1;
			// (These are disabled since they're not implemented yet)
		}
		else // TODO - this should be based on various other things
		{
			//usedPanels["Research"] = 1;
		}

		if (entState.buildEntities && entState.buildEntities.length)
			setupUnitPanel("Construction", usedPanels, entState, entState.buildEntities, startBuildingPlacement);

		if (entState.training && entState.training.entities.length)
			setupUnitPanel("Training", usedPanels, entState, entState.training.entities,
				function (trainEntType) { addToTrainingQueue(entState.id, trainEntType); } );

		if (entState.training && entState.training.queue.length)
			setupUnitPanel("Queue", usedPanels, entState, entState.training.queue,
				function (item) { removeFromTrainingQueue(entState.id, item.id); } );

		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, entState, g_Selection.groups.groupTemplates,
				function (entType) { changePrimarySelectionGroup(entType); } );

		// Stamina
		//	if (entState.stamina != undefined)
			getGUIObjectByName("selectionDetailsStamina").hidden = false;
		//	else
		//		getGUIObjectByName("selectionDetailsStamina").hidden = true;
				
		commandsPanel.hidden = false;
	}
	else
	{
		getGUIObjectByName("selectionDetailsStamina").hidden = true;
		commandsPanel.hidden = true;
	}

	// Hides / unhides Unit Panels (panels should be grouped by type, not by order, but we will leave that for another time)
	var offset = 0;
	for each (var panelName in g_unitPanels)
	{
		var panel = getGUIObjectByName("unit" + panelName + "Panel");
		if (usedPanels[panelName])
		{
//			var size = panel.size;
//			var h = size.bottom - size.top;
//			size.bottom = offset;
//			size.top = offset - h;
//			panel.size = size;
			panel.hidden = false;
//			offset -= (h + 6); // changed 12 point spacing to 6 point:   offset -= (h + 12);
		}
		else
		{
			panel.hidden = true;
		}
	}
}


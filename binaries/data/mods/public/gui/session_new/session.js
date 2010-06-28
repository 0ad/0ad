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
	updateUnitDisplay();
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
	
	var hackLabel = "[font=\"serif-12\"] Hack, [/font]";
	var pierceLabel = "[font=\"serif-12\"] Pierce, [/font]";
	var crushLabel = "[font=\"serif-12\"] Crush[/font]";
	var hackDamage = dmg.hack;
	var pierceDamage = dmg.pierce;
	var crushDamage = dmg.crush;
	
	return  hackDamage + hackLabel + pierceDamage + pierceLabel + crushDamage + crushLabel;
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

// Multiple Selection Layout
function selectionLayoutMultiple()
{
		getGUIObjectByName("selectionDetailsMainText").size = "70 100%-70 100%-2 100%-10";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 0 100% 24";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 30 100% 50";

		getGUIObjectByName("selectionDetailsIcon").size = "0 100%-74 56 100%-18";
		getGUIObjectByName("selectionDetailsHealth").size = "0 100%-16 56 100%-12";
		getGUIObjectByName("selectionDetailsStamina").size = "0 100%-10 56 100%-6";
			
		getGUIObjectByName("selectionDetailsAttack").hidden = true;
		getGUIObjectByName("selectionDetailsArmour").hidden = true;	
			
		getGUIObjectByName("selectionDetailsMainText").sprite = "goldPanel";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "";
}

// Single Selection Layout
function selectionLayoutSingle()
{
		getGUIObjectByName("selectionDetailsMainText").size = "-8 -10 100%+8 56";
		getGUIObjectByName("selectionDetailsSpecific").size = "0 0 100% 30";
		getGUIObjectByName("selectionDetailsPlayer").size = "0 30 100% 56";

		getGUIObjectByName("selectionDetailsIcon").size = "2 100%-104 82 100%-22";
		getGUIObjectByName("selectionDetailsHealth").size = "2 100%-20 82 100%-14";
		getGUIObjectByName("selectionDetailsStamina").size = "2 100%-12 82 100%-6";

		getGUIObjectByName("selectionDetailsAttack").size = "88 72 100% 100%";
		getGUIObjectByName("selectionDetailsArmour").size = "186 72 100% 100%";
		getGUIObjectByName("selectionDetailsAttack").hidden = false;
		getGUIObjectByName("selectionDetailsArmour").hidden = false;

		getGUIObjectByName("selectionDetailsMainText").sprite = "";
		getGUIObjectByName("selectionDetailsSpecific").sprite = "wheatWindowTitle";
}

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0 };

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue", "Selection"];

//-------------------------------- -------------------------------- -------------------------------- 
// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
//-------------------------------- -------------------------------- -------------------------------- 
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
	var buttonSpacer = ((guiName == "Selection")? 35 : 45);
	var numButtons = i;
	var j = 0; // index for second row of buttons
	
	for (i = 0; i < numButtons; ++i)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;

		if (i > 7) // Make second row
		{
			size.left = buttonSpacer*j;
			size.right = buttonSpacer*j + buttonSideLength;
			size.top = buttonSpacer;
			size.bottom = buttonSpacer + buttonSideLength;
			j++;
		}
		else // Make first row
		{
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

//-------------------------------- -------------------------------- -------------------------------- 
// Updates middle Selection Details Panel - runs in the main session loop
//-------------------------------- -------------------------------- -------------------------------- 
function updateUnitDisplay()
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
	var iconTooltip = "";

	// Hitpoints
	if (entState.hitpoints != undefined)
	{
		var healthSize = getGUIObjectByName("selectionDetailsHealthBar").size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		getGUIObjectByName("selectionDetailsHealthBar").size = healthSize;
		getGUIObjectByName("selectionDetailsHealth").tooltip = "Hitpoints " + entState.hitpoints + " / " + entState.maxHitpoints;
		getGUIObjectByName("selectionDetailsHealth").hidden = false;
	}
	else
	{
		getGUIObjectByName("selectionDetailsHealth").hidden = true;
		getGUIObjectByName("selectionDetailsHealth").tooltip = "";
	}

	// Is unit Elite?
	var eliteStatus = isUnitElite(entState.template);
	
	// Specific Name
	getGUIObjectByName("selectionDetailsSpecific").caption = (eliteStatus?  "Elite " + template.name.specific : template.name.specific );
	
	// Generic Name
	if (template.name.generic == template.name.specific)
	{
		getGUIObjectByName("selectionDetailsGeneric").hidden = true;
		getGUIObjectByName("selectionDetailsSpecific").tooltip = "";
		//iconTooltip += template.name.specific;
	}
	else
	{
		getGUIObjectByName("selectionDetailsSpecific").tooltip = template.name.generic;
		//iconTooltip += template.name.specific + " (" + template.name.generic + ")";
	}

	// Player Name
	getGUIObjectByName("selectionDetailsPlayer").caption = "Player " + entState.player; // TODO: get player name

	// Icon
	iconTooltip += (eliteStatus? "[font=\"serif-bold-16\"]Elite [/font]" : "");
	iconTooltip += createIconTooltip(entState, template);
	getGUIObjectByName("selectionDetailsIconImage").tooltip = iconTooltip;
	getGUIObjectByName("selectionDetailsIconImage").sprite = "snPortraitSheetHele";
	getGUIObjectByName("selectionDetailsIconImage").cell_id = template.icon_cell;

	// Attack and Armour Stats
	getGUIObjectByName("selectionDetailsAttackStats").caption = damageTypesToTextStacked(entState.attack);
	getGUIObjectByName("selectionDetailsArmourStats").caption = damageTypesToTextStacked(entState.armour);

	// Different selection details are shown based on whether multiple units or a single unit is selected
	if (selection.length > 1)
		selectionLayoutMultiple();
	else
		selectionLayoutSingle();

	// Show Panels
	detailsPanel.hidden = false;	
		
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

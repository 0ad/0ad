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

	// If we're called during init when the game is first loading, there will be
	// no simulation yet, so do nothing
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
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
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

function damageTypesToText(dmg)
{
	if (!dmg)
		return "(None)";
	return dmg.hack + " Hack\n" + dmg.pierce + " Pierce\n" + dmg.crush + " Crush";
}

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0 };

// The unitSomethingPanel objects, which are displayed in a stack at the bottom of the screen,
// ordered with *lowest* first
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue"];

// Helper function for updateUnitDisplay
function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var i = 0;
	for each (var item in items)
	{
		var entType;
		if (guiName == "Queue")
			entType = item.template;
		else
			entType = item;

		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");

		var template = Engine.GuiInterfaceCall("GetTemplateData", entType);
		if (!template)
			continue; // ignore attempts to use invalid templates (an error should have been reported already)

		var name;
		if (template.name.specific && template.name.generic)
			name = template.name.specific + " (" + template.name.generic + ")";
		else
			name = template.name.specific || template.name.generic || "???";

		var tooltip;
		if (guiName == "Queue")
		{
			var progress = Math.round(item.progress*100) + "%";
			tooltip = name + " - " + progress;

			getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");
			getGUIObjectByName("unit"+guiName+"Progress["+i+"]").caption = (item.progress ? progress : "");
		}
		else
		{
			tooltip = "[font=trebuchet14b]" + name + "[/font]";

			if (template.cost)
			{
				var costs = [];
				if (template.cost.food) costs.push("[font=tahoma10b]Food:[/font] " + template.cost.food);
				if (template.cost.wood) costs.push("[font=tahoma10b]Wood:[/font] " + template.cost.wood);
				if (template.cost.metal) costs.push("[font=tahoma10b]Metal:[/font] " + template.cost.metal);
				if (template.cost.stone) costs.push("[font=tahoma10b]Stone:[/font] " + template.cost.stone);
				if (costs.length)
					tooltip += "\n" + costs.join(", ");
			}

			if (guiName == "Training")
			{
				var [batchSize, batchIncrement] = getTrainingQueueBatchStatus(unitEntState.id, entType);
				tooltip += "\n[font=tahoma11]";
				if (batchSize) tooltip += "Training [font=tahoma12]" + batchSize + "[font=tahoma11] units; ";
				tooltip += "Shift-click to train [font=tahoma12]" + (batchSize+batchIncrement) + "[font=tahoma11] units[/font]";
			}
		}

		button.hidden = false;
		button.tooltip = tooltip;
		button.onpress = (function(e) { return function() { callback(e) } })(item);
			// (need nested functions to get the closure right)

		icon.sprite = "snPortraitSheetHele"; // TODO
		if (typeof template.icon_cell == "undefined")
			icon.cell_id = 0;
		else
			icon.cell_id = template.icon_cell;
		++i;
	}
	var numButtons = i;
	// Position the visible buttons
	// (TODO: if there's lots, maybe they should be squeezed together to fit)
	for (i = 0; i < numButtons; ++i)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;
		size.left = 40*i;
		size.right = 40*i + size.bottom;
		button.size = size;
	}

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;
	g_unitPanelButtons[guiName] = numButtons;
}

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

	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);

	// If the unit has no data (e.g. it was killed), don't try displaying any
	// data for it. (TODO: it should probably be removed from the selection too;
	// also need to handle multi-unit selections)
	if (!entState)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);

	detailsPanel.hidden = false;
	commandsPanel.hidden = false;

	getGUIObjectByName("selectionDetailsIcon").sprite = "snPortraitSheetHele";
	getGUIObjectByName("selectionDetailsIcon").cell_id = template.icon_cell;

	var healthSize = getGUIObjectByName("selectionDetailsHealthBar").size;
	healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
	getGUIObjectByName("selectionDetailsHealthBar").size = healthSize;
	getGUIObjectByName("selectionDetailsHealth").tooltip = "Hitpoints " + entState.hitpoints + " / " + entState.maxHitpoints;

	getGUIObjectByName("selectionDetailsSpecific").caption = template.name.specific;
	if (template.name.generic == template.name.specific)
	{
		getGUIObjectByName("selectionDetailsGeneric").hidden = true;
	}
	else
	{
		getGUIObjectByName("selectionDetailsGeneric").hidden = false;
		getGUIObjectByName("selectionDetailsGeneric").caption = template.name.generic;
	}

	getGUIObjectByName("selectionDetailsPlayer").caption = "Player " + entState.player; // TODO: get player name

	getGUIObjectByName("selectionDetailsAttack").caption = damageTypesToText(entState.attack);
	getGUIObjectByName("selectionDetailsArmour").caption = damageTypesToText(entState.armour);

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
	}

	// Lay out all the used panels in a stack at the bottom of the screen
	var offset = 0;
	for each (var panelName in g_unitPanels)
	{
		var panel = getGUIObjectByName("unit"+panelName+"Panel");
		if (usedPanels[panelName])
		{
			var size = panel.size;
			var h = size.bottom - size.top;
			size.bottom = offset;
			size.top = offset - h;
			panel.size = size;
			panel.hidden = false;
			offset -= (h + 12);
		}
		else
		{
			panel.hidden = true;
		}
	}
}

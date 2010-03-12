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

	onSimulationUpdate();
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
}

function onSimulationUpdate()
{
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

var g_unitConstructionButtons = 0; // the number currently visible

// The unitSomethingPanel objects, which are displayed in a stack at the bottom of the screen,
// ordered with *lowest* first
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue"];

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

	getGUIObjectByName("selectionDetailsAttack").caption = damageTypesToText(entState.attack);
	getGUIObjectByName("selectionDetailsArmour").caption = damageTypesToText(entState.armour);

	var usedPanels = {};

	if (entState.attack) // TODO - this should be based on some AI properties
	{
		usedPanels["Stance"] = 1;
		usedPanels["Formation"] = 1;
	}
	else // TODO - this should be based on various other things
	{
		usedPanels["Queue"] = 1;
		usedPanels["Training"] = 1;
		usedPanels["Research"] = 1;
	}

	// Set up the unit construction buttons
	// (TODO: abstract this to apply to the other button panels)
	if (entState.buildEntities && entState.buildEntities.length)
	{
		usedPanels["Construction"] = 1;
		var i = 0;
		for each (var build in entState.buildEntities)
		{
			var button = getGUIObjectByName("unitConstructionButton["+i+"]");
			var icon = getGUIObjectByName("unitConstructionIcon["+i+"]");

			var template = Engine.GuiInterfaceCall("GetTemplateData", build);

			var name;
			if (template.name.specific && template.name.generic)
				name = template.name.specific + " (" + template.name.generic + ")";
			else
				name = template.name.specific || template.name.generic || "???";

			var tooltip = "[font=trebuchet14b]" + name + "[/font]";

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

			button.hidden = false;
			button.tooltip = tooltip;
			button.onpress = (function(b) { return function() { testBuild(b) } })(build);
				// (need nested functions to get the closure right)

			icon.sprite = "snPortraitSheetHele";
			icon.cell_id = template.icon_cell;
			++i;
		}
		var numButtons = i;
		// Position the visible buttons
		// (TODO: if there's lots, maybe they should be squeezed together to fit)
		for (i = 0; i < numButtons; ++i)
		{
			var button = getGUIObjectByName("unitConstructionButton["+i+"]");
			var size = button.size;
			size.left = 40*i;
			size.right = 40*i + size.bottom;
			button.size = size;
		}

		// Hide any buttons we're no longer using
		for (i = numButtons; i < g_unitConstructionButtons; ++i)
			getGUIObjectByName("unitConstructionButton["+i+"]").hidden = true;
		g_unitConstructionButtons = numButtons;
	}

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

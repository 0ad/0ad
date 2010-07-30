// Constants used by the Queue or Garrison panel
const UNIT_PANEL_BASE = -56; // The offset above the main panel (will often be negative)
const UNIT_PANEL_HEIGHT = 47; // The height needed for a row of buttons

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = { "Construction": 0, "Training": 0, "Queue": 0, "Selection": 0 };

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Stance", "Formation", "Construction", "Research", "Training", "Queue", "Selection"];

// Lay out button rows
function layoutButtonRow(rowNumber, guiName, buttonSideLength, buttonSpacer, startIndex, endIndex)
{
	var colNumber = 0;

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var size = button.size;
		
		size.left = buttonSpacer*colNumber;
		size.right = buttonSpacer*colNumber + buttonSideLength;
		size.top = buttonSpacer*rowNumber;
		size.bottom = buttonSpacer*rowNumber + buttonSideLength;
		
		button.size = size;
		colNumber++;
	}
}

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
function setupUnitPanel(guiName, usedPanels, playerState, unitEntState, items, callback)
{
	usedPanels[guiName] = 1;
	var i = 0;
	var selection = g_Selection.toList();

	for each (var item in items)
	{
		// End loop early if there too many buttons
		if ((guiName != "Selection") && (i > 23))
			break;
		else if (i > 31)
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
			name = "[font=\"serif-bold-16\"]" + (template.name.specific || template.name.generic || "???") + "[/font]";
		else
			name = getFullName(template);

		// Tooltip
		var tooltip = name;

		if (guiName == "Selection")
		{
			var entState = Engine.GuiInterfaceCall("GetEntityState", selection[i]);
			if (!entState)
				return;

			// Rank Title
			var rankText = getRankTitle(getRankCellId(entState.template));
			rankText = (rankText? " [font=\"serif-bold-16\"](" + rankText + ")[/font]" : "" );
			tooltip += rankText;

			// Hitpoints
			if (entState.maxHitpoints != undefined)
			{
				var unitHealthBar = getGUIObjectByName("unitSelectionHealthForeground[" + i + "]");
				var healthSize = unitHealthBar.size;
				healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
				unitHealthBar.size = healthSize;
				tooltip += " [font=\"serif-9\"](" + entState.hitpoints + "/" + entState.maxHitpoints + ")[/font]";
			}
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

		// Button Function
		var parameter = ((guiName == "Selection")? i : item);
		button.onpress = (function(e) { return function() { callback(e) } })(parameter); // (need nested functions to get the closure right)

		// Get icon sheet
		icon.sprite = template.icon_sheet;

		if (typeof template.icon_cell == "undefined")
			icon.cell_id = 0;
		else
			icon.cell_id = template.icon_cell;
			
		++i;
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var numButtons = i;
	var rowLength = 8;
	var numRows = ceiling(numButtons / rowLength);
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = ((guiName == "Selection")? buttonSideLength+1 : buttonSideLength+2);

	// Resize Queue panel if needed
	if (guiName == "Queue") // or garrison
	{
		var panel = getGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Layout buttons
	for (var i = 0; i < numRows; i++)
		layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	g_unitPanelButtons[guiName] = numButtons;
}

// Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(playerState, entState, commandsPanel, selection)
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
			setupUnitPanel("Construction", usedPanels, playerState, entState, entState.buildEntities, startBuildingPlacement);

		if (entState.training && entState.training.entities.length)
			setupUnitPanel("Training", usedPanels, playerState, entState, entState.training.entities,
				function (trainEntType) { addToTrainingQueue(entState.id, trainEntType); } );

		if (entState.training && entState.training.queue.length)
			setupUnitPanel("Queue", usedPanels, playerState, entState, entState.training.queue,
				function (item) { removeFromTrainingQueue(entState.id, item.id); } );

		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, playerState, entState, g_Selection.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );
	
		commandsPanel.hidden = false;
	}
	else
	{
		getGUIObjectByName("sdStamina").hidden = true;
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

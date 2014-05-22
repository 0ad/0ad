// Panel types
const SELECTION = "Selection";
const QUEUE = "Queue";
const GARRISON = "Garrison";
const FORMATION = "Formation";
const TRAINING = "Training";
const RESEARCH = "Research";
const CONSTRUCTION = "Construction";
const TRADING = "Trading";
const COMMAND = "Command";
const STANCE = "Stance";
const GATE = "Gate";
const PACK = "Pack";

// Constants
const COMMANDS_PANEL_WIDTH = 228;
const UNIT_PANEL_BASE = -52; // QUEUE: The offset above the main panel (will often be negative)
const UNIT_PANEL_HEIGHT = 44; // QUEUE: The height needed for a row of buttons

// Trading constants
const TRADING_RESOURCES = ["food", "wood", "stone", "metal"];

// Barter constants
const BARTER_RESOURCE_AMOUNT_TO_SELL = 100;
const BARTER_BUNCH_MULTIPLIER = 5;
const BARTER_RESOURCES = ["food", "wood", "stone", "metal"];
const BARTER_ACTIONS = ["Sell", "Buy"];

// Gate constants
const GATE_ACTIONS = ["lock", "unlock"];

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Trading": 0, "Construction": 0, "Command": 0, "Stance": 0, "Gate": 0, "Pack": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Formation", "Garrison", "Training", "Barter", "Trading", "Construction", "Research", "Stance", "Command", "Gate", "Pack"];

// Indexes of resources to sell and buy on barter panel
var g_barterSell = 0;

// Lay out a row of centered buttons (does not work inside a loop like the other function)
function layoutButtonRowCentered(rowNumber, guiName, startIndex, endIndex, width)
{
	var buttonSideLength = Engine.GetGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;
	var colNumber = 0;

	// Collect buttons
	var buttons = [];
	var icons = [];

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = Engine.GetGUIObjectByName("unit"+guiName+"Icon["+i+"]");

		if (button)
		{
			buttons.push(button);
			icons.push(icon);
		}
	}

	// Location of middle button
	var middleIndex = Math.ceil(buttons.length/2);

	// Determine whether even or odd number of buttons
	var center = (buttons.length/2 == Math.ceil(buttons.length/2))? Math.ceil(width/2) : Math.ceil(width/2+buttonSpacer/2);

	// Left Side
	for (var i = middleIndex-1; i >= 0; i--)
	{
		if (buttons[i])
		{
			var icon = icons[i];
			var size = buttons[i].size;
			size.left = center - buttonSpacer*colNumber - buttonSideLength;
			size.right = center - buttonSpacer*colNumber;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;
			buttons[i].size = size;
			colNumber++;
		}
	}

	// Right Side
	center += 1; // add spacing to center buttons
	colNumber = 0; // reset to 0

	for (var i = middleIndex; i < buttons.length; i++)
	{
		if (buttons[i])
		{
			var icon = icons[i];
			var size = buttons[i].size;
			size.left = center + buttonSpacer*colNumber;
			size.right = center + buttonSpacer*colNumber + buttonSideLength;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;
			buttons[i].size = size;
			colNumber++;
		}
	}
}

// Lay out button rows
function layoutButtonRow(rowNumber, guiName, buttonSideWidth, buttonSpacer, startIndex, endIndex)
{
	layoutRow("Button", rowNumber, guiName, buttonSideWidth, buttonSpacer, buttonSideWidth, buttonSpacer, startIndex, endIndex);
}

// Lay out rows
function layoutRow(objectName, rowNumber, guiName, objectSideWidth, objectSpacerWidth, objectSideHeight, objectSpacerHeight, startIndex, endIndex)
{
	var colNumber = 0;

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = Engine.GetGUIObjectByName("unit"+guiName+objectName+"["+i+"]");

		if (button)
		{
			var size = button.size;

			size.left = objectSpacerWidth*colNumber;
			size.right = objectSpacerWidth*colNumber + objectSideWidth;
			size.top = objectSpacerHeight*rowNumber;
			size.bottom = objectSpacerHeight*rowNumber + objectSideHeight;

			button.size = size;
			colNumber++;
		}
	}
}

// Set the visibility of the object
function setOverlay(object, value)
{
	object.hidden = !value;
}

/**
 * Format entity count/limit message for the tooltip
 */
function formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers)
{
	if (trainEntLimit == undefined)
		return "";
	var text = "\n\n" + sprintf(translate("Current Count: %(count)s, Limit: %(limit)s."), { count: trainEntCount, limit: trainEntLimit });
	if (trainEntCount >= trainEntLimit)
		text = "[color=\"red\"]" + text + "[/color]";
	for (var c in trainEntLimitChangers)
	{
		if (trainEntLimitChangers[c] > 0)
			text += "\n" + sprintf(translate("%(changer)s enlarges the limit with %(change)s."), { changer: translate(c), change: trainEntLimitChangers[c] });
		else if (trainEntLimitChangers[c] < 0)
			text += "\n" + sprintf(translate("%(changer)s lessens the limit with %(change)s."), { changer: translate(c), change: (-trainEntLimitChangers[c]) });
	}
	return text;
}

/**
 * Format batch training string for the tooltip
 * Examples:
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 5"
 * buildingsCountToTrainFullBatch = 2, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 10 (2*5)"
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 15, remainderBatch = 12:
 * "Shift-click to train 27 (15 + 12)"
 */
function formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
{
	var totalBatchTrainingCount = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;
	// Don't show the batch training tooltip if either units of this type can't be trained at all
	// or only one unit can be trained
	if (totalBatchTrainingCount < 2)
		return "";
	var batchTrainingString = "";
	var fullBatchesString = "";
	if (buildingsCountToTrainFullBatch > 0)
	{
		if (buildingsCountToTrainFullBatch > 1)
			fullBatchesString = sprintf(translate("%(buildings)s*%(batchSize)s"), {
				buildings: buildingsCountToTrainFullBatch,
				batchSize: fullBatchSize
			});
		else
			fullBatchesString = fullBatchSize;
	}
	var remainderBatchString = remainderBatch > 0 ? remainderBatch : "";
	var batchDetailsString = "";
	var action = "[font=\"sans-bold-13\"]" + translate("Shift-click") + "[/font][font=\"sans-13\"]"

	// We need to display the batch details part if there is either more than
	// one building with full batch or one building with the full batch and
	// another with a partial batch
	if (buildingsCountToTrainFullBatch > 1 ||
		(buildingsCountToTrainFullBatch == 1 && remainderBatch > 0))
	{
		if (remainderBatch > 0)
			return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s + %(remainderBatch)s)."), {
				action: action,
				number: totalBatchTrainingCount,
				fullBatch: fullBatchesString,
				remainderBatch: remainderBatch
			}) + "[/font]";

		return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s)."), {
			action: action,
			number: totalBatchTrainingCount,
			fullBatch: fullBatchesString
		}) + "[/font]";
	}

	return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s."), {
		action: action,
		number: totalBatchTrainingCount
	}) + "[/font]";
}

function getStanceDisplayName(name)
{
	var displayName;
	switch(name)
	{
		case "violent":
			displayName = translate("Violent");
			break;
		case "aggressive":
			displayName = translate("Aggressive");
			break;
		case "passive":
			displayName = translate("Passive");
			break;
		case "defensive":
			displayName = translate("Defensive");
			break;
		case "standground":
			displayName = translate("Standground");
			break;
		default:
			warn(sprintf("Internationalization: Unexpected stance found with code ‘%(stance)s’. This stance must be internationalized.", { stance: name }));
			displayName = name;
			break;
	}
	return displayName;
}

/**
 * Helper function for updateUnitCommands; sets up "unit panels" (i.e. panels with rows of icons) for the currently selected
 * unit.
 *
 * @param guiName Short identifier string of this panel; see constants defined at the top of this file.
 * @param usedPanels Output object; usedPanels[guiName] will be set to 1 to indicate that this panel was used during this
 * 				run of updateUnitCommands and should not be hidden. TODO: why is this done this way instead of having
 * 				updateUnitCommands keep track of this?
 * @param unitEntState Entity state of the (first) selected unit.
 * @param items Panel-specific data to construct the icons with.
 * @param callback Callback function to argument to execute when an item's icon gets clicked. Takes a single 'item' argument.
 */
function setupUnitPanel(guiName, usedPanels, unitEntState, playerState, items, callback)
{
	usedPanels[guiName] = 1;

	var numberOfItems = items.length;
	var selection = g_Selection.toList();
	var garrisonGroups = new EntityGroups();

	// Determine how many buttons there should be
	switch (guiName)
	{
		case SELECTION:
			if (numberOfItems > 16)
				numberOfItems = 16;
			break;

		case QUEUE:
			if (numberOfItems > 16)
				numberOfItems = 16;
			break;

		case GARRISON:
			if (numberOfItems > 12)
				numberOfItems = 12;
			break;

		case STANCE:
			if (numberOfItems > 5)
				numberOfItems = 5;
			break;

		case FORMATION:
			if (numberOfItems > 16)
				numberOfItems = 16;
			break;

		case TRAINING:
			if (numberOfItems > 24)
				numberOfItems = 24;
			break;

		case RESEARCH:
			if (numberOfItems > 8)
				numberOfItems = 8;
			break;

		case CONSTRUCTION:
			if (numberOfItems > 24)
				numberOfItems = 24;
			break;

		case COMMAND:
			if (numberOfItems > 6)
				numberOfItems = 6;
			break;

		case GATE:
			if(numberOfItems > 8)
				numberOfItems = 8;
			break;

		case PACK:
			if(numberOfItems > 8)
				numberOfItems = 8;
			break;

		default:
			break;
	}

	switch (guiName)
	{
		case GARRISON:
		case COMMAND:
			// Common code for garrison and 'unload all' button counts.
			for (var i = 0; i < selection.length; ++i)
			{
				var state = GetEntityState(selection[i]);
				if (state.garrisonHolder)
					garrisonGroups.add(state.garrisonHolder.entities)
			}
			break;

		default:
			break;
	}

	var rowLength = 8;
	if (guiName == SELECTION)
		rowLength = 4;
	else if (guiName == FORMATION || guiName == GARRISON || guiName == COMMAND)
		rowLength = 4;

	// Make buttons
	var i;
	for (i = 0; i < numberOfItems; i++)
	{
		var item = items[i];

		// If a tech has been researched it leaves an empty slot
		if (guiName == RESEARCH && !item)
		{
			Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;
			// We also remove the paired tech and the pair symbol
			Engine.GetGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]").hidden = true;
			Engine.GetGUIObjectByName("unit"+guiName+"Pair["+i+"]").hidden = true;
			continue;
		}

		// Get the entity type and load the template for that type if necessary
		var entType;
		var template;
		var entType1;
		var template1;
		switch (guiName)
		{
			case QUEUE:
				// The queue can hold both technologies and units so we need to use the correct code for
				// loading the templates
				if (item.unitTemplate)
				{
					entType = item.unitTemplate;
					template = GetTemplateData(entType);
				}
				else if (item.technologyTemplate)
				{
					entType = item.technologyTemplate;
					template = GetTechnologyData(entType);
				}

				if (!template)
					continue; 	// ignore attempts to use invalid templates (an error should have been
								// reported already)
				break;
			case RESEARCH:
				if (item.pair)
				{
					entType1 = item.top;
					template1 = GetTechnologyData(entType1);
					if (!template1)
						continue; 	// ignore attempts to use invalid templates (an error should have been
									// reported already)

					entType = item.bottom;
				}
				else
				{
					entType = item;
				}
				template = GetTechnologyData(entType);
				if (!template)
					continue; 	// ignore attempts to use invalid templates (an error should have been
								// reported already)

				break;
			case SELECTION:
			case GARRISON:
			case TRAINING:
			case CONSTRUCTION:
				entType = item;
				template = GetTemplateData(entType);
				if (!template)
					continue;	// ignore attempts to use invalid templates (an error should have been
								// reported already)
				break;
		}

		switch (guiName)
		{
			case SELECTION:
				var name = getEntityNames(template);
				var tooltip = name;
				var count = g_Selection.groups.getCount(item);
				Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
				break;

			case QUEUE:
				var tooltip = getEntityNames(template);
				if (item.neededSlots)
					tooltip += "\n[color=\"red\"]" + translate("Insufficient population capacity:") + "\n[/color]" + sprintf(translate("%(population)s %(neededSlots)s"), { population: getCostComponentDisplayName("population"), neededSlots: item.neededSlots });

				var progress = Math.round(item.progress*100) + "%";
				Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");

				if (i == 0)
				{
					Engine.GetGUIObjectByName("queueProgress").caption = (item.progress ? progress : "");
					var size = Engine.GetGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size;

					// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
					size.top = size.left + Math.round(item.progress * (size.right - size.left));
					Engine.GetGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size = size;
				}
				break;

			case GARRISON:
				var name = getEntityNames(template);
				var tooltip = sprintf(translate("Unload %(name)s"), { name: name })+ "\n" + translate("Single-click to unload 1. Shift-click to unload all of this type.");
				var count = garrisonGroups.getCount(item);
				Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
				break;

			case GATE:
				var tooltip = item.tooltip;
				if (item.template)
				{
					var template = GetTemplateData(item.template);
					var wallCount = g_Selection.toList().reduce(function (count, ent) {
							var state = GetEntityState(ent);
							if (hasClass(state, "LongWall") && !state.gate)
								count++;
							return count;
						}, 0);

					tooltip += "\n" + getEntityCostTooltip(template, wallCount);

					var affordableMask = Engine.GetGUIObjectByName("unitGateUnaffordable["+i+"]");
					affordableMask.hidden = true;

					var neededResources = Engine.GuiInterfaceCall("GetNeededResources", multiplyEntityCosts(template, wallCount));
					if (neededResources)
					{
						affordableMask.hidden = false;
						tooltip += getNeededResourcesTooltip(neededResources);
					}
				}
				break;

			case PACK:
				var tooltip = item.tooltip;
				break;

			case STANCE:
				var tooltip = getStanceDisplayName(item);
				break;

			case TRAINING:
				var tooltip = getEntityNamesFormatted(template);
				var key = Engine.ConfigDB_GetValue("user", "hotkey.session.queueunit." + (i + 1));
				if (key)
					tooltip = "[color=\"255 251 131\"][font=\"sans-bold-16\"][" + key + "][/font][/color] " + tooltip;

				if (template.visibleIdentityClasses && template.visibleIdentityClasses.length)
				{
					tooltip += "\n[font=\"sans-bold-13\"]" + translate("Classes:") + "[/font] ";
					tooltip += "[font=\"sans-13\"]" + translate(template.visibleIdentityClasses[0]) ;
					for (var c = 1; c < template.visibleIdentityClasses.length; c++)
						tooltip += ", " + translate(template.visibleIdentityClasses[c]);
					tooltip += "[/font]";
				}

				if (template.tooltip)
					tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

				var [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch] =
					getTrainingBatchStatus(playerState, unitEntState.id, entType, selection);
				if (Engine.HotkeyIsPressed("session.batchtrain"))
					trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

				tooltip += "\n" + getEntityCostTooltip(template, trainNum, unitEntState.id);

				var [trainEntLimit, trainEntCount, canBeAddedCount, trainEntLimitChangers] =
					getEntityLimitAndCount(playerState, entType);
				tooltip += formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers);
				if (Engine.ConfigDB_GetValue("user", "showdetailedtooltips") === "true")
				{
					if (template.health)
						tooltip += "\n[font=\"sans-bold-13\"]" + translate("Health:") + "[/font] " + template.health;
					if (template.attack)
						tooltip += "\n" + getEntityAttack(template);
					if (template.armour)
						tooltip += "\n[font=\"sans-bold-13\"]" + translate("Armor:") + "[/font] " + armorTypesToText(template.armour);
					if (template.speed)
						tooltip += "\n" + getEntitySpeed(template);
				}
				tooltip += "[color=\"255 251 131\"]" + formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch) + "[/color]";
				break;

			case RESEARCH:
				var tooltip = getEntityNamesFormatted(template);
				if (template.tooltip)
					tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

				tooltip += "\n" + getEntityCostTooltip(template);

				if (item.pair)
				{
					var tooltip1 = getEntityNamesFormatted(template1);
					if (template1.tooltip)
						tooltip1 += "\n[font=\"sans-13\"]" + template1.tooltip + "[/font]";

					tooltip1 += "\n" + getEntityCostTooltip(template1);
				}
				break;

			case CONSTRUCTION:
				var tooltip = getEntityNamesFormatted(template);

				if (template.visibleIdentityClasses && template.visibleIdentityClasses.length)
				{
					tooltip += "\n[font=\"sans-bold-13\"]" + translate("Classes:") + "[/font] ";
					tooltip += "[font=\"sans-13\"]" + translate(template.visibleIdentityClasses[0]) ;
					for (var c = 1; c < template.visibleIdentityClasses.length; c++)
						tooltip += ", " + translate(template.visibleIdentityClasses[c]);
					tooltip += "[/font]";
				}

				if (template.tooltip)
					tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

				tooltip += "\n" + getEntityCostTooltip(template);
				tooltip += getPopulationBonusTooltip(template);

				var [entLimit, entCount, canBeAddedCount, entLimitChangers] =
					getEntityLimitAndCount(playerState, entType);
				tooltip += formatLimitString(entLimit, entCount, entLimitChangers);

				break;

			case COMMAND:
				// here, "item" is an object with properties .name (command name), .tooltip and .icon (relative to session/icons/single)
				if (item.name == "unload-all")
				{
					var count = garrisonGroups.getTotalCount();
					Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 0 ? count : "");
				}
				else
				{
					Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = "";
				}

				tooltip = (item.tooltip ? item.tooltip : toTitleCase(item.name));
				break;

			default:
				break;
		}

		// Button
		var button = Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var button1 = Engine.GetGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]");
		var affordableMask = Engine.GetGUIObjectByName("unit"+guiName+"Unaffordable["+i+"]");
		var affordableMask1 = Engine.GetGUIObjectByName("unit"+guiName+"Unaffordable["+(i+rowLength)+"]");
		var icon = Engine.GetGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		var guiSelection = Engine.GetGUIObjectByName("unit"+guiName+"Selection["+i+"]");
		var pair = Engine.GetGUIObjectByName("unit"+guiName+"Pair["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip || "";

		// Button Function (need nested functions to get the closure right)
		// Items can have a callback element that overrides the normal caller-supplied callback function.
		button.onpress = (function(e){ return function() { e.callback ? e.callback(e) : callback(e) } })(item);

		if(guiName == SELECTION)
		{
			button.onpressright = (function(e){return function() {callback(e, true) } })(item);
			button.onpress = (function(e){ return function() {callback(e, false) } })(item);
		}

		if (guiName == RESEARCH)
		{
			if (item.pair)
			{
				button.onpress = (function(e){ return function() { callback(e) } })(item.bottom);

				var icon1 = Engine.GetGUIObjectByName("unit"+guiName+"Icon["+(i+rowLength)+"]");
				button1.hidden = false;
				button1.tooltip = tooltip1;
				button1.onpress = (function(e){ return function() { callback(e) } })(item.top);

				// when we hover over a pair, the other one gets a red cross over it to show it won't be available any more.
				var unchosenIcon = Engine.GetGUIObjectByName("unit"+guiName+"UnchosenIcon["+i+"]");
				var unchosenIcon1 = Engine.GetGUIObjectByName("unit"+guiName+"UnchosenIcon["+(i+rowLength)+"]");

				button1.onmouseenter = (function(e){ return function() { setOverlay(e, true) } })(unchosenIcon);
				button1.onmouseleave = (function(e){ return function() { setOverlay(e, false) } })(unchosenIcon);

				button.onmouseenter = (function(e){ return function() { setOverlay(e, true) } })(unchosenIcon1);
				button.onmouseleave = (function(e){ return function() { setOverlay(e, false) } })(unchosenIcon1);

				pair.hidden = false;
			}
			else
			{
				// Hide the overlay.
				var unchosenIcon = Engine.GetGUIObjectByName("unit"+guiName+"UnchosenIcon["+i+"]");
				unchosenIcon.hidden = true;
			}
		}

		// Get icon image
		if (guiName == FORMATION)
		{
			var formationInfo = Engine.GuiInterfaceCall("GetFormationInfoFromTemplate", {"templateName": item});

			button.tooltip = translate(formationInfo.name);
			var formationOk = canMoveSelectionIntoFormation(item);
			var grayscale = "";
			button.enabled = formationOk;
			if (!formationOk)
 			{
				grayscale = "grayscale:";

				if (formationInfo.tooltip)
					button.tooltip += "\n" + "[color=\"red\"]" + translate(formationInfo.tooltip) + "[/color]";
			}

			var formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
				"ents": g_Selection.toList(),
				"formationTemplate": item
			});

			guiSelection.hidden = !formationSelected;
			icon.sprite = "stretched:"+grayscale+"session/icons/"+formationInfo.icon;

 		}
		else if (guiName == STANCE)
		{
			var stanceSelected = Engine.GuiInterfaceCall("IsStanceSelected", {
				"ents": g_Selection.toList(),
				"stance": item
			});

			guiSelection.hidden = !stanceSelected;
			icon.sprite = "stretched:session/icons/stances/"+item+".png";
		}
		else if (guiName == COMMAND)
		{
			icon.sprite = "stretched:session/icons/" + item.icon;
		}
		else if (guiName == GATE)
		{
			var gateIcon;
			// If already a gate, show locking actions
			if (item.gate)
			{
				gateIcon = "icons/lock_" + GATE_ACTIONS[item.locked ? 0 : 1] + "ed.png";
				guiSelection.hidden = item.gate.locked === undefined ? false : item.gate.locked != item.locked;
			}
			// otherwise show gate upgrade icon
			else
			{
				template = GetTemplateData(item.template);
				gateIcon = template.icon ?  "portraits/" + template.icon : "icons/gate_closed.png";
				guiSelection.hidden = true;
			}

			icon.sprite = "stretched:session/" + gateIcon;
		}
		else if (guiName == PACK)
		{
			if (item.packing)
			{
				icon.sprite = "stretched:session/icons/cancel.png";
			}
			else
			{
				if (item.packed)
					icon.sprite = "stretched:session/icons/unpack.png";
				else
					icon.sprite = "stretched:session/icons/pack.png";
			}
		}
		else if (template.icon)
		{
			var grayscale = "";
			button.enabled = true;
			if (affordableMask)
				affordableMask.hidden = true;	// actually used for the red "lack of resource" overlay, and darkening if unavailable. Sort of a hack.

			// In case this is an icon that would require tech checking, make sure we have the requirements.
			if (guiName != SELECTION && guiName != GARRISON && guiName != QUEUE && template.requiredTechnology && !Engine.GuiInterfaceCall("IsTechnologyResearched", template.requiredTechnology))
			{
				button.enabled = false;
				var techName = getEntityNames(GetTechnologyData(template.requiredTechnology));
				button.tooltip += "\n" + sprintf(translate("Requires %(technology)s"), { technology: techName });
				grayscale = "grayscale:";
				affordableMask.hidden = false;
				affordableMask.sprite = "colour: 0 0 0 127";
			}

			if (guiName == RESEARCH && !Engine.GuiInterfaceCall("CheckTechnologyRequirements", entType))
			{
				button.enabled = false;
				button.tooltip += "\n" + GetTechnologyData(entType).requirementsTooltip;
				if (GetTechnologyData(entType).classRequirements)
				{
					var player = Engine.GetPlayerID();
					var current = GetSimState().players[player].classCounts[GetTechnologyData(entType).classRequirements.class];
					// If current is undefined, this means no building filling the requirement has been found
					current = current ? current : 0;
					var remaining = GetTechnologyData(entType).classRequirements.number - current;
					button.tooltip += " " + sprintf(translatePlural("Remaining: %(number)s to build.", "Remaining: %(number)s to build.", remaining), { number: remaining});
				}
				grayscale = "grayscale:";
				affordableMask.hidden = false;
				affordableMask.sprite = "colour: 0 0 0 127";
			}

			if ((guiName == CONSTRUCTION || guiName == TRAINING) && canBeAddedCount == 0)
			{
				grayscale = "grayscale:";
				affordableMask.hidden = false;
				affordableMask.sprite = "colour: 0 0 0 127";
			}

			if (guiName == GARRISON)
			{
				var ents = garrisonGroups.getEntsByName(item);
				var entplayer = GetEntityState(ents[0]).player;
				button.sprite = "colour: " + g_Players[entplayer].color.r + " " + g_Players[entplayer].color.g + " " + g_Players[entplayer].color.b;

				var player = Engine.GetPlayerID();
				if(player != unitEntState.player && !g_DevSettings.controlAll)
				{
					if (entplayer != player)
					{
						button.enabled = false;
						grayscale = "grayscale:";
					}
				}
			}

			icon.sprite = "stretched:" + grayscale + "session/portraits/" + template.icon;

			if (guiName == RESEARCH)
			{
				// Check resource requirements
				var neededResources = Engine.GuiInterfaceCall("GetNeededResources", template.cost);
				if (neededResources)
				{
					if (button.enabled !== false)
					{
						button.enabled = false;
						affordableMask.hidden = false;

						var totalCost = 0;
						for each (var resource in neededResources)
							totalCost += resource;
						var alpha = 50 + totalCost/10;
						alpha = alpha > 125 ? 125 : alpha;
						affordableMask.sprite = "colour: 255 0 0 " + (alpha);
					}
					button.tooltip += getNeededResourcesTooltip(neededResources);
				}

				if (item.pair)
				{
					grayscale = "";
					button1.enabled = true;
					affordableMask1.hidden = true;

					if (!Engine.GuiInterfaceCall("CheckTechnologyRequirements", entType1))
					{
						button1.enabled = false;
						button1.tooltip += "\n" + GetTechnologyData(entType1).requirementsTooltip;
						grayscale = "grayscale:";
						affordableMask1.hidden = false;
						affordableMask1.sprite = "colour: 0 0 0 127";
					}
					icon1.sprite = "stretched:" + grayscale + "session/portraits/" +template1.icon;

					// Check resource requirements for second button
					neededResources = Engine.GuiInterfaceCall("GetNeededResources", template1.cost);
					if (neededResources)
					{
						if (button1.enabled !== false)
						{
							button1.enabled = false;
							affordableMask1.hidden = false;

							var totalCost = 0;
							for each (var resource in neededResources)
								totalCost += resource;
							var alpha = 50 + totalCost/10;
							alpha = alpha > 125 ? 125 : alpha;
							affordableMask1.sprite = "colour: 255 0 0 " + (alpha);
						}
						button1.tooltip += getNeededResourcesTooltip(neededResources);
					}
				}
				else
				{
					pair.hidden = true;
					button1.hidden = true;
					affordableMask1.hidden = true;
				}
			}
			else if (guiName == CONSTRUCTION || guiName == TRAINING)
			{
				var totalCosts = {};
				var trainNum = 1;
				var button_disableable = true;

				if (guiName == TRAINING)
				{
					if (Engine.HotkeyIsPressed("session.batchtrain"))
					{
						var [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch, batchTrainingCount] =
							getTrainingBatchStatus(playerState, unitEntState.id, entType, selection);
						trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;
						button_disableable = !Engine.HotkeyIsPressed("selection.remove");
					}
					Engine.GetGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (batchTrainingCount > 0) ? batchTrainingCount : "";
				}

				// Walls have no cost defined.
				if (template.cost !== undefined)
					totalCosts = multiplyEntityCosts(template, trainNum);

				var neededResources = Engine.GuiInterfaceCall("GetNeededResources", totalCosts);
				if (neededResources)
				{
					if (button.enabled !== false)
					{
						button.enabled = (button_disableable ? false : true);
						// Don't display the red overlay if we can't even train/build it
						if (canBeAddedCount != 0)
						{
							affordableMask.hidden = false;

							var totalCost = 0;
							for each (var resource in neededResources)
								totalCost += resource;
							var alpha = 50 + totalCost/10;
							alpha = alpha > 125 ? 125 : alpha;
							affordableMask.sprite = "colour: 255 0 0 " + (alpha);
						}
					}
					button.tooltip += getNeededResourcesTooltip(neededResources);
				}
			}
		}
		else
		{
			// TODO: we should require all entities to have icons, so this case never occurs
			icon.sprite = "bkFillBlack";
		}
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var numButtons = i;

	var numRows = Math.ceil(numButtons / rowLength);

	var buttonSideLength = Engine.GetGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;

	// We sort pairs upside down, so get the size from the topmost button.
	if (guiName == RESEARCH)
		buttonSideLength = Engine.GetGUIObjectByName("unit"+guiName+"Button["+(rowLength*numRows)+"]").size.bottom;

	var buttonSpacer = buttonSideLength+1;

	// Layout buttons
	if (guiName == COMMAND)
	{
		layoutButtonRowCentered(0, guiName, 0, numButtons, COMMANDS_PANEL_WIDTH);
	}
	else if (guiName == RESEARCH)
	{
		// We support pairs so we need to add a row
		numRows++;
		// Layout rows from bottom to top
		for (var i = 0, j = numRows; i < numRows; i++, j--)
		{
			layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*(j-1), rowLength*j);
		}
	}
	else
	{
		for (var i = 0; i < numRows; i++)
			layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );
	}

	// Layout pair icons
	if (guiName == RESEARCH)
	{
		var pairSize = Engine.GetGUIObjectByName("unit"+guiName+"Pair[0]").size;
		var pairSideWidth = pairSize.right;
		var pairSideHeight = pairSize.bottom;
		var pairSpacerHeight = pairSideHeight + 1;
		var pairSpacerWidth = pairSideWidth + 1;

		layoutRow("Pair", 0, guiName, pairSideWidth, pairSpacerWidth, pairSideHeight, pairSpacerHeight, 0, rowLength);
	}

	// Resize Queue panel if needed
	if (guiName == QUEUE) // or garrison
	{
		var panel = Engine.GetGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Hide any buttons we're no longer using
	for (var i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		Engine.GetGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	// Hide unused pair buttons and symbols
	if (guiName == RESEARCH)
	{
		for (var i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		{
			Engine.GetGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]").hidden = true;
			Engine.GetGUIObjectByName("unit"+guiName+"Pair["+i+"]").hidden = true;
		}
	}

	g_unitPanelButtons[guiName] = numButtons;
}

// Sets up "unit trading panel" - special case for setupUnitPanel
function setupUnitTradingPanel(usedPanels, unitEntState, selection)
{
	usedPanels[TRADING] = 1;

	var requiredGoods = unitEntState.trader.requiredGoods;
	for (var i = 0; i < TRADING_RESOURCES.length; i++)
	{
		var resource = TRADING_RESOURCES[i];
		var button = Engine.GetGUIObjectByName("unitTradingButton["+i+"]");
		button.size = (i * 46) + " 0 " + ((i + 1) * 46) + " 46";
		if (resource == requiredGoods)
			var selectRequiredGoodsData = { "entities": selection, "requiredGoods": undefined };
		else
			var selectRequiredGoodsData = { "entities": selection, "requiredGoods": resource };
		button.onpress = (function(e){ return function() { selectRequiredGoods(e); } })(selectRequiredGoodsData);
		button.enabled = true;
		button.tooltip = sprintf(translate("Set/unset %(resource)s as forced trading goods."), { resource: resource });
		var icon = Engine.GetGUIObjectByName("unitTradingIcon["+i+"]");
		var selected = Engine.GetGUIObjectByName("unitTradingSelection["+i+"]");
		selected.hidden = !(resource == requiredGoods);
		var grayscale = (resource != requiredGoods) ? "grayscale:" : "";
		icon.sprite = "stretched:"+grayscale+"session/icons/resources/" + resource + ".png";
	}
}

// Sets up "unit barter panel" - special case for setupUnitPanel
function setupUnitBarterPanel(unitEntState, playerState)
{
	// Amount of player's resource to exchange
	var amountToSell = BARTER_RESOURCE_AMOUNT_TO_SELL;
	if (Engine.HotkeyIsPressed("session.massbarter"))
		amountToSell *= BARTER_BUNCH_MULTIPLIER;
	// One pass for each resource
	for (var i = 0; i < BARTER_RESOURCES.length; i++)
	{
		var resource = BARTER_RESOURCES[i];
		// One pass for 'sell' row and another for 'buy'
		for (var j = 0; j < 2; j++)
		{
			var action = BARTER_ACTIONS[j];

			if (j == 0)
			{
				// Display the selection overlay
				var selection = Engine.GetGUIObjectByName("unitBarter" + action + "Selection["+i+"]");
				selection.hidden = !(i == g_barterSell);
			}

			// We gray out the not selected icons in 'sell' row
			var grayscale = (j == 0 && i != g_barterSell) ? "grayscale:" : "";
			var icon = Engine.GetGUIObjectByName("unitBarter" + action + "Icon["+i+"]");

			var button = Engine.GetGUIObjectByName("unitBarter" + action + "Button["+i+"]");
			button.size = (i * 46) + " 0 " + ((i + 1) * 46) + " 46";
			var amountToBuy;
			// We don't display a button in 'buy' row if the same resource is selected in 'sell' row
			if (j == 1 && i == g_barterSell)
			{
				button.hidden = true;
			}
			else
			{
				button.hidden = false;
				button.tooltip = action + " " + resource;
				icon.sprite = "stretched:"+grayscale+"session/icons/resources/" + resource + ".png";
				var sellPrice = unitEntState.barterMarket.prices["sell"][BARTER_RESOURCES[g_barterSell]];
				var buyPrice = unitEntState.barterMarket.prices["buy"][resource];
				amountToBuy = "+" + Math.round(sellPrice / buyPrice * amountToSell);
			}

			var amount;
			if (j == 0)
			{
				button.onpress = (function(i){ return function() { g_barterSell = i; } })(i);
				if (i == g_barterSell)
				{
					amount = "-" + amountToSell;

					var neededRes = {};
					neededRes[resource] = amountToSell;
					var neededResources = Engine.GuiInterfaceCall("GetNeededResources", neededRes);
					var hidden = neededResources ? false : true;
					for (var ii = 0; ii < BARTER_RESOURCES.length; ii++)
					{
						var affordableMask = Engine.GetGUIObjectByName("unitBarterBuyUnaffordable["+ii+"]");
						affordableMask.hidden = hidden;
					}
				}
				else
					amount = "";
 			}
			else
			{
				var exchangeResourcesParameters = { "sell": BARTER_RESOURCES[g_barterSell], "buy": BARTER_RESOURCES[i], "amount": amountToSell };
				button.onpress = (function(exchangeResourcesParameters){ return function() { exchangeResources(exchangeResourcesParameters); } })(exchangeResourcesParameters);
				amount = amountToBuy;
			}
			Engine.GetGUIObjectByName("unitBarter" + action + "Amount["+i+"]").caption = amount;
		}
	}
}

/**
 * Updates the right hand side "Unit Commands" panel. Runs in the main session loop via updateSelectionDetails().
 * Delegates to setupUnitPanel to set up individual subpanels, appropriately activated depending on the selected
 * unit's state.
 *
 * @param entState Entity state of the (first) selected unit.
 * @param supplementalDetailsPanel Reference to the "supplementalSelectionDetails" GUI Object
 * @param commandsPanel Reference to the "commandsPanel" GUI Object
 * @param selection Array of currently selected entity IDs.
 */
function updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection)
{
	// Panels that are active
	var usedPanels = {};

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();

	// Get player state to check some constraints
	// e.g. presence of a hero or build limits
	var simState = GetSimState();
	var playerState = simState.players[player];

	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (selection.length > 1)
			setupUnitPanel(SELECTION, usedPanels, entState, playerState, g_Selection.groups.getTemplateNames(),
				function (entType, rightPressed) { changePrimarySelectionGroup(entType, rightPressed); } );

		var commands = getEntityCommandsList(entState);
		if (commands.length)
			setupUnitPanel(COMMAND, usedPanels, entState, playerState, commands,
				function (item) { performCommand(entState.id, item.name); } );

		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (state.garrisonHolder)
					groups.add(state.garrisonHolder.entities)
			}

			setupUnitPanel(GARRISON, usedPanels, entState, playerState, groups.getTemplateNames(),
				function (item) { unloadTemplate(item); } );
		}

		var formations = Engine.GuiInterfaceCall("GetAvailableFormations");
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && formations.length)
		{
			setupUnitPanel(FORMATION, usedPanels, entState, playerState, formations,
				function (item) { performFormation(entState.id, item); } );
		}

		// TODO: probably should load the stance list from a data file,
		// and/or vary depending on what units are selected
		var stances = ["violent", "aggressive", "passive", "defensive", "standground"];
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && stances.length)
		{
			setupUnitPanel(STANCE, usedPanels, entState, playerState, stances,
				function (item) { performStance(entState.id, item); } );
		}

		Engine.GetGUIObjectByName("unitBarterPanel").hidden = !entState.barterMarket;
		if (entState.barterMarket)
		{
			usedPanels["Barter"] = 1;
			setupUnitBarterPanel(entState, playerState);
		}

		var buildableEnts = getAllBuildableEntitiesFromSelection();
		var trainableEnts = getAllTrainableEntitiesFromSelection();

		// Whether the GUI's right panel has been filled.
		var rightUsed = true;

		// The first selected entity's type has priority.
		if (entState.buildEntities)
			setupUnitPanel(CONSTRUCTION, usedPanels, entState, playerState, buildableEnts,
				function (trainEntType) { startBuildingPlacement(trainEntType, playerState); } );
		else if (entState.production && entState.production.entities)
			setupUnitPanel(TRAINING, usedPanels, entState, playerState, trainableEnts,
				function (trainEntType) { addTrainingToQueue(selection, trainEntType, playerState); } );
//		else if (entState.trader)
//			setupUnitTradingPanel(usedPanels, entState, selection);
		else if (!entState.foundation && entState.gate || hasClass(entState, "LongWall"))
		{
			// Allow long wall pieces to be converted to gates
			var longWallTypes = {};
			var walls = [];
			var gates = [];
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (hasClass(state, "LongWall") && !state.gate && !longWallTypes[state.template])
				{
					var gateTemplate = getWallGateTemplate(state.id);
					if (gateTemplate)
					{
						var tooltipString = GetTemplateDataWithoutLocalization(state.template).gateConversionTooltip;
						if (!tooltipString)
						{
							warn(state.template + " is supposed to be convertable to a gate, but it's missing the GateConversionTooltip in the Identity template");
							tooltipString = "";
						}
						walls.push({
							"tooltip": translate(tooltipString),
							"template": gateTemplate,
							"callback": function (item) { transformWallToGate(item.template); }
						});
					}

					// We only need one entity per type.
					longWallTypes[state.template] = true;
				}
				else if (state.gate && !gates.length)
				{
					gates.push({
						"gate": state.gate,
						"tooltip": translate("Lock Gate"),
						"locked": true,
						"callback": function (item) { lockGate(item.locked); }
					});
					gates.push({
						"gate": state.gate,
						"tooltip": translate("Unlock Gate"),
						"locked": false,
						"callback": function (item) { lockGate(item.locked); }
					});
				}
				// Show both 'locked' and 'unlocked' as active if the selected gates have both lock states.
				else if (state.gate && state.gate.locked != gates[0].gate.locked)
					for (var j = 0; j < gates.length; ++j)
						delete gates[j].gate.locked;
			}

			// Place wall conversion options after gate lock/unlock icons.
			var items = gates.concat(walls);
			if (items.length)
				setupUnitPanel(GATE, usedPanels, entState, playerState, items);
			else
				rightUsed = false;
		}
		else if (entState.pack)
		{
			var items = [];
			var packButton = false;
			var unpackButton = false;
			var packCancelButton = false;
			var unpackCancelButton = false;
			for (var i in selection)
			{
				// Find un/packable entities
				var state = GetEntityState(selection[i]);
				if (state.pack)
				{
					if (state.pack.progress == 0)
					{
						if (!state.pack.packed)
							packButton = true;
						else if (state.pack.packed)
							unpackButton = true;
					}
					else
					{
						// Already un/packing - show cancel button
						if (!state.pack.packed)
							packCancelButton = true;
						else if (state.pack.packed)
							unpackCancelButton = true;
					}
				}
			}
			if (packButton)
				items.push({ "packing": false, "packed": false, "tooltip": translate("Pack"), "callback": function() { packUnit(true); } });
			if (unpackButton)
				items.push({ "packing": false, "packed": true, "tooltip": translate("Unpack"), "callback": function() { packUnit(false); } });
			if (packCancelButton)
				items.push({ "packing": true, "packed": false, "tooltip": translate("Cancel Packing"), "callback": function() { cancelPackUnit(true); } });
			if (unpackCancelButton)
				items.push({ "packing": true, "packed": true, "tooltip": translate("Cancel Unpacking"), "callback": function() { cancelPackUnit(false); } });

			if (items.length)
				setupUnitPanel(PACK, usedPanels, entState, playerState, items);
			else
				rightUsed = false;
		}
		else
			rightUsed = false;

		if (!rightUsed)
		{
			// The right pane is empty. Fill the pane with a sane type.
			// Prefer buildables for units and trainables for structures.
			if (buildableEnts.length && (hasClass(entState, "Unit") || !trainableEnts.length))
				setupUnitPanel(CONSTRUCTION, usedPanels, entState, playerState, buildableEnts,
					function (trainEntType) { startBuildingPlacement(trainEntType, playerState); });
			else if (trainableEnts.length)
				setupUnitPanel(TRAINING, usedPanels, entState, playerState, trainableEnts,
					function (trainEntType) { addTrainingToQueue(selection, trainEntType, playerState); } );
		}
		// Show technologies if the active panel has at most one row of icons.
		if (entState.production && entState.production.technologies.length)
		{
			var activepane = usedPanels[CONSTRUCTION] ? buildableEnts.length : trainableEnts.length;
			if (selection.length == 1 || activepane <= 8)
				setupUnitPanel(RESEARCH, usedPanels, entState, playerState, entState.production.technologies,
					function (researchType) { addResearchToQueue(entState.id, researchType); } );
		}

		if (entState.production && entState.production.queue.length)
			setupUnitPanel(QUEUE, usedPanels, entState, playerState, entState.production.queue,
				function (item) { removeFromProductionQueue(entState.id, item.id); } );

		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = false;
	}
	else if (playerState.isMutualAlly[entState.player]) // owned by allied player
	{
		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			for (var i in selection)
			{
				var state = GetEntityState(selection[i]);
				if (state.garrisonHolder)
					groups.add(state.garrisonHolder.entities)
			}

			setupUnitPanel(GARRISON, usedPanels, entState, playerState, groups.getTemplateNames(),
				function (item) { unloadTemplate(item); } );

			supplementalDetailsPanel.hidden = false;
		}
		else
		{
			supplementalDetailsPanel.hidden = true;
		}

		commandsPanel.hidden = true;
	}
	else // owned by another player
	{
		supplementalDetailsPanel.hidden = true;
		commandsPanel.hidden = true;
	}

	// Hides / unhides Unit Panels (panels should be grouped by type, not by order, but we will leave that for another time)
	var offset = 0;
	for each (var panelName in g_unitPanels)
	{
		var panel = Engine.GetGUIObjectByName("unit" + panelName + "Panel");
		if (usedPanels[panelName])
			panel.hidden = false;
		else
			panel.hidden = true;
	}
}

// Force hide commands panels
function hideUnitCommands()
{
	for each (var panelName in g_unitPanels)
		Engine.GetGUIObjectByName("unit" + panelName + "Panel").hidden = true;
}

// Get all of the available entities which can be trained by the selected entities
function getAllTrainableEntities(selection)
{
	var trainableEnts = [];
	var state;
	// Get all buildable and trainable entities
	for (var i in selection)
	{
		if ((state = GetEntityState(selection[i])) && state.production && state.production.entities.length)
			trainableEnts = trainableEnts.concat(state.production.entities);
	}

	// Remove duplicates
	removeDupes(trainableEnts);
	return trainableEnts;
}

function getAllTrainableEntitiesFromSelection()
{
	if (!g_allTrainableEntities)
		g_allTrainableEntities = getAllTrainableEntities(g_Selection.toList());

	return g_allTrainableEntities;
}

// Get all of the available entities which can be built by the selected entities
function getAllBuildableEntities(selection)
{
	var buildableEnts = [];
	var state;
	// Get all buildable entities
	for (var i in selection)
	{
		if ((state = GetEntityState(selection[i])) && state.buildEntities && state.buildEntities.length)
			buildableEnts = buildableEnts.concat(state.buildEntities);
	}

	// Remove duplicates
	removeDupes(buildableEnts);
	return buildableEnts;
}

function getAllBuildableEntitiesFromSelection()
{
	if (!g_allBuildableEntities)
		g_allBuildableEntities = getAllBuildableEntities(g_Selection.toList());

	return g_allBuildableEntities;
}

// Check if the selection can move into formation, and cache the result
function canMoveSelectionIntoFormation(formationTemplate)
{
	if (!(formationTemplate in g_canMoveIntoFormation))
	{
		g_canMoveIntoFormation[formationTemplate] = Engine.GuiInterfaceCall("CanMoveEntsIntoFormation", {
			"ents": g_Selection.toList(),
			"formationTemplate": formationTemplate
		});
	}
	return g_canMoveIntoFormation[formationTemplate];
}

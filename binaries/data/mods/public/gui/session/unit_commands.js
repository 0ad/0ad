// Panel types
const SELECTION = "Selection";
const QUEUE = "Queue";
const GARRISON = "Garrison";
const FORMATION = "Formation";
const TRAINING = "Training";
const RESEARCH = "Research";
const CONSTRUCTION = "Construction";
const COMMAND = "Command";
const STANCE = "Stance";
const GATE = "Gate";

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

// The number of currently visible buttons (used to optimise showing/hiding)
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Trading": 0, "Construction": 0, "Command": 0, "Stance": 0, "Gate": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Formation", "Garrison", "Training", "Barter", "Trading", "Construction", "Research", "Stance", "Command", "Gate"];

// Indexes of resources to sell and buy on barter panel
var g_barterSell = 0;

// Lay out a row of centered buttons (does not work inside a loop like the other function)
function layoutButtonRowCentered(rowNumber, guiName, startIndex, endIndex, width)
{
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;
	var colNumber = 0;

	// Collect buttons
	var buttons = [];
	var icons = [];

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");

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
		var button = getGUIObjectByName("unit"+guiName+objectName+"["+i+"]");

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
 * Helper function for updateUnitCommands; sets up "unit panels" (i.e. panels with rows of icons) for the currently selected
 * unit.
 * 
 * @param guiName Short identifier string of this panel; see constants defined at the top of this file.
 * @param usedPanels Output object; usedPanels[guiName] will be set to 1 to indicate that this panel was used during this
 *                     run of updateUnitCommands and should not be hidden. TODO: why is this done this way instead of having
 *                     updateUnitCommands keep track of this?
 * @param unitEntState Entity state of the (first) selected unit.
 * @param items Panel-specific data to construct the icons with.
 * @param callback Callback function to argument to execute when an item's icon gets clicked. Takes a single 'item' argument.
 */
function setupUnitPanel(guiName, usedPanels, unitEntState, items, callback)
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
				numberOfItems =  16;
			break;

		case QUEUE:
			if (numberOfItems > 16)
				numberOfItems = 16;
			break;

		case GARRISON:
			if (numberOfItems > 16)
				numberOfItems = 16;
			//Group garrisoned units based on class
			garrisonGroups.add(unitEntState.garrisonHolder.entities);
			break;

		case STANCE:
			if (numberOfItems > 5)
				numberOfItems =  5;
			break;

		case FORMATION:
			if (numberOfItems > 16)
				numberOfItems =  16;
			break;

		case TRAINING:
			if (numberOfItems > 24)
				numberOfItems =  24;
			break;

		case RESEARCH:
			if (numberOfItems > 8)
				numberOfItems =  8;
			break;

		case CONSTRUCTION:
			if (numberOfItems > 24)
				numberOfItems =  24;
			break;

		case COMMAND:
			if (numberOfItems > 6)
				numberOfItems = 6;
			break;

		case GATE:
			if(numberOfItems > 1)
				numberOfItems = 1;
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
			getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;
			// We also remove the paired tech and the pair symbol
			getGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]").hidden = true;
			getGUIObjectByName("unit"+guiName+"Pair["+i+"]").hidden = true;
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
					continue; // ignore attempts to use invalid templates (an error should have been
					          // reported already)
				break;
			case RESEARCH:
				if (item.pair)
				{
					entType1 = item.top;
					template1 = GetTechnologyData(entType1);
					if (!template1)
						continue; // ignore attempts to use invalid templates (an error should have been
						          // reported already)
					
					entType = item.bottom;
				}
				else
				{
					entType = item;
				}
				template = GetTechnologyData(entType);
				if (!template)
					continue; // ignore attempts to use invalid templates (an error should have been
					          // reported already)
				
				break;
			case SELECTION:
			case GARRISON:
			case TRAINING:
			case CONSTRUCTION:
				entType = item;
				template = GetTemplateData(entType);
				if (!template)
					continue; // ignore attempts to use invalid templates (an error should have been
					          // reported already)
				break;
		}

		switch (guiName)
		{
			case SELECTION:
				var name = getEntityName(template);
				var tooltip = name;
				var count = g_Selection.groups.getCount(item);
				getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
				break;

			case QUEUE:
				var tooltip = getEntityName(template);
				var progress = Math.round(item.progress*100) + "%";
				getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (item.count > 1 ? item.count : "");

				if (i == 0)
				{
					getGUIObjectByName("queueProgress").caption = (item.progress ? progress : "");
					var size = getGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size;
					size.top = Math.round(item.progress*40);
					getGUIObjectByName("unit"+guiName+"ProgressSlider["+i+"]").size = size;
				}
				break;

			case GARRISON:
				var name = getEntityName(template);
				var tooltip = "Unload " + getEntityName(template) + "\nSingle-click to unload 1. Shift-click to unload all of this type.";
				var count = garrisonGroups.getCount(item);
				getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 1 ? count : "");
				break;

			case STANCE:
			case FORMATION:
			case GATE:
				var tooltip = toTitleCase(item);
				break;

			case TRAINING:
				var tooltip = getEntityNameWithGenericType(template);

				if (template.tooltip)
					tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

				var [batchSize, batchIncrement] = getTrainingBatchStatus(unitEntState.id, entType);
				var trainNum = batchSize ? batchSize+batchIncrement : batchIncrement;

				tooltip += "\n" + getEntityCostTooltip(template);

				if (template.health)
					tooltip += "\n[font=\"serif-bold-13\"]Health:[/font] " + template.health;
				if (template.armour)
					tooltip += "\n[font=\"serif-bold-13\"]Armour:[/font] " + damageTypesToText(template.armour);
				if (template.attack)
					tooltip += "\n" + getEntityAttack(template);
				if (template.speed)
					tooltip += "\n" + getEntitySpeed(template);

				tooltip += "\n\n[font=\"serif-bold-13\"]Shift-click[/font][font=\"serif-13\"] to train " + trainNum + ".[/font]";

				break;
				
			case RESEARCH:
				var tooltip = getEntityNameWithGenericType(template);
				if (template.tooltip)
					tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

				tooltip += "\n" + getEntityCostTooltip(template);

				if (item.pair)
				{
					var tooltip1 = getEntityNameWithGenericType(template1);
					if (template1.tooltip)
						tooltip1 += "\n[font=\"serif-13\"]" + template1.tooltip + "[/font]";

					tooltip1 += "\n" + getEntityCostTooltip(template1);
				}
				break;

			case CONSTRUCTION:
				var tooltip = getEntityNameWithGenericType(template);
				if (template.tooltip)
					tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

				tooltip += "\n" + getEntityCostTooltip(template); // see utility_functions.js
				tooltip += getPopulationBonusTooltip(template); // see utility_functions.js

				if (template.health)
					tooltip += "\n[font=\"serif-bold-13\"]Health:[/font] " + template.health;

				break;

			case COMMAND:
				// here, "item" is an object with properties .name (command name), .tooltip and .icon (relative to session/icons/single)
				if (item.name == "unload-all")
				{
					var count = unitEntState.garrisonHolder.entities.length;
					getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = (count > 0 ? count : "");
				}
				else
				{
					getGUIObjectByName("unit"+guiName+"Count["+i+"]").caption = "";
				}

				tooltip = (item.tooltip ? item.tooltip : toTitleCase(item.name));
				break;

			default:
				break;
		}

		// Button
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");
		var button1 = getGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]");
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		var selection = getGUIObjectByName("unit"+guiName+"Selection["+i+"]");
		var pair = getGUIObjectByName("unit"+guiName+"Pair["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip;

		// Button Function (need nested functions to get the closure right)
		button.onpress = (function(e){ return function() { callback(e) } })(item);

		if (guiName == RESEARCH)
		{
			if (item.pair)
			{
				button.onpress = (function(e){ return function() { callback(e) } })(item.bottom);

				var icon1 = getGUIObjectByName("unit"+guiName+"Icon["+(i+rowLength)+"]");
				button1.hidden = false;
				button1.tooltip = tooltip1;
				button1.onpress = (function(e){ return function() { callback(e) } })(item.top);

				// We add a red overlay to the paired button (we reuse the selection for that)
				button1.onmouseenter = (function(e){ return function() { setOverlay(e, true) } })(selection);
				button1.onmouseleave = (function(e){ return function() { setOverlay(e, false) } })(selection);

				var selection1 = getGUIObjectByName("unit"+guiName+"Selection["+(i+rowLength)+"]");;
				button.onmouseenter = (function(e){ return function() { setOverlay(e, true) } })(selection1);
				button.onmouseleave = (function(e){ return function() { setOverlay(e, false) } })(selection1);

				pair.hidden = false;
			}
			else
			{
				// Hide the overlay
				selection.hidden = true;
			}
		}

		// Get icon image
		if (guiName == FORMATION)
		{
			var formationOk = Engine.GuiInterfaceCall("CanMoveEntsIntoFormation", {
				"ents": g_Selection.toList(),
				"formationName": item
			});

			var grayscale = "";
			button.enabled = formationOk;
			if (!formationOk)
 			{
				grayscale = "grayscale:";
				
				// Display a meaningful tooltip why the formation is disabled
				var requirements = Engine.GuiInterfaceCall("GetFormationRequirements", {
					"formationName": item
				});

 				button.tooltip += " (disabled)";
				if (requirements.count > 1)
					button.tooltip += "\n" + requirements.count + " units required";
				if (requirements.classesRequired)
				{
					button.tooltip += "\nOnly units of type";
					for each (var classRequired in requirements.classesRequired)
					{
						button.tooltip += " " + classRequired;
					}
					button.tooltip += " allowed.";
				}
 			}

			var formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
				"ents": g_Selection.toList(),
				"formationName": item
			});

			selection.hidden = !formationSelected;
			icon.sprite = "stretched:"+grayscale+"session/icons/formations/"+item.replace(/\s+/,'').toLowerCase()+".png";
			
 		}
		else if (guiName == STANCE)
		{
			var stanceSelected = Engine.GuiInterfaceCall("IsStanceSelected", {
				"ents": g_Selection.toList(),
				"stance": item
			});

			selection.hidden = !stanceSelected;
			icon.sprite = "stretched:session/icons/stances/"+item+".png";
		}
		else if (guiName == COMMAND)
		{
			icon.sprite = "stretched:session/icons/" + item.icon;

		}
		else if (guiName == GATE)
		{
			var lockIcon = unitEntState.gate.locked ? "gate_unlocked.png" : "gate_locked.png";
			icon.sprite = "stretched:session/icons/" + lockIcon;
		}
		else if (template.icon)
		{
			var grayscale = "";
			button.enabled = true;
			
			if (guiName != SELECTION && template.requiredTechnology && !Engine.GuiInterfaceCall("IsTechnologyResearched", template.requiredTechnology))
			{
				button.enabled = false;
				var techName = getEntityName(GetTechnologyData(template.requiredTechnology));
				button.tooltip += "\nRequires " + techName;
				grayscale = "grayscale:";
			}
			
			if (guiName == RESEARCH && !Engine.GuiInterfaceCall("CheckTechnologyRequirements", entType))
			{
				button.enabled = false;
				button.tooltip += "\n" + GetTechnologyData(entType).requirementsTooltip;
				grayscale = "grayscale:";
			}
			
			icon.sprite = "stretched:" + grayscale + "session/portraits/" + template.icon;
			
			if (guiName == RESEARCH)
			{
				if (item.pair)
				{
					grayscale = "";
					button1.enabled = true;
					if (guiName == RESEARCH && !Engine.GuiInterfaceCall("CheckTechnologyRequirements", entType1))
					{
						button1.enabled = false;
						button1.tooltip += "\n" + GetTechnologyData(entType1).requirementsTooltip;
						grayscale = "grayscale:";
					}
					icon1.sprite = "stretched:" + grayscale + "session/portraits/" +template1.icon;
				}
				else
				{
					pair.hidden = true;
					button1.hidden = true;
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

	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;

	// We sort pairs upside down, so get the size from the topmost button.
	if (guiName == RESEARCH)
		buttonSideLength = getGUIObjectByName("unit"+guiName+"Button["+(rowLength*numRows)+"]").size.bottom;

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
		var pairSize = getGUIObjectByName("unit"+guiName+"Pair[0]").size;
		var pairSideWidth = pairSize.right;
		var pairSideHeight = pairSize.bottom;
		var pairSpacerHeight = pairSideHeight + 1;
		var pairSpacerWidth = pairSideWidth + 1;

		layoutRow("Pair", 0, guiName, pairSideWidth, pairSpacerWidth, pairSideHeight, pairSpacerHeight, 0, rowLength);
	}

	// Resize Queue panel if needed
	if (guiName == QUEUE) // or garrison
	{
		var panel = getGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

	// Hide unused pair buttons and symbols
	if (guiName == RESEARCH)
	{
		for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		{
			getGUIObjectByName("unit"+guiName+"Button["+(i+rowLength)+"]").hidden = true;
			getGUIObjectByName("unit"+guiName+"Pair["+i+"]").hidden = true;
		}
	}

	g_unitPanelButtons[guiName] = numButtons;
}

// Sets up "unit trading panel" - special case for setupUnitPanel
function setupUnitTradingPanel(unitEntState, selection)
{
	for (var i = 0; i < TRADING_RESOURCES.length; i++)
	{
		var resource = TRADING_RESOURCES[i];
		var button = getGUIObjectByName("unitTradingButton["+i+"]");
		button.size = (i * 46) + " 0 " + ((i + 1) * 46) + " 46";
		var selectTradingPreferredGoodsData = { "entities": selection, "preferredGoods": resource };
		button.onpress = (function(e){ return function() { selectTradingPreferredGoods(e); } })(selectTradingPreferredGoodsData);
		button.enabled = true;
		button.tooltip = "Set " + resource + " as trading goods";
		var icon = getGUIObjectByName("unitTradingIcon["+i+"]");
		var preferredGoods = unitEntState.trader.preferredGoods;
		var selected = getGUIObjectByName("unitTradingSelection["+i+"]");
		selected.hidden = !(resource == preferredGoods);
		var grayscale = (resource != preferredGoods) ? "grayscale:" : "";
		icon.sprite = "stretched:"+grayscale+"session/icons/resources/" + resource + ".png";
	}
}

// Sets up "unit barter panel" - special case for setupUnitPanel
function setupUnitBarterPanel(unitEntState)
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
				var selection = getGUIObjectByName("unitBarter" + action + "Selection["+i+"]");
				selection.hidden = !(i == g_barterSell);
			}

			// We gray out the not selected icons in 'sell' row
			var grayscale = (j == 0 && i != g_barterSell) ? "grayscale:" : "";
			var icon = getGUIObjectByName("unitBarter" + action + "Icon["+i+"]");

			var button = getGUIObjectByName("unitBarter" + action + "Button["+i+"]");
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
				amount = (i == g_barterSell) ? "-" + amountToSell : "";
			}
			else
			{
				var exchangeResourcesParameters = { "sell": BARTER_RESOURCES[g_barterSell], "buy": BARTER_RESOURCES[i], "amount": amountToSell };
				button.onpress = (function(exchangeResourcesParameters){ return function() { exchangeResources(exchangeResourcesParameters); } })(exchangeResourcesParameters);
				amount = amountToBuy;
			}
			getGUIObjectByName("unitBarter" + action + "Amount["+i+"]").caption = amount;
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
	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (selection.length > 1)
			setupUnitPanel(SELECTION, usedPanels, entState, g_Selection.groups.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );

		var commands = getEntityCommandsList(entState);
		if (commands.length)
			setupUnitPanel(COMMAND, usedPanels, entState, commands,
				function (item) { performCommand(entState.id, item.name); } );

		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			groups.add(entState.garrisonHolder.entities);
			setupUnitPanel(GARRISON, usedPanels, entState, groups.getTemplateNames(),
				function (item) { unload(entState.id, groups.getEntsByName(item)); } );
		}

		var formations = Engine.GuiInterfaceCall("GetAvailableFormations");
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && formations.length)
		{
			setupUnitPanel(FORMATION, usedPanels, entState, formations,
				function (item) { performFormation(entState.id, item); } );
		}

		// TODO: probably should load the stance list from a data file,
		// and/or vary depending on what units are selected
		var stances = ["violent", "aggressive", "passive", "defensive", "standground"];
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && stances.length)
		{
			setupUnitPanel(STANCE, usedPanels, entState, stances,
				function (item) { performStance(entState.id, item); } );
		}

		getGUIObjectByName("unitBarterPanel").hidden = !entState.barterMarket;
		if (entState.barterMarket)
		{
			usedPanels["Barter"] = 1;
			setupUnitBarterPanel(entState);
		}

		var buildableEnts = [];
		var trainableEnts = [];
		var state;
		// Get all buildable and trainable entities
		for (var i in selection)
		{
			if ((state = GetEntityState(selection[i])) && state.buildEntities && state.buildEntities.length)
				buildableEnts = buildableEnts.concat(state.buildEntities);
			if ((state = GetEntityState(selection[i])) && state.production && state.production.entities.length)
				trainableEnts = trainableEnts.concat(state.production.entities);
		}
		
		// Remove duplicates
		removeDupes(buildableEnts);
		removeDupes(trainableEnts);

		if (buildableEnts.length && ((trainableEnts.length && hasClass(entState, "Unit")) || !trainableEnts.length))
			setupUnitPanel(CONSTRUCTION, usedPanels, entState, buildableEnts, startBuildingPlacement);
		else if (trainableEnts.length)
			setupUnitPanel(TRAINING, usedPanels, entState, trainableEnts,
				function (trainEntType) { addTrainingToQueue(selection, trainEntType); } );
		
		if (entState.production && entState.production.technologies.length && selection.length == 1)
		{
			setupUnitPanel(RESEARCH, usedPanels, entState, entState.production.technologies,
				function (researchType) { addResearchToQueue(entState.id, researchType); } );
		}

		if (entState.production && entState.production.queue.length)
			setupUnitPanel(QUEUE, usedPanels, entState, entState.production.queue,
				function (item) { removeFromProductionQueue(entState.id, item.id); } );

		if (entState.trader)
		{
			usedPanels["Trading"] = 1;
			setupUnitTradingPanel(entState, selection);
		}
		
		if(!entState.foundation && (entState.gate || hasClass(entState, "StoneWall") && !hasClass(entState, "Tower")))
		{
			if (entState.gate)
			{
				var action = entState.gate.locked ? "Unlock gate": "Lock gate";
				setupUnitPanel(GATE, usedPanels, entState, [action],
					function (item) { lockGate(!entState.gate.locked); } );
			}
			else // Wall
			{ 
				// Only allow long walls section to be transformed to gates
				var longPos = entState.template.indexOf("long");
				if (longPos != -1)
				{
					var template = entState.template.substr(0, longPos) + "gate";
					setupUnitPanel(GATE, usedPanels, entState, ["Convert to gate"],
						function (item) { transformWallToGate(template); } );
				}
			}
		}
		
		supplementalDetailsPanel.hidden = false;
		commandsPanel.hidden = false;
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
		var panel = getGUIObjectByName("unit" + panelName + "Panel");
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
		getGUIObjectByName("unit" + panelName + "Panel").hidden = true;
}

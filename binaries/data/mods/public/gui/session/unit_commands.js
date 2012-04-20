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
var g_unitPanelButtons = {"Selection": 0, "Queue": 0, "Formation": 0, "Garrison": 0, "Training": 0, "Research": 0, "Barter": 0, "Trading": 0, "Construction": 0, "Command": 0, "Stance": 0};

// Unit panels are panels with row(s) of buttons
var g_unitPanels = ["Selection", "Queue", "Formation", "Garrison", "Training", "Barter", "Trading", "Construction", "Research", "Stance", "Command"];

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
function layoutButtonRow(rowNumber, guiName, buttonSideLength, buttonSpacer, startIndex, endIndex)
{
	var colNumber = 0;

	for (var i = startIndex; i < endIndex; i++)
	{
		var button = getGUIObjectByName("unit"+guiName+"Button["+i+"]");

		if (button)
		{
			var size = button.size;

			size.left = buttonSpacer*colNumber;
			size.right = buttonSpacer*colNumber + buttonSideLength;
			size.top = buttonSpacer*rowNumber;
			size.bottom = buttonSpacer*rowNumber + buttonSideLength;

			button.size = size;
			colNumber++;
		}
	}
}

// Sets up "unit panels" - the panels with rows of icons (Helper function for updateUnitDisplay)
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

		default:
			break;
	}

	// Make buttons
	var i;
	for (i = 0; i < numberOfItems; i++)
	{
		var item = items[i];
		
		// If a tech has been researched it leaves an empty slot
		if (guiName == RESEARCH && !item)
		{
			getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;
			continue;
		}
		
		// Get the entity type and load the template for that type if necessary
		var entType;
		var template;
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
				entType = item;
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
				var tooltip = toTitleCase(item);
				break;

			case TRAINING:
				var tooltip = getEntityNameWithGenericType(template);

				if (template.tooltip)
					tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

				var [batchSize, batchIncrement] = getTrainingBatchStatus(unitEntState.id, entType);
				var trainNum = batchSize ? batchSize+batchIncrement : batchIncrement;

				tooltip += "\n" + getEntityCost(template);

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

				tooltip += "\n" + getEntityCost(template);
				break;

			case CONSTRUCTION:
				var tooltip = getEntityNameWithGenericType(template);
				if (template.tooltip)
					tooltip += "\n[font=\"serif-13\"]" + template.tooltip + "[/font]";

				tooltip += "\n" + getEntityCost(template);

				tooltip += getPopulationBonus(template);
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
		var icon = getGUIObjectByName("unit"+guiName+"Icon["+i+"]");
		var selection = getGUIObjectByName("unit"+guiName+"Selection["+i+"]");
		button.hidden = false;
		button.tooltip = tooltip;

		// Button Function (need nested functions to get the closure right)
		button.onpress = (function(e){ return function() { callback(e) } })(item);

		// Get icon image
		if (guiName == "Formation")
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
		else if (guiName == "Stance")
		{
			var stanceSelected = Engine.GuiInterfaceCall("IsStanceSelected", {
				"ents": g_Selection.toList(),
				"stance": item
			});

			selection.hidden = !stanceSelected;
			icon.sprite = "stretched:session/icons/stances/"+item+".png";
		}
		else if (guiName == "Command")
		{
			icon.sprite = "stretched:session/icons/" + item.icon;

		}
		else if (template.icon)
		{
			var grayscale = "";
			button.enabled = true;
			
			if (template.requiredTechnology && !Engine.GuiInterfaceCall("IsTechnologyResearched", template.requiredTechnology))
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
		}
		else
		{
			// TODO: we should require all entities to have icons, so this case never occurs
			icon.sprite = "bkFillBlack";
		}
	}

	// Position the visible buttons (TODO: if there's lots, maybe they should be squeezed together to fit)
	var numButtons = i;

	var rowLength = 8;
	if (guiName == "Selection")
		rowLength = 4;
	else if (guiName == "Formation" || guiName == "Garrison" || guiName == "Command")
		rowLength = 4;

	var numRows = Math.ceil(numButtons / rowLength);
	var buttonSideLength = getGUIObjectByName("unit"+guiName+"Button[0]").size.bottom;
	var buttonSpacer = buttonSideLength+1;

	// Layout buttons
	if (guiName == "Command")
	{
		layoutButtonRowCentered(0, guiName, 0, numButtons, COMMANDS_PANEL_WIDTH);
	}
	else
	{
		for (var i = 0; i < numRows; i++)
			layoutButtonRow(i, guiName, buttonSideLength, buttonSpacer, rowLength*i, rowLength*(i+1) );
	}

	// Resize Queue panel if needed
	if (guiName == "Queue") // or garrison
	{
		var panel = getGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		size.top = (UNIT_PANEL_BASE - ((numRows-1)*UNIT_PANEL_HEIGHT));
		panel.size = size;
	}

	// Hide any buttons we're no longer using
	for (i = numButtons; i < g_unitPanelButtons[guiName]; ++i)
		getGUIObjectByName("unit"+guiName+"Button["+i+"]").hidden = true;

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

// Updates right Unit Commands Panel - runs in the main session loop via updateSelectionDetails()
function updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection)
{
	// Panels that are active
	var usedPanels = {};

	// If the selection is friendly units, add the command panels
	var player = Engine.GetPlayerID();
	if (entState.player == player || g_DevSettings.controlAll)
	{
		if (selection.length > 1)
			setupUnitPanel("Selection", usedPanels, entState, g_Selection.groups.getTemplateNames(),
				function (entType) { changePrimarySelectionGroup(entType); } );

		var commands = getEntityCommandsList(entState);
		if (commands.length)
			setupUnitPanel("Command", usedPanels, entState, commands,
				function (item) { performCommand(entState.id, item.name); } );

		if (entState.garrisonHolder)
		{
			var groups = new EntityGroups();
			groups.add(entState.garrisonHolder.entities);
			setupUnitPanel("Garrison", usedPanels, entState, groups.getTemplateNames(),
				function (item) { unload(entState.id, groups.getEntsByName(item)); } );
		}

		var formations = getEntityFormationsList(entState);
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && formations.length)
		{
			setupUnitPanel("Formation", usedPanels, entState, formations,
				function (item) { performFormation(entState.id, item); } );
		}

		// TODO: probably should load the stance list from a data file,
		// and/or vary depending on what units are selected
		var stances = ["violent", "aggressive", "passive", "defensive", "standground"];
		if (hasClass(entState, "Unit") && !hasClass(entState, "Animal") && !entState.garrisonHolder && stances.length)
		{
			setupUnitPanel("Stance", usedPanels, entState, stances,
				function (item) { performStance(entState.id, item); } );
		}

		getGUIObjectByName("unitBarterPanel").hidden = !entState.barterMarket;
		if (entState.barterMarket)
		{
			usedPanels["Barter"] = 1;
			setupUnitBarterPanel(entState);
		}

		if (entState.buildEntities && entState.buildEntities.length)
		{
			setupUnitPanel("Construction", usedPanels, entState, entState.buildEntities, startBuildingPlacement);
		}
		
		if (entState.production && entState.production.entities.length)
		{
			setupUnitPanel("Training", usedPanels, entState, entState.production.entities,
				function (trainEntType) { addTrainingToQueue(entState.id, trainEntType); } );
		}
		
		if (entState.production && entState.production.technologies.length)
		{
			setupUnitPanel("Research", usedPanels, entState, entState.production.technologies,
				function (researchType) { addResearchToQueue(entState.id, researchType); } );
		}

		if (entState.production && entState.production.queue.length)
			setupUnitPanel("Queue", usedPanels, entState, entState.production.queue,
				function (item) { removeFromProductionQueue(entState.id, item.id); } );

		if (entState.trader)
		{
			usedPanels["Trading"] = 1;
			setupUnitTradingPanel(entState, selection);
		}
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

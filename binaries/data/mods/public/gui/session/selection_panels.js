/**
 * Contains the layout and button settings per selection panel
 *
 * getItems returns a list of basic items used to fill the panel.
 * This method is obligated. If the items list is empty, the panel
 * won't be rendered.
 * 
 * Then there's a loop over all items provided. In the loop, 
 * the item and some other standard data is added to a data object.
 *
 * The standard data is 
 * var data = {
 *   "i":              index
 *   "item":           item coming from the getItems function
 *   "selection":      list of currently selected items
 *   "playerState":    playerState
 *   "unitEntState":   first selected entity state
 *   "rowLength":      rowLength
 *   "numberOfItems":  number of items that will be processed
 *   "button":         gui Button object
 *   "affordableMask": gui Unaffordable overlay
 *   "icon":           gui Icon object
 *   "guiSelection":   gui button Selection overlay
 *   "countDisplay":   gui caption space
 * };
 *
 * Then, addData is called, and can be used to abort the processing 
 * of the current item by returning false. 
 * It should return true if you want the panel to be filled.
 *
 * addData is used to add data to the data object on top 
 * (or instead of) the standard data.
 * addData is not obligated, the function will just continue
 * with the content setters if no addData is present.
 * 
 * After the addData, all functions starting with "set" are called. 
 * These are used to set various parts of content.
 */

var g_SelectionPanels = {};

// BARTER
g_SelectionPanels.Barter = {
	"getMaxNumberOfItems": function()
	{
		return 4;
	},
	"rowLength": 4,
	"getItems": function(unitEntState, selection)
	{
		if (!unitEntState.barterMarket)
			return [];
		// ["food", "wood", "stone", "metal"]
		return BARTER_RESOURCES;
	},
	"addData": function(data)
	{
		// data.item is the resource name in this case
		data.button = {};
		data.icon = {};
		data.amount = {};
		for (var a of BARTER_ACTIONS)
		{
			data.button[a] = Engine.GetGUIObjectByName("unitBarter"+a+"Button["+data.i+"]");
			data.icon[a] = Engine.GetGUIObjectByName("unitBarter"+a+"Icon["+data.i+"]");
			data.amount[a] = Engine.GetGUIObjectByName("unitBarter"+a+"Amount["+data.i+"]");
		}
		data.selection = Engine.GetGUIObjectByName("unitBarterSellSelection["+data.i+"]");
		data.affordableMask = Engine.GetGUIObjectByName("unitBarterSellUnaffordable["+data.i+"]");

		data.amountToSell = BARTER_RESOURCE_AMOUNT_TO_SELL;
		if (Engine.HotkeyIsPressed("session.massbarter"))
			data.amountToSell *= BARTER_BUNCH_MULTIPLIER;
		data.isSelected = data.item == g_barterSell;
		return true;
	},
	"setCountDisplay": function(data)
	{
		data.amount.Sell.caption = "-" + data.amountToSell;
		var sellPrice = data.unitEntState.barterMarket.prices["sell"][g_barterSell];
		var buyPrice = data.unitEntState.barterMarket.prices["buy"][data.item];
		data.amount.Buy.caption = "+" + Math.round(sellPrice / buyPrice * data.amountToSell);
	},
	"setTooltip": function(data)
	{
		var resource = getLocalizedResourceName(data.item, "withinSentence");
		data.button.Buy.tooltip = sprintf(translate("Buy %(resource)s"), {"resource": resource});
		data.button.Sell.tooltip = sprintf(translate("Sell %(resource)s"), {"resource": resource});
	},
	"setAction": function(data)
	{
		data.button.Sell.onPress = function() { g_barterSell = data.item; };
		var exchangeResourcesParameters = {
			"sell": g_barterSell,
			"buy": data.item,
			"amount": data.amountToSell
		};
		data.button.Buy.onPress = function() { exchangeResources(exchangeResourcesParameters); };
	},
	"setGraphics": function(data)
	{
		var grayscale = data.isSelected ? "grayscale:" : ""; 
		data.button.Buy.hidden = data.isSelected;
		data.button.Sell.hidden = false;
		for each (var icon in data.icon)
			icon.sprite = "stretched:"+grayscale+"session/icons/resources/" + data.item + ".png";

		var neededRes = {};
		neededRes[data.item] = data.amountToSell;
		if (Engine.GuiInterfaceCall("GetNeededResources", neededRes))
			data.affordableMask.hidden = false;
		else
			data.affordableMask.hidden = true;
		data.selection.hidden = !data.isSelected;
	},
	"setPosition": function(data)
	{
		setPanelObjectPosition(data.button.Sell, data.i, data.rowLength);
		setPanelObjectPosition(data.button.Buy, data.i + data.rowLength, data.rowLength);
	},
},

// COMMAND
g_SelectionPanels.Command = {
	"getMaxNumberOfItems": function()
	{
		return 6;
	},
	"getItems": function(unitEntState)
	{
		var commands = [];
		for (var c in g_EntityCommands)
		{
			var info = g_EntityCommands[c].getInfo(unitEntState);
			if (!info)
				continue;
			info.name = c;
			commands.push(info);
		}
		return commands;
	},
	"setTooltip": function(data)
	{
		if (data.item.tooltip)
			data.button.tooltip = data.item.tooltip
		else
			data.button.tooltip = toTitleCase(data.item.name);
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { data.item.callback ? data.item.callback(data.item) : performCommand(data.unitEntState.id, data.item.name); };
	},
	"setCountDisplay": function(data)
	{
		data.countDisplay.caption = data.item.count || "";
	},
	"setGraphics": function(data)
	{
		data.icon.sprite = "stretched:session/icons/" + data.item.icon;
	},
	"setPosition": function(data)
	{
		var size = data.button.size;
		// count on square buttons, so size.bottom is the width too
		var spacer = size.bottom + 1;
		// relative to the center ( = 50%)
		size.rleft = size.rright = 50;
		// offset from the center calculation
		size.left = (data.i - data.numberOfItems/2) * spacer;
		size.right = size.left + size.bottom;
		data.button.size = size;
	},
};

// CONSTRUCTION
g_SelectionPanels.Construction = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function()
	{
		return getAllBuildableEntitiesFromSelection();
	},
	"addData": function(data)
	{
		data.entType = data.item;
		data.template = GetTemplateData(data.entType);
		if (!data.template) // abort if no template
			return false;
		data.technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", data.template.requiredTechnology);
		if (data.template.cost)
		{
			var totalCost = multiplyEntityCosts(data.template, 1);
			data.neededResources = Engine.GuiInterfaceCall("GetNeededResources", totalCost);
		}
		data.limits = getEntityLimitAndCount(data.playerState, data.entType);
		return true;
	},
	"setAction": function(data)
	{
		data.button.onPress = function () { startBuildingPlacement(data.item, data.playerState); };
	},
	"setTooltip": function(data)
	{
		var tooltip = getEntityNamesFormatted(data.template);
		tooltip += getVisibleEntityClassesFormatted(data.template);

		if (data.template.tooltip)
			tooltip += "\n[font=\"sans-13\"]" + data.template.tooltip + "[/font]";

		tooltip += "\n" + getEntityCostTooltip(data.template);
		tooltip += getPopulationBonusTooltip(data.template);

		tooltip += formatLimitString(data.limits.entLimit, data.limits.entCount, data.limits.entLimitChangers);
		if (!data.technologyEnabled)
		{
			var techName = getEntityNames(GetTechnologyData(data.template.requiredTechnology));
			tooltip += "\n" + sprintf(translate("Requires %(technology)s"), { technology: techName });
		}
		if (data.neededResources)
			tooltip += getNeededResourcesTooltip(data.neededResources);
		data.button.tooltip = tooltip;
		return true;
	},
	"setGraphics": function(data)
	{
		var grayscale = "";
		if (!data.technologyEnabled || data.limits.canBeAddedCount == 0)
		{
			data.button.enabled = false;
			grayscale = "grayscale:";
			data.affordableMask.hidden = false;
			data.affordableMask.sprite = "colour: 0 0 0 127";
		}
		else if (data.neededResources)
		{
			data.button.enabled = false;
			data.affordableMask.hidden = false;
			data.affordableMask.sprite = resourcesToAlphaMask(data.neededResources);
		}
		if (data.template.icon)
			data.icon.sprite = "stretched:" + grayscale + "session/portraits/" + data.template.icon;
	},
	"setPosition": function(data)
	{
		var index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
	},
};

// FORMATION
g_SelectionPanels.Formation = {
	"getMaxNumberOfItems": function()
	{
		return 16
	},
	"rowLength": 4,
	"conflictsWith": ["Garrison"],
	"getItems": function(unitEntState)
	{
		if (!hasClass(unitEntState, "Unit") || hasClass(unitEntState, "Animal"))
			return [];
		return Engine.GuiInterfaceCall("GetAvailableFormations");
	},
	"addData": function(data)
	{
		data.formationInfo = Engine.GuiInterfaceCall("GetFormationInfoFromTemplate", {"templateName": data.item});
		data.formationOk = canMoveSelectionIntoFormation(data.item);
		data.formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
			"ents": data.selection,
			"formationTemplate": data.item
		});
		return true;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { performFormation(data.unitEntState.id, data.item); };
	},
	"setTooltip": function(data)
	{
		var tooltip = translate(data.formationInfo.name);
		if (!data.formationOk && data.formationInfo.tooltip)
			tooltip += "\n" + "[color=\"red\"]" + translate(data.formationInfo.tooltip) + "[/color]";
		data.button.tooltip = tooltip;
	},
	"setGraphics": function(data)
	{
		data.button.enabled = data.formationOk;
		var grayscale = data.formationOk ? "" : "grayscale:";
		data.guiSelection.hidden = !data.formationSelected;
		data.icon.sprite = "stretched:"+grayscale+"session/icons/"+data.formationInfo.icon;
	},
};

// GARRISON
g_SelectionPanels.Garrison = {
	"getMaxNumberOfItems": function()
	{
		return 12;
	},
	"rowLength": 4,
	"getItems": function(unitEntState, selection)
	{
		if (!unitEntState.garrisonHolder)
			return [];
		var groups = new EntityGroups();
		for (var ent of selection)
		{
			var state = GetEntityState(ent);
			if (state.garrisonHolder)
				groups.add(state.garrisonHolder.entities)
		}
		return groups.getEntsGrouped();
	},
	"addData": function(data)
	{
		data.entType = data.item.template;
		data.template = GetTemplateData(data.entType);
		if (!data.template)
			return false;
		data.name = getEntityNames(data.template);
		data.count = data.item.ents.length;
		return true;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { unloadTemplate(data.item.template); };
	},
	"setTooltip": function(data)
	{
		var tooltip = sprintf(translate("Unload %(name)s"), { name: data.name }) + "\n";
		tooltip += translate("Single-click to unload 1. Shift-click to unload all of this type.");
		data.button.tooltip = tooltip;
	},
	"setCountDisplay": function(data)
	{
		data.countDisplay.caption = data.count || "";
	},
	"setGraphics": function(data)
	{
		var grayscale = "";
		var ents = data.item.ents;
		var entplayer = GetEntityState(ents[0]).player;
		data.button.sprite = "colour: " + rgbToGuiColor(g_Players[entplayer].color);

		var player = Engine.GetPlayerID();
		if(player != data.unitEntState.player && !g_DevSettings.controlAll)
		{
			if (data.entplayer != player)
			{
				data.button.enabled = false;
				grayscale = "grayscale:";
			}
		}
		data.icon.sprite = "stretched:" + grayscale + "session/portraits/" + data.template.icon;
	},
};

// GATE
g_SelectionPanels.Gate = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function(unitEntState, selection)
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
		return items;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() {data.item.callback(data.item); };
	},
	"setTooltip": function(data)
	{
		var tooltip = data.item.tooltip;
		if (data.item.template)
		{
			data.template = GetTemplateData(data.item.template);
			data.wallCount = data.selection.reduce(function (count, ent) {
					var state = GetEntityState(ent);
					if (hasClass(state, "LongWall") && !state.gate)
						count++;
					return count;
				}, 0);

			tooltip += "\n" + getEntityCostTooltip(data.template, data.wallCount);


			data.neededResources = Engine.GuiInterfaceCall("GetNeededResources", multiplyEntityCosts(data.template, data.wallCount));
			if (data.neededResources)
				tooltip += getNeededResourcesTooltip(data.neededResources);
		}
		data.button.tooltip = tooltip;
	},
	"setGraphics": function(data)
	{
		data.affordableMask.hidden == data.neededResources ? true : false;
		var gateIcon;
		if (data.item.gate)
		{
			// If already a gate, show locking actions
			gateIcon = "icons/lock_" + GATE_ACTIONS[data.item.locked ? 0 : 1] + "ed.png";
			if (data.item.gate.locked === undefined)
				data.guiSelection.hidden = false
			else
				data.guiSelection.hidden = data.item.gate.locked != data.item.locked;
		}
		else
		{
			// otherwise show gate upgrade icon
			var template = GetTemplateData(data.item.template);
			if (!template)
				return;
			gateIcon = data.template.icon ? "portraits/" + data.template.icon : "icons/gate_closed.png";
			data.guiSelection.hidden = true;
		}

		data.icon.sprite = "stretched:session/" + gateIcon;
	},
	"setPosition": function(data)
	{
		var index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
	},
};

// PACK
g_SelectionPanels.Pack = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function(unitEntState, selection)
	{
		var checks = {};
		for (var ent of selection)
		{
			var state = GetEntityState(ent);
			if (!state.pack)
				continue;
			if (state.pack.progress == 0)
			{
				if (!state.pack.packed)
					checks.packButton = true;
				else if (state.pack.packed)
					checks.unpackButton = true;
			}
			else
			{
				// Already un/packing - show cancel button
				if (!state.pack.packed)
					checks.packCancelButton = true;
				else if (state.pack.packed)
					checks.unpackCancelButton = true;
			}
		}
		var items = [];
		if (checks.packButton)
			items.push({ "packing": false, "packed": false, "tooltip": translate("Pack"), "callback": function() { packUnit(true); } });
		if (checks.unpackButton)
			items.push({ "packing": false, "packed": true, "tooltip": translate("Unpack"), "callback": function() { packUnit(false); } });
		if (checks.packCancelButton)
			items.push({ "packing": true, "packed": false, "tooltip": translate("Cancel Packing"), "callback": function() { cancelPackUnit(true); } });
		if (checks.unpackCancelButton)
			items.push({ "packing": true, "packed": true, "tooltip": translate("Cancel Unpacking"), "callback": function() { cancelPackUnit(false); } });
		return items;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() {data.item.callback(data.item); };
	},
	"setTooltip": function(data)
	{
		data.button.tooltip = data.item.tooltip;
	},
	"setGraphics": function(data)
	{
		if (data.item.packing)
			data.icon.sprite = "stretched:session/icons/cancel.png";
		else if (data.item.packed)
			data.icon.sprite = "stretched:session/icons/unpack.png";
		else
			data.icon.sprite = "stretched:session/icons/pack.png";
	},
	"setPosition": function(data)
	{
		var index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
	},
};

// QUEUE
g_SelectionPanels.Queue = {
	"getMaxNumberOfItems": function()
	{
		return 16;
	},
	"getItems": function(unitEntState, selection)
	{
		return getTrainingQueueItems(selection);
	},
	"resizePanel": function(numberOfItems, rowLength)
	{
		var numRows = Math.ceil(numberOfItems / rowLength);
		var panel = Engine.GetGUIObjectByName("unitQueuePanel");
		var size = panel.size;
		var buttonSize = Engine.GetGUIObjectByName("unitQueueButton[0]").size.bottom;
		var margin = 4;
		size.top = size.bottom - numRows*buttonSize - (numRows+2)*margin;
		panel.size = size;
	},
	"addData": function(data)
	{
		// differentiate between units and techs
		if (data.item.unitTemplate)
		{
			data.entType = data.item.unitTemplate;
			data.template = GetTemplateData(data.entType);
		}
		else if (data.item.technologyTemplate)
		{
			data.entType = data.item.technologyTemplate;
			data.template = GetTechnologyData(data.entType);
		}
		data.progress = Math.round(data.item.progress*100) + "%";

		return data.template;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { removeFromProductionQueue(data.item.producingEnt, data.item.id); };
	},
	"setTooltip": function(data)
	{
		var tooltip = getEntityNames(data.template);
		if (data.item.neededSlots)
		{
			tooltip += "\n[color=\"red\"]" + translate("Insufficient population capacity:") + "\n[/color]";
			tooltip += sprintf(translate("%(population)s %(neededSlots)s"), { population: getCostComponentDisplayName("population"), neededSlots: data.item.neededSlots });
		}
		data.button.tooltip = tooltip;
	},
	"setCountDisplay": function(data)
	{
		data.countDisplay.caption = data.item.count > 1 ? data.item.count : "";
	},
	"setProgressDisplay": function(data)
	{
		// show the progress number for the first item
		if (data.i == 0)
			Engine.GetGUIObjectByName("queueProgress").caption = data.progress;

		var guiObject = Engine.GetGUIObjectByName("unitQueueProgressSlider["+data.i+"]");
		var size = guiObject.size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(data.item.progress * (size.right - size.left));
		guiObject.size = size;
	},
	"setGraphics": function(data)
	{
		if (data.template.icon)
			data.icon.sprite = "stretched:session/portraits/" + data.template.icon;
	},
};

// RESEARCH
g_SelectionPanels.Research = {
	"getMaxNumberOfItems": function()
	{
		return 8;
	},
	"getItems": function(unitEntState, selection)
	{
		// TODO 8 is the row lenght, make variable
		if (getNumberOfRightPanelButtons() > 8 && selection.length > 1)
			return [];
		for (var ent of selection)
		{
			var entState = GetEntityState(ent);
			if (entState.production && entState.production.technologies.length)
				return entState.production.technologies
		}
		return [];
	},
	"hideItem": function(i, rowLength) // called when no item is found
	{
		Engine.GetGUIObjectByName("unitResearchButton["+i+"]").hidden = true;
		// We also remove the paired tech and the pair symbol
		Engine.GetGUIObjectByName("unitResearchButton["+(i+rowLength)+"]").hidden = true;
		Engine.GetGUIObjectByName("unitResearchPair["+i+"]").hidden = true;
	},
	"addData": function(data)
	{
		data.entType = data.item.pair ? [data.item.top, data.item.bottom] : [data.item];
		data.template = data.entType.map(GetTechnologyData);
		// abort if no template found for any of the techs
		if (!data.template.every(function(v) { return v; }))
			return false;
		// index one row below
		var shiftedIndex = data.i + data.rowLength;
		data.positions = data.item.pair ? [data.i, shiftedIndex] : [shiftedIndex];
		data.positionsToHide = data.item.pair ? [] : [data.i];

		// add top buttons to the data
		data.button = data.positions.map(function(p) { 
			return Engine.GetGUIObjectByName("unitResearchButton["+p+"]"); 
		});

		data.buttonsToHide = data.positionsToHide.map(function(p) { 
			return Engine.GetGUIObjectByName("unitResearchButton["+p+"]"); 
		});


		data.affordableMask = data.positions.map(function(p) { 
			return Engine.GetGUIObjectByName("unitResearchUnaffordable["+p+"]");
		});

		data.icon = data.positions.map(function(p) { 
			return Engine.GetGUIObjectByName("unitResearchIcon["+p+"]");
		});

		data.unchosenIcon = data.positions.map(function(p) { 
			return Engine.GetGUIObjectByName("unitResearchUnchosenIcon["+p+"]");
		});

		data.neededResources = data.template.map(function(t) { 
			return Engine.GuiInterfaceCall("GetNeededResources", t.cost);
		});

		data.requirementsPassed = data.entType.map(function(e) { 
			return Engine.GuiInterfaceCall("CheckTechnologyRequirements",e); 
		});

		data.pair = Engine.GetGUIObjectByName("unitResearchPair["+data.i+"]");

		return true;
	},
	"setTooltip": function(data)
	{
		for (var i in data.entType)
		{
			var tooltip = "";
			var template = data.template[i];
			tooltip = getEntityNamesFormatted(template);
			if (template.tooltip)
				tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

			tooltip += "\n" + getEntityCostTooltip(template);
			if (!data.requirementsPassed[i])
			{
				tooltip += "\n" + template.requirementsTooltip;
				if (template.classRequirements)
				{
					var player = Engine.GetPlayerID();
					var current = GetSimState().players[player].classCounts[template.classRequirements.class] || 0;
					var remaining = template.classRequirements.number - current;
					tooltip += " " + sprintf(translatePlural("Remaining: %(number)s to build.", "Remaining: %(number)s to build.", remaining), { number: remaining});
				}
			}
			if (data.neededResources[i])
				tooltip += getNeededResourcesTooltip(data.neededResources[i]);
			data.button[i].tooltip = tooltip;
		}
	},
	"setAction": function(data)
	{
		for (var i in data.entType)
		{
			// array containing the indices other buttons
			var others = Object.keys(data.template);
			others.splice(i, 1);
			var button = data.button[i];
			// as we're in a loop, we need to limit the scope with a closure
			// else the last value of the loop will be taken, rather than the current one
			button.onpress = (function(template) { return function () { addResearchToQueue(data.unitEntState.id, template); }; })(data.entType[i]);
			// on mouse enter, show a cross over the other icons
			button.onmouseenter = (function(others, icons) {
				return function() {
					for (var j of others)
						icons[j].hidden = false;
				};
			})(others, data.unchosenIcon);
			button.onmouseleave = (function(others, icons) {
				return function() {
					for (var j of others)
						icons[j].hidden = true;
				};
			})(others, data.unchosenIcon);
		}
	},
	"setGraphics": function(data)
	{
		for (var i in data.entType)
		{
			var button = data.button[i];
			button.hidden = false;
			var grayscale = "";
			if (!data.requirementsPassed[i])
			{
				button.enabled = false;
				grayscale = "grayscale:";
				data.affordableMask[i].hidden = false;
				data.affordableMask[i].sprite = "colour: 0 0 0 127";
			}
			else if (data.neededResources[i])
			{
				button.enabled = false;
				data.affordableMask[i].hidden = false;
				data.affordableMask[i].sprite = resourcesToAlphaMask(data.neededResources[i]);
			}
			else
			{
				data.affordableMask[i].hidden = true;
				button.enabled = true; 
			}
			if (data.template[i].icon)
				data.icon[i].sprite = "stretched:" + grayscale + "session/portraits/" + data.template[i].icon;
		}
		for (var button of data.buttonsToHide)
			button.hidden = true;
		// show the tech connector
		data.pair.hidden = data.item.pair == null;
	},
	"setPosition": function(data)
	{
		for (var i in data.button)
			setPanelObjectPosition(data.button[i], data.positions[i], data.rowLength);
		setPanelObjectPosition(data.pair, data.i, data.rowLength);
	},
};

// SELECTION
g_SelectionPanels.Selection = {
	"getMaxNumberOfItems": function()
	{
		return 16;
	},
	"rowLength": 4,
	"getItems": function(unitEntState, selection)
	{
		if (selection.length < 2)
			return [];
		return g_Selection.groups.getTemplateNames();
	},
	"addData": function(data)
	{
		data.entType = data.item;
		data.template = GetTemplateData(data.entType);
		if (!data.template)
			return false;
		data.name = getEntityNames(data.template);
		data.count = g_Selection.groups.getCount(data.item);
		return true;
	},
	"setTooltip": function(data)
	{
		data.button.tooltip = data.name;
	},
	"setCountDisplay": function(data)
	{
		data.countDisplay.caption = data.count || "";
	},
	"setAction": function(data)
	{
		data.button.onpressright = function() { changePrimarySelectionGroup(data.item, true); };
		data.button.onpress = function() { changePrimarySelectionGroup(data.item, false); };
	},
	"setGraphics": function(data)
	{
		if (data.template.icon)
			data.icon.sprite = "stretched:session/portraits/" + data.template.icon;
	},
};

// STANCE
g_SelectionPanels.Stance = {
	"getMaxNumberOfItems": function()
	{
		return 5;
	},
	"getItems": function(unitEntState)
	{
		if (!unitEntState.unitAI || !hasClass(unitEntState, "Unit") || hasClass(unitEntState, "Animal"))
			return [];
		return unitEntState.unitAI.possibleStances;
	},
	"addData": function(data)
	{
		data.stanceSelected = Engine.GuiInterfaceCall("IsStanceSelected", {
			"ents": data.selection,
			"stance": data.item
		});
		return true;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { performStance(data.unitEntState, data.item); };
	},
	"setTooltip": function(data)
	{
		data.button.tooltip = getStanceDisplayName(data.item);
	},
	"setGraphics": function(data)
	{
		data.guiSelection.hidden = !data.stanceSelected;
		data.icon.sprite = "stretched:session/icons/stances/"+data.item+".png";
	},
};

// TRAINING
g_SelectionPanels.Training = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function()
	{
		return getAllTrainableEntitiesFromSelection();
	},
	"addData": function(data)
	{
		data.entType = data.item;
		data.template = GetTemplateData(data.entType);
		if (!data.template)
			return false;
		data.technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", data.template.requiredTechnology);

		var [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch] =
			getTrainingBatchStatus(data.playerState, data.unitEntState.id, data.entType, data.selection);
		data.buildingsCountToTrainFullBatch = buildingsCountToTrainFullBatch;
		data.fullBatchSize = fullBatchSize;
		data.remainderBatch = remainderBatch;
		data.trainNum = buildingsCountToTrainFullBatch || 1; // train at least one unit
		if (Engine.HotkeyIsPressed("session.batchtrain"))
			data.trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

		if (data.template.cost)
		{
			var totalCosts = multiplyEntityCosts(data.template, data.trainNum);
			data.neededResources = Engine.GuiInterfaceCall("GetNeededResources", totalCosts);
		}

		return true;
	},
	"setAction": function(data)
	{
		data.button.onPress = function() { addTrainingToQueue(data.selection, data.item, data.playerState); };
	},
	"setCountDisplay": function(data)
	{
		var count = data.trainNum > 1 ? data.trainNum : "";
		data.countDisplay.caption = count;
	},
	"setTooltip": function(data)
	{
		var tooltip = "";
		var key = Engine.ConfigDB_GetValue("user", "hotkey.session.queueunit." + (data.i + 1));
		if (key)
			tooltip += "[color=\"255 251 131\"][font=\"sans-bold-16\"][" + key + "][/font][/color] ";

		tooltip += getEntityNamesFormatted(data.template);
		tooltip += getVisibleEntityClassesFormatted(data.template);

		if (data.template.auras)
		{
			for (var auraName in data.template.auras)
			{
				tooltip += "\n[font=\"sans-bold-13\"]" + translate(auraName) + "[/font]";
				if (data.template.auras[auraName])
					tooltip += ": " + translate(data.template.auras[auraName]);
			}
		}

		if (data.template.tooltip)
			tooltip += "\n[font=\"sans-13\"]" + data.template.tooltip + "[/font]";

		tooltip += "\n" + getEntityCostTooltip(data.template, data.trainNum, data.unitEntState.id);

		data.limits = getEntityLimitAndCount(data.playerState, data.entType);

		tooltip += formatLimitString(data.limits.entLimit, data.limits.entCount, data.limits.entLimitChangers);
		if (Engine.ConfigDB_GetValue("user", "showdetailedtooltips") === "true")
		{
			if (data.template.health)
				tooltip += "\n[font=\"sans-bold-13\"]" + translate("Health:") + "[/font] " + data.template.health;
			if (data.template.attack)
				tooltip += "\n" + getEntityAttack(data.template);
			if (data.template.armour)
				tooltip += "\n[font=\"sans-bold-13\"]" + translate("Armor:") + "[/font] " + armorTypesToText(data.template.armour);
			if (data.template.speed)
				tooltip += "\n" + getEntitySpeed(data.template);
		}
		tooltip += "[color=\"255 251 131\"]" + formatBatchTrainingString(data.buildingsCountToTrainFullBatch, data.fullBatchSize, data.remainderBatch) + "[/color]";
		if (!data.technologyEnabled)
		{
			var techName = getEntityNames(GetTechnologyData(data.template.requiredTechnology));
			tooltip += "\n" + sprintf(translate("Requires %(technology)s"), { technology: techName });
		}
		if (data.neededResources)
			tooltip += getNeededResourcesTooltip(data.neededResources);

		data.button.tooltip = tooltip;
	},
	// disable and enable buttons in the same way as when you do for the construction
	"setGraphics": g_SelectionPanels.Construction.setGraphics,
	"setPosition": function(data)
	{
		var index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
	},
};



/**
 * If two panels need the same space, so they collide,
 * the one appearing first in the order is rendered.
 *
 * Note that the panel needs to appear in the list to get rendered.
 */
var g_PanelsOrder = [
	// LEFT PANE
	"Barter", // must always be visible on markets
	"Garrison", // more important than Formation, as you want to see the garrisoned units in ships
	"Formation",
	"Stance", // normal together with formation

	// RIGHT PANE
	"Gate", // must always be shown on gates
	"Pack", // must always be shown on packable entities
	"Training",
	"Construction",
	"Research", // normal together with training

	// UNIQUE PANES (importance doesn't matter)
	"Command",
	"Queue",
	"Selection",
];

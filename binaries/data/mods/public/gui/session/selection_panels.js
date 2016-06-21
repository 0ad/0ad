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
 * {
 *   "i":              index
 *   "item":           item coming from the getItems function
 *   "selection":      list of currently selected items
 *   "playerState":    playerState
 *   "unitEntState":   first selected entity state
 *   "rowLength":      rowLength
 *   "numberOfItems":  number of items that will be processed
 *   "button":         gui Button object
 *   "icon":           gui Icon object
 *   "guiSelection":   gui button Selection overlay
 *   "countDisplay":   gui caption space
 * }
 *
 * Then for every data object, the setupButton function is called which
 * sets the view and handlers of the button.
 */

// Cache some formation info
// Available formations per player
let g_AvailableFormations = new Map();
let g_FormationsInfo = new Map();

let g_SelectionPanels = {};

g_SelectionPanels.Alert = {
	"getMaxNumberOfItems": function()
	{
		return 2;
	},
	"getItems": function(unitEntState)
	{
		if (!unitEntState.alertRaiser)
			return [];
		return ["increase", "end"];
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() {
			if (data.item == "increase")
				increaseAlertLevel();
			else if (data.item == "end")
				endOfAlert();
		};

		if (data.item == "increase")
		{
			if (data.unitEntState.alertRaiser.hasRaisedAlert)
				data.button.tooltip = translate("Increase the alert level to protect more units");
			else
				data.button.tooltip = translate("Raise an alert!");
		}
		else if (data.item == "end")
			data.button.tooltip = translate("End of alert.");

		if (data.item == "increase")
		{
			data.button.hidden = !data.unitEntState.alertRaiser.canIncreaseLevel;
			if (data.unitEntState.alertRaiser.hasRaisedAlert)
				data.icon.sprite = "stretched:session/icons/bell_level2.png";
			else
				data.icon.sprite = "stretched:session/icons/bell_level1.png";
		}
		else if (data.item == "end")
		{
			data.button.hidden = !data.unitEntState.alertRaiser.hasRaisedAlert;
			data.icon.sprite = "stretched:session/icons/bell_level0.png";
		}
		data.button.enabled = !data.button.hidden && controlsPlayer(data.unitEntState.player);

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

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
	"setupButton": function(data)
	{
		// data.item is the resource name in this case
		let button = {};
		let icon = {};
		let amount = {};
		for (let a of BARTER_ACTIONS)
		{
			button[a] = Engine.GetGUIObjectByName("unitBarter" + a + "Button[" + data.i + "]");
			icon[a] = Engine.GetGUIObjectByName("unitBarter" + a + "Icon[" + data.i + "]");
			amount[a] = Engine.GetGUIObjectByName("unitBarter" + a + "Amount[" + data.i + "]");
		}
		let selectionIcon = Engine.GetGUIObjectByName("unitBarterSellSelection[" + data.i + "]");

		let amountToSell = BARTER_RESOURCE_AMOUNT_TO_SELL;
		if (Engine.HotkeyIsPressed("session.massbarter"))
			amountToSell *= BARTER_BUNCH_MULTIPLIER;

		amount.Sell.caption = "-" + amountToSell;
		let prices = data.unitEntState.barterMarket.prices;
		amount.Buy.caption = "+" + Math.round(prices.sell[g_BarterSell] / prices.buy[data.item] * amountToSell);

		let resource = getLocalizedResourceName(data.item, "withinSentence");
		button.Buy.tooltip = sprintf(translate("Buy %(resource)s"), { "resource": resource });
		button.Sell.tooltip = sprintf(translate("Sell %(resource)s"), { "resource": resource });

		button.Sell.onPress = function() {
			g_BarterSell = data.item;
			updateSelectionDetails();
		};

		button.Buy.onPress = function() { 
			Engine.PostNetworkCommand({
				"type": "barter",
				"sell": g_BarterSell,
				"buy": data.item,
				"amount": amountToSell
			});
		};

		let isSelected = data.item == g_BarterSell;
		let grayscale = isSelected ? "color: 0 0 0 100:grayscale:" : "";

		// do we have enough of this resource to sell?
		let neededRes = {};
		neededRes[data.item] = amountToSell;
		let canSellCurrent = Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": neededRes,
			"player": data.unitEntState.player
		}) ? "color:255 0 0 80:" : "";

		// Let's see if we have enough resources to barter.
		neededRes = {};
		neededRes[g_BarterSell] = amountToSell;
		let canBuyAny = Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": neededRes,
			"player": data.unitEntState.player
		}) ? "color:255 0 0 80:" : "";

		icon.Sell.sprite = canSellCurrent + "stretched:" + grayscale + "session/icons/resources/" + data.item + ".png";
		icon.Buy.sprite = canBuyAny + "stretched:" + grayscale + "session/icons/resources/" + data.item + ".png";

		button.Buy.hidden = isSelected;
		button.Buy.enabled = controlsPlayer(data.unitEntState.player);
		button.Sell.hidden = false;
		selectionIcon.hidden = !isSelected;

		setPanelObjectPosition(button.Sell, data.i, data.rowLength);
		setPanelObjectPosition(button.Buy, data.i + data.rowLength, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Command = {
	"getMaxNumberOfItems": function()
	{
		return 6;
	},
	"getItems": function(unitEntState)
	{
		let commands = [];

		for (let c in g_EntityCommands)
		{
			let info = g_EntityCommands[c].getInfo(unitEntState);
			if (!info)
				continue;

			info.name = c;
			commands.push(info);
		}
		return commands;
	},
	"setupButton": function(data)
	{
		data.button.tooltip = data.item.tooltip;

		data.button.onPress = function() {
			if (data.item.callback)
				data.item.callback(data.item);
			else
				performCommand(data.unitEntState.id, data.item.name);
		};

		data.countDisplay.caption = data.item.count || "";

		data.button.enabled =
			controlsPlayer(data.unitEntState.player) ||
			(g_IsObserver && data.item.name == "focus-rally");

		data.icon.sprite = "stretched:session/icons/" + data.item.icon;

		let size = data.button.size;
		// count on square buttons, so size.bottom is the width too
		let spacer = size.bottom + 1;
		// relative to the center ( = 50%)
		size.rleft = size.rright = 50;
		// offset from the center calculation
		size.left = (data.i - data.numberOfItems/2) * spacer;
		size.right = size.left + size.bottom;
		data.button.size = size;
		return true;
	}
};

g_SelectionPanels.AllyCommand = {
	"getMaxNumberOfItems": function()
	{
		return 2;
	},
	"conflictsWith": ["Command"],
	"getItems": function(unitEntState)
	{
		let commands = [];
		for (let c in g_AllyEntityCommands)
		{
			let info = g_AllyEntityCommands[c].getInfo(unitEntState);
			if (!info)
				continue;
			info.name = c;
			commands.push(info);
		}
		return commands;
	},
	"setupButton": function(data)
	{
		data.button.tooltip = data.item.tooltip;

		data.button.onPress = function() {
			if (data.item.callback)
				data.item.callback(data.item);
			else
				performAllyCommand(data.unitEntState.id, data.item.name);
		};

		data.countDisplay.caption = data.item.count || "";

		data.button.enabled = !!data.item.count;

		let grayscale = data.button.enabled ? "" : "grayscale:";
		data.icon.sprite = "stretched:" + grayscale + "session/icons/" + data.item.icon;

		let size = data.button.size;
		// count on square buttons, so size.bottom is the width too
		let spacer = size.bottom + 1;
		// relative to the center ( = 50%)
		size.rleft = size.rright = 50;
		// offset from the center calculation
		size.left = (data.i - data.numberOfItems/2) * spacer;
		size.right = size.left + size.bottom;
		data.button.size = size;

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Construction = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function()
	{
		return getAllBuildableEntitiesFromSelection();
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item);
		if (!template)
			return false;

		let technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
			"tech": template.requiredTechnology,
			"player": data.unitEntState.player
		});

		let neededResources;
		if (template.cost)
			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, 1),
				"player": data.unitEntState.player
			});

		let limits = getEntityLimitAndCount(data.playerState, data.item);

		if (template.wallSet)
			template.auras = GetTemplateData(template.wallSet.templates.long).auras;

		data.button.onPress = function () { startBuildingPlacement(data.item, data.playerState); };

		let tooltip = getEntityNamesFormatted(template);
		tooltip += getVisibleEntityClassesFormatted(template);
		tooltip += getAurasTooltip(template);

		if (template.tooltip)
			tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

		tooltip += "\n" + getEntityCostTooltip(template);
		tooltip += getPopulationBonusTooltip(template);

		tooltip += formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers);

		if (!technologyEnabled)
			tooltip += "\n" + sprintf(translate("Requires %(technology)s"), {
				"technology": getEntityNames(GetTechnologyData(template.requiredTechnology))
			});

		if (neededResources)
			tooltip += getNeededResourcesTooltip(neededResources);

		data.button.tooltip = tooltip;

		let modifier = "";
		if (!technologyEnabled || limits.canBeAddedCount == 0)
		{
			data.button.enabled = false;
			modifier += "color:0 0 0 127:grayscale:";
		}
		else if (neededResources)
		{
			data.button.enabled = false;
			modifier += resourcesToAlphaMask(neededResources) +":";
		}
		else
			data.button.enabled = controlsPlayer(data.unitEntState.player);

		if (template.icon)
			data.icon.sprite = modifier + "stretched:session/portraits/" + template.icon;

		let index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Formation = {
	"getMaxNumberOfItems": function()
	{
		return 16;
	},
	"rowLength": 4,
	"conflictsWith": ["Garrison"],
	"getItems": function(unitEntState)
	{
		if (!hasClass(unitEntState, "Unit") || hasClass(unitEntState, "Animal"))
			return [];
		if (!g_AvailableFormations.has(unitEntState.player))
			g_AvailableFormations.set(unitEntState.player, Engine.GuiInterfaceCall("GetAvailableFormations", unitEntState.player));
		return g_AvailableFormations.get(unitEntState.player);
	},
	"setupButton": function(data)
	{
		if (!g_FormationsInfo.has(data.item))
			g_FormationsInfo.set(data.item, Engine.GuiInterfaceCall("GetFormationInfoFromTemplate", { "templateName": data.item }));

		let formationInfo = g_FormationsInfo.get(data.item);
		let formationOk = canMoveSelectionIntoFormation(data.item);
		let formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
			"ents": data.selection,
			"formationTemplate": data.item
		});

		data.button.onPress = function() { performFormation(data.unitEntState.id, data.item); };

		let tooltip = translate(formationInfo.name);
		if (!formationOk && formationInfo.tooltip)
			tooltip += "\n" + "[color=\"red\"]" + translate(formationInfo.tooltip) + "[/color]";
		data.button.tooltip = tooltip;

		data.button.enabled = formationOk && controlsPlayer(data.unitEntState.player);
		let grayscale = formationOk ? "" : "grayscale:";
		data.guiSelection.hidden = !formationSelected;
		data.icon.sprite = "stretched:" + grayscale + "session/icons/" + formationInfo.icon;

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

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

		let groups = new EntityGroups();

		for (let ent of selection)
		{
			let state = GetEntityState(ent);
			if (state.garrisonHolder)
				groups.add(state.garrisonHolder.entities);
		}

		return groups.getEntsGrouped();
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item.template);
		if (!template)
			return false;

		data.button.onPress = function() {
			unloadTemplate(data.item.template);
		};

		data.countDisplay.caption = data.item.ents.length || "";

		let garrisonedUnitOwner = GetEntityState(data.item.ents[0]).player;

		let canUngarrison =
			g_ViewedPlayer == data.unitEntState.player ||
			g_ViewedPlayer == garrisonedUnitOwner;

		data.button.enabled = canUngarrison && controlsPlayer(g_ViewedPlayer);

		let tooltip = canUngarrison || g_IsObserver ?
			sprintf(translate("Unload %(name)s"),
			{ "name": getEntityNames(template) }) + "\n" +
			translate("Single-click to unload 1. Shift-click to unload all of this type.") :
			getEntityNames(template);

		tooltip += "\n" + sprintf(translate("Player: %(playername)s"), {
			"playername": g_Players[garrisonedUnitOwner].name
		});

		data.button.tooltip = tooltip;

		data.button.sprite = "color:" + rgbToGuiColor(g_Players[garrisonedUnitOwner].color) + ":";
		data.button.sprite_disabled = data.button.sprite;

		// Selection panel buttons only appear disabled if they
		// also appear disabled to the owner of the building.
		data.icon.sprite =
			(canUngarrison || g_IsObserver ? "" : "grayscale:") +
			"stretched:session/portraits/" + template.icon;

		setPanelObjectPosition(data.button, data.i, data.rowLength);

		return true;
	}
};

g_SelectionPanels.Gate = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function(unitEntState, selection)
	{
		// Allow long wall pieces to be converted to gates
		let longWallTypes = {};
		let walls = [];
		let gates = [];
		for (let ent of selection)
		{
			let state = GetEntityState(ent);
			if (hasClass(state, "LongWall") && !state.gate && !longWallTypes[state.template])
			{
				let gateTemplate = getWallGateTemplate(state.id);
				if (gateTemplate)
				{
					let tooltipString = GetTemplateDataWithoutLocalization(state.template).gateConversionTooltip;
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
				for (let j = 0; j < gates.length; ++j)
					delete gates[j].gate.locked;
		}

		// Place wall conversion options after gate lock/unlock icons.
		return gates.concat(walls);
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() {data.item.callback(data.item); };

		let tooltip = data.item.tooltip;
		if (data.item.template)
		{
			data.template = GetTemplateData(data.item.template);
			data.wallCount = data.selection.reduce(function (count, ent) {
					let state = GetEntityState(ent);
					if (hasClass(state, "LongWall") && !state.gate)
						++count;
					return count;
				}, 0);

			tooltip += "\n" + getEntityCostTooltip(data.template, data.wallCount);

			data.neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(data.template, data.wallCount)
			});

			if (data.neededResources)
				tooltip += getNeededResourcesTooltip(data.neededResources);
		}
		data.button.tooltip = tooltip;

		data.button.enabled = controlsPlayer(data.unitEntState.player);
		let gateIcon;
		if (data.item.gate)
		{
			// If already a gate, show locking actions
			gateIcon = "icons/lock_" + GATE_ACTIONS[data.item.locked ? 0 : 1] + "ed.png";
			if (data.item.gate.locked === undefined)
				data.guiSelection.hidden = false;
			else
				data.guiSelection.hidden = data.item.gate.locked != data.item.locked;
		}
		else
		{
			// Otherwise show gate upgrade icon
			let template = GetTemplateData(data.item.template);
			if (!template)
				return false;
			gateIcon = data.template.icon ? "portraits/" + data.template.icon : "icons/gate_closed.png";
			data.guiSelection.hidden = true;
		}

		data.icon.sprite = (data.neededResources ? resourcesToAlphaMask(data.neededResources) + ":" : "") + "stretched:session/" + gateIcon;

		let index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Pack = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function(unitEntState, selection)
	{
		let checks = {};
		for (let ent of selection)
		{
			let state = GetEntityState(ent);
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
		let items = [];
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
	"setupButton": function(data)
	{
		data.button.onPress = function() {data.item.callback(data.item); };

		data.button.tooltip = data.item.tooltip;

		if (data.item.packing)
			data.icon.sprite = "stretched:session/icons/cancel.png";
		else if (data.item.packed)
			data.icon.sprite = "stretched:session/icons/unpack.png";
		else
			data.icon.sprite = "stretched:session/icons/pack.png";

		data.button.enabled = controlsPlayer(data.unitEntState.player);

		let index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);
		return true;
	}
};

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
		let numRows = Math.ceil(numberOfItems / rowLength);
		let panel = Engine.GetGUIObjectByName("unitQueuePanel");
		let size = panel.size;
		let buttonSize = Engine.GetGUIObjectByName("unitQueueButton[0]").size.bottom;
		let margin = 4;
		size.top = size.bottom - numRows*buttonSize - (numRows+2)*margin;
		panel.size = size;
	},
	"setupButton": function(data)
	{
		// Differentiate between units and techs
		let template;
		if (data.item.unitTemplate)
			template = GetTemplateData(data.item.unitTemplate);
		else if (data.item.technologyTemplate)
			template = GetTechnologyData(data.item.technologyTemplate);

		if (!template)
			return false;

		data.button.onPress = function() { removeFromProductionQueue(data.item.producingEnt, data.item.id); };

		let tooltip = getEntityNames(template);
		if (data.item.neededSlots)
		{
			tooltip += "\n[color=\"red\"]" + translate("Insufficient population capacity:") + "\n[/color]";
			tooltip += sprintf(translate("%(population)s %(neededSlots)s"), {
				"population": getCostComponentDisplayIcon("population"),
				"neededSlots": data.item.neededSlots
			});
		}
		data.button.tooltip = tooltip;

		data.countDisplay.caption = data.item.count > 1 ? data.item.count : "";

		// Show the progress number for the first item
		if (data.i == 0)
			Engine.GetGUIObjectByName("queueProgress").caption = Math.round(data.item.progress*100) + "%";

		let guiObject = Engine.GetGUIObjectByName("unitQueueProgressSlider["+data.i+"]");
		let size = guiObject.size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(data.item.progress * (size.right - size.left));
		guiObject.size = size;

		if (template.icon)
			data.icon.sprite = "stretched:session/portraits/" + template.icon;

		data.button.enabled = controlsPlayer(data.unitEntState.player);

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Research = {
	"getMaxNumberOfItems": function()
	{
		return 8;
	},
	"getItems": function(unitEntState, selection)
	{
		// Tech-pairs require two rows. Thus only show techs when there is only one row in use.
		// TODO Use a reference instead of a magic number
		if (getNumberOfRightPanelButtons() > 8 && selection.length > 1)
			return [];

		for (let ent of selection)
		{
			let entState = GetEntityState(ent);
			if (entState.production && entState.production.technologies.length)
				return entState.production.technologies.map(tech => ({
					"tech": tech,
					"techCostMultiplier": entState.production.techCostMultiplier
				}));
		}
		return [];
	},
	"hideItem": function(i, rowLength) // Called when no item is found
	{
		Engine.GetGUIObjectByName("unitResearchButton[" + i + "]").hidden = true;
		// We also remove the paired tech and the pair symbol
		Engine.GetGUIObjectByName("unitResearchButton[" + (i + rowLength) + "]").hidden = true;
		Engine.GetGUIObjectByName("unitResearchPair[" + i + "]").hidden = true;
	},
	"setupButton": function(data)
	{
		if (!data.item.tech)
		{
			g_SelectionPanels.Research.hideItem(data.i, data.rowLength);
			return false;
		}
		let techs = data.item.tech.pair ? [data.item.tech.bottom, data.item.tech.top] : [data.item.tech];

		// Start position (start at the bottom)
		let position = data.i + data.rowLength;

		// Only show the top button for pairs
		if (!data.item.tech.pair)
			Engine.GetGUIObjectByName("unitResearchButton[" + data.i + "]").hidden = true;

		// Set up the tech connector
		let pair = Engine.GetGUIObjectByName("unitResearchPair[" + data.i + "]");
		pair.hidden = data.item.tech.pair == null;
		setPanelObjectPosition(pair, data.i, data.rowLength);

		// Handle one or two techs
		for (let i in techs)
		{
			let tech = techs[i];
			let template = GetTechnologyData(tech);
			if (!template)
				return false;

			for (let res in template.cost)
				template.cost[res] *= data.item.techCostMultiplier[res];

			let neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": template.cost,
				"player": data.unitEntState.player
			});

			let requirementsPassed = Engine.GuiInterfaceCall("CheckTechnologyRequirements", {
				"tech": tech,
				"player": data.unitEntState.player
			});

			let button = Engine.GetGUIObjectByName("unitResearchButton[" + position + "]");
			let icon = Engine.GetGUIObjectByName("unitResearchIcon[" + position + "]");

			let tooltip = getEntityNamesFormatted(template);
			if (template.tooltip)
				tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

			tooltip += "\n" + getEntityCostTooltip(template);
			if (!requirementsPassed)
			{
				tooltip += "\n" + template.requirementsTooltip;
				if (template.classRequirements)
				{
					let player = data.unitEntState.player;
					let current = GetSimState().players[player].classCounts[template.classRequirements.class] || 0;
					let remaining = template.classRequirements.number - current;
					tooltip += " " + sprintf(translatePlural("Remaining: %(number)s to build.", "Remaining: %(number)s to build.", remaining), { "number": remaining });
				}
			}
			if (neededResources)
				tooltip += getNeededResourcesTooltip(neededResources);
			button.tooltip = tooltip;

			button.onPress = function () {
				addResearchToQueue(data.unitEntState.id, tech);
			};

			if (data.item.tech.pair)
			{
				// On mouse enter, show a cross over the other icon
				let otherPosition = (position + data.rowLength) % (2 * data.rowLength);
				let unchosenIcon = Engine.GetGUIObjectByName("unitResearchUnchosenIcon[" + otherPosition + "]");
				button.onMouseEnter = function() {
						unchosenIcon.hidden = false;
				};
				button.onMouseLeave = function() {
						unchosenIcon.hidden = true;
				};
			}

			button.hidden = false;
			let modifier = "";
			if (!requirementsPassed)
			{
				button.enabled = false;
				modifier += "color:0 0 0 127:grayscale:";
			}
			else if (neededResources)
			{
				button.enabled = false;
				modifier += resourcesToAlphaMask(neededResources) + ":";
			}
			else
				button.enabled = controlsPlayer(data.unitEntState.player);

			if (template.icon)
				icon.sprite = modifier + "stretched:session/portraits/" + template.icon;

			setPanelObjectPosition(button, position, data.rowLength);

			// Prepare to handle the top button (if any)
			position -= data.rowLength;
		}

		return true;
	}
};

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
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item);
		if (!template)
			return false;

		let ents = g_Selection.groups.getEntsByName(data.item);
		for (let ent of ents)
		{
			let state = GetEntityState(ent);

			if (state.resourceCarrying && state.resourceCarrying.length !== 0)
			{
				if (!data.carried)
					data.carried = {};
				let carrying = state.resourceCarrying[0];
				if (data.carried[carrying.type])
					data.carried[carrying.type] += carrying.amount;
				else
					data.carried[carrying.type] = carrying.amount;
			}

			if (state.trader && state.trader.goods && state.trader.goods.amount)
			{
				if (!data.carried)
					data.carried = {};
				let amount = state.trader.goods.amount;
				let type = state.trader.goods.type;
				let totalGain = amount.traderGain;
				if (amount.market1Gain)
					totalGain += amount.market1Gain;
				if (amount.market2Gain)
					totalGain += amount.market2Gain;
				if (data.carried[type])
					data.carried[type] += totalGain;
				else
					data.carried[type] = totalGain;
			}
		}

		let tooltip = getEntityNames(template);
		if (data.carried)
			tooltip += "\n" + Object.keys(data.carried).map(res =>
				getCostComponentDisplayIcon(res) + data.carried[res]
			).join(" ");
		data.button.tooltip = tooltip;

		data.countDisplay.caption = ents.length || "";

		data.button.onPress = function() { changePrimarySelectionGroup(data.item, false); };
		data.button.onPressRight = function() { changePrimarySelectionGroup(data.item, true); };

		if (template.icon)
			data.icon.sprite = "stretched:session/portraits/" + template.icon;

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

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
	"setupButton": function(data)
	{
		data.button.onPress = function() { performStance(data.unitEntState, data.item); };

		data.button.tooltip = getStanceDisplayName(data.item) + "\n" +
			"[font=\"sans-13\"]" + getStanceTooltip(data.item) + "[/font]";

		data.guiSelection.hidden = !Engine.GuiInterfaceCall("IsStanceSelected", {
			"ents": data.selection,
			"stance": data.item
		});
		data.icon.sprite = "stretched:session/icons/stances/" + data.item + ".png";
		data.button.enabled = controlsPlayer(data.unitEntState.player);

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Training = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function()
	{
		return getAllTrainableEntitiesFromSelection();
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item);
		if (!template)
			return false;

		let technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
			"tech": template.requiredTechnology,
			"player": data.unitEntState.player
		});

		let [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch] =
			getTrainingBatchStatus(data.playerState, data.unitEntState.id, data.item, data.selection);

		let trainNum = buildingsCountToTrainFullBatch || 1;
		if (Engine.HotkeyIsPressed("session.batchtrain"))
			trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

		let neededResources;
		if (template.cost)
			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, trainNum),
				"player": data.unitEntState.player
			});

		data.button.onPress = function() { addTrainingToQueue(data.selection, data.item, data.playerState); };

		data.countDisplay.caption = trainNum > 1 ? trainNum : "";

		let tooltip = "[font=\"sans-bold-16\"]" +
		colorizeHotkey("%(hotkey)s", "session.queueunit." + (data.i + 1)) +
			"[/font]"; 

		tooltip += getEntityNamesFormatted(template);
		tooltip += getVisibleEntityClassesFormatted(template);
		tooltip += getAurasTooltip(template);

		if (template.tooltip)
			tooltip += "\n[font=\"sans-13\"]" + template.tooltip + "[/font]";

		tooltip += "\n" + getEntityCostTooltip(template, trainNum, data.unitEntState.id);

		let limits = getEntityLimitAndCount(data.playerState, data.item);

		tooltip += formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers);
		if (Engine.ConfigDB_GetValue("user", "showdetailedtooltips") === "true")
		{
			if (template.health)
				tooltip += "\n[font=\"sans-bold-13\"]" + translate("Health:") + "[/font] " + template.health;
			if (template.attack)
				tooltip += "\n" + getAttackTooltip(template);
			if (template.armour)
				tooltip += "\n" + getArmorTooltip(template.armour);
			if (template.speed)
				tooltip += "\n" + getSpeedTooltip(template);
		}

		tooltip += "[color=\"" + g_HotkeyColor + "\"]" +
			formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch) +
			"[/color]";

		if (!technologyEnabled)
		{
			let techName = getEntityNames(GetTechnologyData(template.requiredTechnology));
			tooltip += "\n" + sprintf(translate("Requires %(technology)s"), { "technology": techName });
		}
		if (neededResources)
			tooltip += getNeededResourcesTooltip(neededResources);

		data.button.tooltip = tooltip;

		let modifier = "";
		if (!technologyEnabled || limits.canBeAddedCount == 0)
		{
			data.button.enabled = false;
			modifier = "color:0 0 0 127:grayscale:";
		}
		else if (neededResources)
		{
			data.button.enabled = false;
			modifier = resourcesToAlphaMask(neededResources) +":";
		}
		else
			data.button.enabled = controlsPlayer(data.unitEntState.player);

		if (template.icon)
		data.icon.sprite = modifier + "stretched:session/portraits/" + template.icon;

		let index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);

		return true;
	}
};

/**
 * If two panels need the same space, so they collide,
 * the one appearing first in the order is rendered.
 *
 * Note that the panel needs to appear in the list to get rendered.
 */
let g_PanelsOrder = [
	// LEFT PANE
	"Barter", // Must always be visible on markets
	"Garrison", // More important than Formation, as you want to see the garrisoned units in ships
	"Alert",
	"Formation",
	"Stance", // Normal together with formation

	// RIGHT PANE
	"Gate", // Must always be shown on gates
	"Pack", // Must always be shown on packable entities
	"Training",
	"Construction",
	"Research", // Normal together with training

	// UNIQUE PANES (importance doesn't matter)
	"Command",
	"AllyCommand",
	"Queue",
	"Selection",
];

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
 *   "playerState":    playerState
 *   "unitEntStates":  states of the selected entities
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

let g_BarterSell;

function getPlayerHighlightColor(player)
{
	return "color:" + rgbToGuiColor(g_Players[player].color) + " 160";
}

g_SelectionPanels.Alert = {
	"getMaxNumberOfItems": function()
	{
		return 3;
	},
	"conflictsWith": ["Barter"],
	"getItems": function(unitEntStates)
	{
		let ret = [];

		if (unitEntStates.some(state => state.alertRaiser && !state.alertRaiser.hasRaisedAlert))
			ret.push("raise");

		if (unitEntStates.some(state => state.alertRaiser && state.alertRaiser.hasRaisedAlert && state.alertRaiser.canIncreaseLevel))
			ret.push("increase");

		if (unitEntStates.some(state => state.alertRaiser && state.alertRaiser.hasRaisedAlert))
			ret.push("end");

		return ret;
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() {
			switch (data.item)
			{
			case "raise":
				raiseAlert();
				return;
			case "increase":
				increaseAlertLevel();
				return;
			case "end":
				endOfAlert();
				return;
			}
		};

		switch (data.item)
		{
		case "raise":
			data.icon.sprite = "stretched:session/icons/bell_level1.png";
			data.button.tooltip = translate("Raise an alert!");
			break;
		case "increase":
			data.icon.sprite = "stretched:session/icons/bell_level2.png";
			data.button.tooltip = translate("Increase the alert level to protect more units");
			break;
		case "end":
			data.button.tooltip = translate("End of alert.");
			data.icon.sprite = "stretched:session/icons/bell_level0.png";
			break;
		}
		data.button.enabled = controlsPlayer(data.player);

		setPanelObjectPosition(data.button, this.getMaxNumberOfItems() - data.i - 1, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Barter = {
	"getMaxNumberOfItems": function()
	{
		return 4;
	},
	"rowLength": 4,
	"conflictsWith": ["Alert", "Garrison"],
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.every(state => !state.barterMarket))
			return [];
		return g_ResourceData.GetCodes();
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

		if (!g_BarterSell)
			g_BarterSell = g_ResourceData.GetCodes()[0];

		amount.Sell.caption = "-" + amountToSell;
		let prices;
		for (let state of data.unitEntStates)
			if (state.barterMarket)
			{
				prices = state.barterMarket.prices;
				break;
			}

		amount.Buy.caption = "+" + Math.round(prices.sell[g_BarterSell] / prices.buy[data.item] * amountToSell);

		let resource = getLocalizedResourceName(g_ResourceData.GetNames()[data.item], "withinSentence");
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
			"player": data.player
		}) ? "color:255 0 0 80:" : "";

		// Let's see if we have enough resources to barter.
		neededRes = {};
		neededRes[g_BarterSell] = amountToSell;
		let canBuyAny = Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": neededRes,
			"player": data.player
		}) ? "color:255 0 0 80:" : "";

		icon.Sell.sprite = canSellCurrent + "stretched:" + grayscale + "session/icons/resources/" + data.item + ".png";
		icon.Buy.sprite = canBuyAny + "stretched:" + grayscale + "session/icons/resources/" + data.item + ".png";

		button.Buy.hidden = isSelected;
		button.Buy.enabled = controlsPlayer(data.player);
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
	"getItems": function(unitEntStates)
	{
		let commands = [];

		for (let command in g_EntityCommands)
		{
			for (let state of unitEntStates)
			{
				let info = g_EntityCommands[command].getInfo(state);
				if (info)
				{
					info.name = command;
					commands.push(info);
					break;
				}
			}
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
				performCommand(data.unitEntStates[0], data.item.name);
		};

		data.countDisplay.caption = data.item.count || "";

		data.button.enabled =
			g_IsObserver && data.item.name == "focus-rally" ||
			controlsPlayer(data.player) && (data.item.name != "delete" ||
				data.unitEntStates.some(state => !isUndeletable(state)));

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
	"getItems": function(unitEntStates)
	{
		let commands = [];
		for (let command in g_AllyEntityCommands)
		{
			for (let state of unitEntStates)
			{
				let info = g_AllyEntityCommands[command].getInfo(state);
				if (info)
				{
					info.name = command;
					commands.push(info);
					break;
				}
			}
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
				performAllyCommand(data.unitEntStates[0].id, data.item.name);
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
			"player": data.player
		});

		let neededResources;
		if (template.cost)
			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, 1),
				"player": data.player
			});

		if (template.wallSet)
			template.auras = GetTemplateData(template.wallSet.templates.long).auras;

		data.button.onPress = function () { startBuildingPlacement(data.item, data.playerState); };

		let tooltips = [
			getEntityNamesFormatted,
			getVisibleEntityClassesFormatted,
			getAurasTooltip,
			getEntityTooltip,
			getEntityCostTooltip,
			getGarrisonTooltip,
			getPopulationBonusTooltip
		].map(func => func(template));

		let limits = getEntityLimitAndCount(data.playerState, data.item);
		tooltips.push(
			formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers),
			getRequiredTechnologyTooltip(technologyEnabled, template.requiredTechnology, GetSimState().players[data.player].civ),
			getNeededResourcesTooltip(neededResources));

		data.button.tooltip = tooltips.filter(tip => tip).join("\n");

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
			data.button.enabled = controlsPlayer(data.player);

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
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.some(state => !hasClass(state, "Unit") || hasClass(state, "Animal")))
			return [];
		if (!g_AvailableFormations.has(unitEntStates[0].player))
			g_AvailableFormations.set(unitEntStates[0].player, Engine.GuiInterfaceCall("GetAvailableFormations", unitEntStates[0].player));
		return g_AvailableFormations.get(unitEntStates[0].player);
	},
	"setupButton": function(data)
	{
		if (!g_FormationsInfo.has(data.item))
			g_FormationsInfo.set(data.item, Engine.GuiInterfaceCall("GetFormationInfoFromTemplate", { "templateName": data.item }));

		let formationInfo = g_FormationsInfo.get(data.item);
		let formationOk = canMoveSelectionIntoFormation(data.item);
		let formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
			"ents": data.unitEntStates.map(state => state.id),
			"formationTemplate": data.item
		});

		data.button.onPress = function() {
			performFormation(data.unitEntStates.map(state => state.id), data.item);
		};

		let tooltip = translate(formationInfo.name);
		if (!formationOk && formationInfo.tooltip)
			tooltip += "\n" + "[color=\"red\"]" + translate(formationInfo.tooltip) + "[/color]";
		data.button.tooltip = tooltip;

		data.button.enabled = formationOk && controlsPlayer(data.player);
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
	"conflictsWith": ["Barter"],
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.every(state => !state.garrisonHolder))
			return [];

		let groups = new EntityGroups();

		for (let state of unitEntStates)
			if (state.garrisonHolder)
				groups.add(state.garrisonHolder.entities);

		return groups.getEntsGrouped();
	},
	"setupButton": function(data)
	{
		let entState = GetEntityState(data.item.ents[0]);

		let template = GetTemplateData(entState.template);
		if (!template)
			return false;

		data.button.onPress = function() {
			unloadTemplate(template.selectionGroupName || entState.template, entState.player);
		};

		data.countDisplay.caption = data.item.ents.length || "";

		let garrisonedUnitOwner = entState.player;

		let canUngarrison =
			g_ViewedPlayer == data.player ||
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

		data.guiSelection.sprite = getPlayerHighlightColor(garrisonedUnitOwner);
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
	"getItems": function(unitEntStates)
	{
		let gates = [];
		for (let state of unitEntStates)
		{
			if (state.gate && !gates.length)
			{
				gates.push({
					"gate": state.gate,
					"tooltip": translate("Lock Gate"),
					"locked": true,
					"callback": function (item) { lockGate(item.locked); }
				},
				{
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

		return gates;
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() {data.item.callback(data.item); };

		data.button.tooltip = data.item.tooltip;

		data.button.enabled = controlsPlayer(data.player);
		let gateIcon;
		if (data.item.gate)
		{
			// show locking actions
			gateIcon = "icons/lock_" + GATE_ACTIONS[data.item.locked ? 0 : 1] + "ed.png";
			if (data.item.gate.locked === undefined)
				data.guiSelection.hidden = false;
			else
				data.guiSelection.hidden = data.item.gate.locked != data.item.locked;
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
	"getItems": function(unitEntStates)
	{
		let checks = {};
		for (let state of unitEntStates)
		{
			if (!state.pack)
				continue;

			if (state.pack.progress == 0)
			{
				if (state.pack.packed)
					checks.unpackButton = true;
				else
					checks.packButton = true;
			}
			else
			{
				if (state.pack.packed)
					checks.unpackCancelButton = true;
				else
					checks.packCancelButton = true;
			}
		}

		let items = [];
		if (checks.packButton)
			items.push({
				"packing": false,
				"packed": false,
				"tooltip": translate("Pack"),
				"callback": function() { packUnit(true); }
			});

		if (checks.unpackButton)
			items.push({
				"packing": false,
				"packed": true,
				"tooltip": translate("Unpack"),
				"callback": function() { packUnit(false); }
			});

		if (checks.packCancelButton)
			items.push({
				"packing": true,
				"packed": false,
				"tooltip": translate("Cancel Packing"),
				"callback": function() { cancelPackUnit(true); }
			});

		if (checks.unpackCancelButton)
			items.push({
				"packing": true,
				"packed": true,
				"tooltip": translate("Cancel Unpacking"),
				"callback": function() { cancelPackUnit(false); }
			});

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

		data.button.enabled = controlsPlayer(data.player);

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
	/**
	 * Returns a list of all items in the productionqueue of the selection
	 * The first entry of every entity's production queue will come before
	 * the second entry of every entity's production queue
	 */
	"getItems": function(unitEntStates)
	{
		let queue = [];
		let foundNew = true;
		for (let i = 0; foundNew; ++i)
		{
			foundNew = false;
			for (let state of unitEntStates)
			{
				if (!state.production || !state.production.queue[i])
					continue;
				let item = state.production.queue[i];
				item.producingEnt = state.id;
				queue.push(item);
				foundNew = true;
			}
		}
		return queue;
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
				"population": resourceIcon("population"),
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

		data.button.enabled = controlsPlayer(data.player);

		setPanelObjectPosition(data.button, data.i, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Research = {
	"getMaxNumberOfItems": function()
	{
		return 8;
	},
	"getItems": function(unitEntStates)
	{
		let ret = [];
		if (unitEntStates.length == 1)
			return !unitEntStates[0].production || !unitEntStates[0].production.technologies ? ret :
				unitEntStates[0].production.technologies.map(tech => ({
					"tech": tech,
					"techCostMultiplier": unitEntStates[0].production.techCostMultiplier,
					"researchFacilityId": unitEntStates[0].id
				}));

		for (let state of unitEntStates)
		{
			if (!state.production || !state.production.technologies)
				continue;
			// Remove the techs we already have in ret (with the same name and techCostMultiplier)
			let filteredTechs = state.production.technologies.filter(
				tech => tech != null && !ret.some(
					item =>
						(item.tech == tech ||
							item.tech.pair &&
							tech.pair &&
							item.tech.bottom == tech.bottom &&
							item.tech.top == tech.top) &&
						Object.keys(item.techCostMultiplier).every(
							k => item.techCostMultiplier[k] == state.production.techCostMultiplier[k])
			));

			if (filteredTechs.length + ret.length <= this.getMaxNumberOfItems() &&
			    getNumberOfRightPanelButtons() <= this.getMaxNumberOfItems() * (filteredTechs.some(tech => !!tech.pair) ? 1 : 2))
				ret = ret.concat(filteredTechs.map(tech => ({
					"tech": tech,
					"techCostMultiplier": state.production.techCostMultiplier,
					"researchFacilityId": state.id
				})));
		}
		return ret;
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

		// Handle one or two techs (tech pair)
		let player = data.player;
		for (let i in techs)
		{
			let tech = techs[i];
			let playerState = GetSimState().players[player];

			// Don't change the object returned by GetTechnologyData
			let template = clone(GetTechnologyData(tech, playerState.civ));
			if (!template)
				return false;

			for (let res in template.cost)
				template.cost[res] *= data.item.techCostMultiplier[res];

			let neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": template.cost,
				"player": player
			});

			let requirementsPassed = Engine.GuiInterfaceCall("CheckTechnologyRequirements", {
				"tech": tech,
				"player": player
			});

			let button = Engine.GetGUIObjectByName("unitResearchButton[" + position + "]");
			let icon = Engine.GetGUIObjectByName("unitResearchIcon[" + position + "]");

			let tooltips = [
				getEntityNamesFormatted,
				getEntityTooltip,
				getEntityCostTooltip
			].map(func => func(template));

			if (!requirementsPassed)
			{
				let tip = template.requirementsTooltip;
				let reqs = template.reqs;
				for (let req of reqs)
				{
					if (!req.entities)
						continue;

					let entityCounts = [];
					for (let entity of req.entities)
					{
						let current = 0;
						switch (entity.check)
						{
						case "count":
							current = playerState.classCounts[entity.class] || 0;
							break;

						case "variants":
							current = playerState.typeCountsByClass[entity.class] ?
								Object.keys(playerState.typeCountsByClass[entity.class]).length : 0;
							break;
						}

						let remaining = entity.number - current;
						if (remaining < 1)
							continue;

						entityCounts.push(sprintf(translatePlural("%(number)s entity of class %(class)s", "%(number)s entities of class %(class)s", remaining), {
							"number": remaining,
							"class": entity.class
						}));
					}

					tip += " " + sprintf(translate("Remaining: %(entityCounts)s"), {
						"entityCounts": entityCounts.join(translate(", "))
					});
				}
				tooltips.push(tip);
			}
			tooltips.push(getNeededResourcesTooltip(neededResources));
			button.tooltip = tooltips.filter(tip => tip).join("\n");

			button.onPress = function () {
				addResearchToQueue(data.item.researchFacilityId, tech);
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
				button.enabled = controlsPlayer(data.player);

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
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.length < 2)
			return [];
		return g_Selection.groups.getEntsGrouped();
	},
	"setupButton": function(data)
	{
		let entState = GetEntityState(data.item.ents[0]);
		let template = GetTemplateData(entState.template);
		if (!template)
			return false;

		for (let ent of data.item.ents)
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

		let unitOwner = GetEntityState(data.item.ents[0]).player;
		let tooltip = getEntityNames(template);
		if (data.carried)
			tooltip += "\n" + Object.keys(data.carried).map(res =>
				resourceIcon(res) + data.carried[res]
			).join(" ");
		if (g_IsObserver)
			tooltip += "\n" + sprintf(translate("Player: %(playername)s"), {
				"playername": g_Players[unitOwner].name
			});
		data.button.tooltip = tooltip;

		data.guiSelection.sprite = getPlayerHighlightColor(unitOwner);
		data.guiSelection.hidden = !g_IsObserver;

		data.countDisplay.caption = data.item.ents.length || "";

		data.button.onPress = function() { changePrimarySelectionGroup(data.item.key, false); };
		data.button.onPressRight = function() { changePrimarySelectionGroup(data.item.key, true); };

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
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.some(state => !state.unitAI || !hasClass(state, "Unit") || hasClass(state, "Animal")))
			return [];

		return unitEntStates[0].unitAI.possibleStances;
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() { performStance(data.unitEntStates.map(state => state.id), data.item); };

		data.button.tooltip = getStanceDisplayName(data.item) + "\n" +
			"[font=\"sans-13\"]" + getStanceTooltip(data.item) + "[/font]";

		data.guiSelection.hidden = !Engine.GuiInterfaceCall("IsStanceSelected", {
			"ents": data.unitEntStates.map(state => state.id),
			"stance": data.item
		});
		data.icon.sprite = "stretched:session/icons/stances/" + data.item + ".png";
		data.button.enabled = controlsPlayer(data.player);

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
			"player": data.player
		});

		let [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch] =
			getTrainingStatus(data.playerState, data.item, data.unitEntStates.map(status => status.id));

		let trainNum = buildingsCountToTrainFullBatch || 1;
		if (Engine.HotkeyIsPressed("session.batchtrain"))
			trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

		let neededResources;
		if (template.cost)
			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, trainNum),
				"player": data.player
			});

		data.button.onPress = function() {
			addTrainingToQueue(data.unitEntStates.map(state => state.id), data.item, data.playerState);
		};

		data.countDisplay.caption = trainNum > 1 ? trainNum : "";

		let tooltips = [
			"[font=\"sans-bold-16\"]" +
				colorizeHotkey("%(hotkey)s", "session.queueunit." + (data.i + 1)) +
				"[/font]" + " " + getEntityNamesFormatted(template),
			getVisibleEntityClassesFormatted(template),
			getAurasTooltip(template),
			getEntityTooltip(template),
			getEntityCostTooltip(template, trainNum, data.unitEntStates[0].id)
		];

		let limits = getEntityLimitAndCount(data.playerState, data.item);
		tooltips.push(formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers));

		if (Engine.ConfigDB_GetValue("user", "showdetailedtooltips") === "true")
			tooltips = tooltips.concat([
				getHealthTooltip,
				getAttackTooltip,
				getSplashDamageTooltip,
				getHealerTooltip,
				getArmorTooltip,
				getGarrisonTooltip,
				getProjectilesTooltip,
				getSpeedTooltip
			].map(func => func(template)));

		tooltips.push(
			"[color=\"" + g_HotkeyColor + "\"]" +
			formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch) +
			"[/color]",
			getRequiredTechnologyTooltip(technologyEnabled, template.requiredTechnology, GetSimState().players[data.player].civ),
			getNeededResourcesTooltip(neededResources));

		data.button.tooltip = tooltips.filter(tip => tip).join("\n");

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
			data.button.enabled = controlsPlayer(data.player);

		if (template.icon)
		data.icon.sprite = modifier + "stretched:session/portraits/" + template.icon;

		let index = data.i + getNumberOfRightPanelButtons();
		setPanelObjectPosition(data.button, index, data.rowLength);

		return true;
	}
};

g_SelectionPanels.Upgrade = {
	"getMaxNumberOfItems": function()
	{
		return 24 - getNumberOfRightPanelButtons();
	},
	"getItems": function(unitEntStates)
	{
		// Interface becomes complicated with multiple different units and this is meant per-entity, so prevent it if the selection has multiple different units.
		if (unitEntStates.some(state => state.template != unitEntStates[0].template))
			return false;

		return unitEntStates[0].upgrade && unitEntStates[0].upgrade.upgrades;
	},
	"setupButton" : function(data)
	{
		let template = GetTemplateData(data.item.entity);
		if (!template)
			return false;

		let technologyEnabled = true;

		if (data.item.requiredTechnology)
			technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
				"tech": data.item.requiredTechnology,
				"player": data.player
			});

		let neededResources;
		if (data.item.cost)
		{
			for (let cost in data.item.cost)
				if (cost != "time")
					data.item.cost[cost] *= data.unitEntStates.length;

			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": data.item.cost,
				"player": data.player
			});
		}

		let limits = getEntityLimitAndCount(data.playerState, data.item.entity);
		let progress = data.unitEntStates[0].upgrade.progress || 0;
		let isUpgrading = data.unitEntStates[0].upgrade.template == data.item.entity;

		let tooltip;
		if (!progress)
		{
			let tooltips = [];
			if (data.item.tooltip)
				tooltips.push(sprintf(translate("Upgrade into a %(name)s. %(tooltip)s"), {
					"name": template.name.generic,
					"tooltip": translate(data.item.tooltip)
				}));
			else
				tooltips.push(sprintf(translate("Upgrade into a %(name)s."), {
					"name": template.name.generic
				}));

			tooltips.push(
				getEntityCostTooltip(data.item),
				formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers),
				getRequiredTechnologyTooltip(technologyEnabled, data.item.requiredTechnology, GetSimState().players[data.player].civ),
				getNeededResourcesTooltip(neededResources));

			tooltip = tooltips.filter(tip => tip).join("\n");

			data.button.onPress = function() { upgradeEntity(data.item.entity); };
		}
		else if (isUpgrading)
		{
			tooltip = translate("Cancel Upgrading");
			data.button.onPress = function() { cancelUpgradeEntity(); };
		}
		else
		{
			tooltip = translate("Cannot upgrade when the entity is already upgrading.");
			data.button.onPress = function() {};
		}
		data.button.enabled = controlsPlayer(data.player);
		data.button.tooltip = tooltip;

		let modifier = "";
		if (!isUpgrading)
		{
			if (progress || !technologyEnabled || limits.canBeAddedCount == 0 &&
				!hasSameRestrictionCategory(data.item.entity, data.unitEntStates[0].template))
			{
				data.button.enabled = false;
				modifier = "color:0 0 0 127:grayscale:";
			}
			else if (neededResources)
			{
				data.button.enabled = false;
				modifier = resourcesToAlphaMask(neededResources) + ":";
			}
		}

		data.icon.sprite = modifier + "stretched:session/" +
			(data.item.icon || "portraits/" + template.icon);

		data.countDisplay.caption = data.unitEntStates.length > 1 ? data.unitEntStates.length : "";

		let progressOverlay = Engine.GetGUIObjectByName("unitUpgradeProgressSlider[" + data.i + "]");
		if (isUpgrading)
		{
			let size = progressOverlay.size;
			size.top = size.left + Math.round(progress * (size.right - size.left));
			progressOverlay.size = size;
		}
		progressOverlay.hidden = !isUpgrading;

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
	"Upgrade", // Must always be shown on upgradable entities
	"Training",
	"Construction",
	"Research", // Normal together with training

	// UNIQUE PANES (importance doesn't matter)
	"Command",
	"AllyCommand",
	"Queue",
	"Selection",
];

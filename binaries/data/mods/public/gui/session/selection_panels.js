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
var g_AvailableFormations = new Map();
var g_FormationsInfo = new Map();

var g_SelectionPanels = {};

var g_SelectionPanelBarterButtonManager;

g_SelectionPanels.Alert = {
	"getMaxNumberOfItems": function()
	{
		return 2;
	},
	"getItems": function(unitEntStates)
	{
		return unitEntStates.some(state => !!state.alertRaiser) ? ["raise", "end"] : [];
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() {
			switch (data.item)
			{
			case "raise":
				raiseAlert();
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
		case "end":
			data.button.tooltip = translate("End of alert.");
			data.icon.sprite = "stretched:session/icons/bell_level0.png";
			break;
		}
		data.button.enabled = controlsPlayer(data.player);

		setPanelObjectPosition(data.button, this.getMaxNumberOfItems() - data.i, data.rowLength);
		return true;
	}
};

g_SelectionPanels.Barter = {
	"getMaxNumberOfItems": function()
	{
		return 5;
	},
	"rowLength": 5,
	"conflictsWith": ["Garrison"],
	"getItems": function(unitEntStates)
	{
		// If more than `rowLength` resources, don't display icons.
		if (unitEntStates.every(state => !state.isBarterMarket) || g_ResourceData.GetBarterableCodes().length > this.rowLength)
			return [];
		return g_ResourceData.GetBarterableCodes();
	},
	"setupButton": function(data)
	{
		if (g_SelectionPanelBarterButtonManager)
		{
			g_SelectionPanelBarterButtonManager.setViewedPlayer(data.player);
			g_SelectionPanelBarterButtonManager.update();
		}
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
			let info = getCommandInfo(command, unitEntStates);
			if (info)
			{
				info.name = command;
				commands.push(info);
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
				performCommand(data.unitEntStates, data.item.name);
		};

		data.countDisplay.caption = data.item.count || "";

		data.button.enabled = data.item.enabled == true;

		data.icon.sprite = "stretched:session/icons/" + data.item.icon;

		let size = data.button.size;
		// relative to the center ( = 50%)
		size.rleft = 50;
		size.rright = 50;
		// offset from the center calculation, count on square buttons, so size.bottom is the width too
		size.left = (data.i - data.numberOfItems / 2) * (size.bottom + 1);
		size.right = size.left + size.bottom;
		data.button.size = size;
		return true;
	}
};

g_SelectionPanels.Construction = {
	"getMaxNumberOfItems": function()
	{
		return 40 - getNumberOfRightPanelButtons();
	},
	"rowLength": 10,
	"getItems": function()
	{
		return getAllBuildableEntitiesFromSelection();
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item, data.player);
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

		data.button.onPress = function() { startBuildingPlacement(data.item, data.playerState); };
		let showTemplateFunc = () => { showTemplateDetails(data.item); };
		data.button.onPressRight = showTemplateFunc;
		data.button.onPressRightDisabled = showTemplateFunc;

		let tooltips = [
			getEntityNamesFormatted,
			getVisibleEntityClassesFormatted,
			getAurasTooltip,
			getEntityTooltip
		].map(func => func(template));
		tooltips.push(
			getEntityCostTooltip(template, data.player),
			getResourceDropsiteTooltip(template),
			getGarrisonTooltip(template),
			getPopulationBonusTooltip(template),
			showTemplateViewerOnRightClickTooltip(template)
		);


		let limits = getEntityLimitAndCount(data.playerState, data.item);
		tooltips.push(
			formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers),
			formatMatchLimitString(limits.matchLimit, limits.matchCount, limits.type),
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
			modifier += resourcesToAlphaMask(neededResources) + ":";
		}
		else
			data.button.enabled = controlsPlayer(data.player);

		if (template.icon)
			data.icon.sprite = modifier + "stretched:session/portraits/" + template.icon;

		setPanelObjectPosition(data.button, data.i + getNumberOfRightPanelButtons(), data.rowLength);
		return true;
	}
};

g_SelectionPanels.Formation = {
	"getMaxNumberOfItems": function()
	{
		return 15;
	},
	"rowLength": 5,
	"conflictsWith": ["Garrison"],
	"getItems": function(unitEntStates)
	{
		if (unitEntStates.some(state => !hasClass(state, "Unit")))
			return [];

		if (unitEntStates.every(state => !state.identity || !state.identity.hasSomeFormation))
			return [];

		if (!g_AvailableFormations.has(unitEntStates[0].player))
			g_AvailableFormations.set(unitEntStates[0].player, Engine.GuiInterfaceCall("GetAvailableFormations", unitEntStates[0].player));

		return g_AvailableFormations.get(unitEntStates[0].player).filter(formation => unitEntStates.some(state => !!state.identity && state.identity.formations.indexOf(formation) != -1));
	},
	"setupButton": function(data)
	{
		if (!g_FormationsInfo.has(data.item))
			g_FormationsInfo.set(data.item, Engine.GuiInterfaceCall("GetFormationInfoFromTemplate", { "templateName": data.item }));

		let formationOk = canMoveSelectionIntoFormation(data.item);
		let unitIds = data.unitEntStates.map(state => state.id);
		let formationSelected = Engine.GuiInterfaceCall("IsFormationSelected", {
			"ents": unitIds,
			"formationTemplate": data.item
		});

		data.button.onPress = function() {
			performFormation(unitIds, data.item);
		};

		data.button.onMouseRightPress = () => g_AutoFormation.setDefault(data.item);

		let formationInfo = g_FormationsInfo.get(data.item);
		let tooltip = translate(formationInfo.name);

		let isDefaultFormation = g_AutoFormation.isDefault(data.item);
		if (data.item === NULL_FORMATION)
			tooltip += "\n" + (isDefaultFormation ?
				translate("Default formation is disabled.") :
				translate("Right-click to disable the default formation feature."));
		else
			tooltip += "\n" + (isDefaultFormation ?
				translate("This is the default formation, used for movement orders.") :
				translate("Right-click to set this as the default formation."));

		if (!formationOk && formationInfo.tooltip)
			tooltip += "\n" + coloredText(translate(formationInfo.tooltip), "red");
		data.button.tooltip = tooltip;

		data.button.enabled = formationOk && controlsPlayer(data.player);
		let grayscale = formationOk ? "" : "grayscale:";
		data.guiSelection.hidden = !formationSelected;
		data.countDisplay.hidden = !isDefaultFormation;
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

		let canUngarrison = controlsPlayer(data.player) || controlsPlayer(entState.player);

		data.button.enabled = canUngarrison;

		data.button.tooltip = (canUngarrison ?
			sprintf(translate("Unload %(name)s"), { "name": getEntityNames(template) }) + "\n" +
			translate("Single-click to unload 1. Shift-click to unload all of this type.") :
			getEntityNames(template)) + "\n" +
			sprintf(translate("Player: %(playername)s"), {
				"playername": g_Players[entState.player].name
			});

		data.guiSelection.sprite = "color:" + g_DiplomacyColors.getPlayerColor(entState.player, 160);
		data.button.sprite_disabled = data.button.sprite;

		// Selection panel buttons only appear disabled if they
		// also appear disabled to the owner of the structure.
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
		return 40 - getNumberOfRightPanelButtons();
	},
	"rowLength": 10,
	"getItems": function(unitEntStates)
	{
		let hideLocked = unitEntStates.every(state => !state.gate || !state.gate.locked);
		let hideUnlocked = unitEntStates.every(state => !state.gate || state.gate.locked);

		if (hideLocked && hideUnlocked)
			return [];

		return [
			{
				"hidden": hideLocked,
				"tooltip": translate("Lock Gate"),
				"icon": "session/icons/lock_locked.png",
				"locked": true
			},
			{
				"hidden": hideUnlocked,
				"tooltip": translate("Unlock Gate"),
				"icon": "session/icons/lock_unlocked.png",
				"locked": false
			}
		];
	},
	"setupButton": function(data)
	{
		data.button.onPress = function() { lockGate(data.item.locked); };
		data.button.tooltip = data.item.tooltip;
		data.button.enabled = controlsPlayer(data.player);
		data.guiSelection.hidden = data.item.hidden;
		data.icon.sprite = "stretched:" + data.item.icon;

		setPanelObjectPosition(data.button, data.i + getNumberOfRightPanelButtons(), data.rowLength);
		return true;
	}
};

g_SelectionPanels.Pack = {
	"getMaxNumberOfItems": function()
	{
		return 40 - getNumberOfRightPanelButtons();
	},
	"rowLength": 10,
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
			else if (state.pack.packed)
				checks.unpackCancelButton = true;
			else
				checks.packCancelButton = true;
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

		setPanelObjectPosition(data.button, data.i + getNumberOfRightPanelButtons(), data.rowLength);
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
				queue.push({
					"producingEnt": state.id,
					"queuedItem": state.production.queue[i]
				});
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
		size.top = size.bottom - numRows * buttonSize - (numRows + 2) * margin;
		panel.size = size;
	},
	"setupButton": function(data)
	{
		let queuedItem = data.item.queuedItem;

		// Differentiate between units and techs
		let template;
		if (queuedItem.unitTemplate)
			template = GetTemplateData(queuedItem.unitTemplate);
		else if (queuedItem.technologyTemplate)
			template = GetTechnologyData(queuedItem.technologyTemplate, GetSimState().players[data.player].civ);
		else
		{
			warning("Unknown production queue template " + uneval(queuedItem));
			return false;
		}

		data.button.onPress = function() { removeFromProductionQueue(data.item.producingEnt, queuedItem.id); };

		let tooltip = getEntityNames(template);
		if (queuedItem.neededSlots)
		{
			tooltip += "\n" + coloredText(translate("Insufficient population capacity:"), "red");
			tooltip += "\n" + sprintf(translate("%(population)s %(neededSlots)s"), {
				"population": resourceIcon("population"),
				"neededSlots": queuedItem.neededSlots
			});
		}
		data.button.tooltip = tooltip;

		data.countDisplay.caption = queuedItem.count > 1 ? queuedItem.count : "";

		// Show the time remaining to finish the first item
		if (data.i == 0)
			Engine.GetGUIObjectByName("queueTimeRemaining").caption =
				Engine.FormatMillisecondsIntoDateStringGMT(queuedItem.timeRemaining, translateWithContext("countdown format", "m:ss"));

		let guiObject = Engine.GetGUIObjectByName("unitQueueProgressSlider[" + data.i + "]");
		let size = guiObject.size;

		// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
		size.top = size.left + Math.round(queuedItem.progress * (size.right - size.left));
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
		return 10;
	},
	"rowLength": 10,
	"getItems": function(unitEntStates)
	{
		let ret = [];
		if (unitEntStates.length == 1)
			return !unitEntStates[0].production || !unitEntStates[0].production.technologies ? ret :
				unitEntStates[0].production.technologies.map(tech => ({
					"tech": tech,
					"techCostMultiplier": unitEntStates[0].production.techCostMultiplier,
					"researchFacilityId": unitEntStates[0].id,
					"isUpgrading": !!unitEntStates[0].upgrade && unitEntStates[0].upgrade.isUpgrading
				}));

		let sortedEntStates = unitEntStates.sort((a, b) =>
			(!b.upgrade || !b.upgrade.isUpgrading) - (!a.upgrade || !a.upgrade.isUpgrading));

		for (let state of sortedEntStates)
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
					"researchFacilityId": state.id,
					"isUpgrading": !!state.upgrade && state.upgrade.isUpgrading
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
		let playerState = GetSimState().players[player];
		for (let tech of data.item.tech.pair ? [data.item.tech.bottom, data.item.tech.top] : [data.item.tech])
		{
			// Don't change the object returned by GetTechnologyData
			let template = clone(GetTechnologyData(tech, playerState.civ));
			if (!template)
				return false;

			// Not allowed by civ.
			if (!template.reqs)
			{
				// One of the pair may still be researchable by the current civ,
				// hence don't hide everything.
				Engine.GetGUIObjectByName("unitResearchButton[" + data.i + "]").hidden = true;
				pair.hidden = true;
				continue;
			}

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
				getEntityCostTooltip,
				showTemplateViewerOnRightClickTooltip
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
						"entityCounts": entityCounts.join(translateWithContext("Separator for a list of entity counts", ", "))
					});
				}
				tooltips.push(tip);
			}
			tooltips.push(getNeededResourcesTooltip(neededResources));
			button.tooltip = tooltips.filter(tip => tip).join("\n");

			button.onPress = (t => function() {
				addResearchToQueue(data.item.researchFacilityId, t);
			})(tech);

			let showTemplateFunc =  (t => function() {
				showTemplateDetails(
					t,
					GetTemplateData(data.unitEntStates.find(state => state.id == data.item.researchFacilityId).template).nativeCiv);
			});

			button.onPressRight = showTemplateFunc(tech);
			button.onPressRightDisabled = showTemplateFunc(tech);

			if (data.item.tech.pair)
			{
				// On mouse enter, show a cross over the other icon
				let unchosenIcon = Engine.GetGUIObjectByName("unitResearchUnchosenIcon[" + (position + data.rowLength) % (2 * data.rowLength) + "]");
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

			if (data.item.isUpgrading)
			{
				button.enabled = false;
				modifier += "color:0 0 0 127:grayscale:";
				button.tooltip += "\n" + coloredText(translate("Cannot research while upgrading."), "red");

			}

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

		data.guiSelection.sprite = "color:" + g_DiplomacyColors.getPlayerColor(unitOwner, 160);
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

		return unitEntStates[0].unitAI.selectableStances;
	},
	"setupButton": function(data)
	{
		let unitIds = data.unitEntStates.map(state => state.id);
		data.button.onPress = function() { performStance(unitIds, data.item); };

		data.button.tooltip = getStanceDisplayName(data.item) + "\n" +
			"[font=\"sans-13\"]" + getStanceTooltip(data.item) + "[/font]";

		data.guiSelection.hidden = !Engine.GuiInterfaceCall("IsStanceSelected", {
			"ents": unitIds,
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
		return 40 - getNumberOfRightPanelButtons();
	},
	"rowLength": 10,
	"getItems": function()
	{
		return getAllTrainableEntitiesFromSelection();
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item, data.player);
		if (!template)
			return false;

		let technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
			"tech": template.requiredTechnology,
			"player": data.player
		});

		let unitIds = data.unitEntStates.map(status => status.id);
		let [buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch] =
			getTrainingStatus(unitIds, data.item, data.playerState);

		let trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

		let neededResources;
		if (template.cost)
			neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": multiplyEntityCosts(template, trainNum),
				"player": data.player
			});

		data.button.onPress = function() {
			if (!neededResources)
				addTrainingToQueue(unitIds, data.item, data.playerState);
		};

		let showTemplateFunc = () => { showTemplateDetails(data.item); };
		data.button.onPressRight = showTemplateFunc;
		data.button.onPressRightDisabled = showTemplateFunc;

		data.countDisplay.caption = trainNum > 1 ? trainNum : "";

		let tooltips = [
			"[font=\"sans-bold-16\"]" +
				colorizeHotkey("%(hotkey)s", "session.queueunit." + (data.i + 1)) +
				"[/font]" + " " + getEntityNamesFormatted(template),
			getVisibleEntityClassesFormatted(template),
			getAurasTooltip(template),
			getEntityTooltip(template),
			getEntityCostTooltip(template, data.player, unitIds[0], buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
		];
		let limits = getEntityLimitAndCount(data.playerState, data.item);
		tooltips.push(formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers),
			formatMatchLimitString(limits.matchLimit, limits.matchCount, limits.type));

		if (Engine.ConfigDB_GetValue("user", "showdetailedtooltips") === "true")
			tooltips = tooltips.concat([
				getHealthTooltip,
				getAttackTooltip,
				getHealerTooltip,
				getResistanceTooltip,
				getGarrisonTooltip,
				getProjectilesTooltip,
				getSpeedTooltip,
				getResourceDropsiteTooltip
			].map(func => func(template)));

		tooltips.push(showTemplateViewerOnRightClickTooltip());
		tooltips.push(
			formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch),
			getRequiredTechnologyTooltip(technologyEnabled, template.requiredTechnology, GetSimState().players[data.player].civ),
			getNeededResourcesTooltip(neededResources));

		data.button.tooltip = tooltips.filter(tip => tip).join("\n");

		let modifier = "";
		if (!technologyEnabled || limits.canBeAddedCount == 0)
		{
			data.button.enabled = false;
			modifier = "color:0 0 0 127:grayscale:";
		}
		else
		{
			data.button.enabled = controlsPlayer(data.player);
			if (neededResources)
				modifier = resourcesToAlphaMask(neededResources) + ":";
		}

		if (data.unitEntStates.every(state => state.upgrade && state.upgrade.isUpgrading))
		{
			data.button.enabled = false;
			modifier = "color:0 0 0 127:grayscale:";
			data.button.tooltip += "\n" + coloredText(translate("Cannot train while upgrading."), "red");
		}

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
		return 40 - getNumberOfRightPanelButtons();
	},
	"rowLength": 10,
	"getItems": function(unitEntStates)
	{
		// Interface becomes complicated with multiple different units and this is meant per-entity, so prevent it if the selection has multiple different units.
		if (unitEntStates.some(state => state.template != unitEntStates[0].template))
			return false;

		return unitEntStates[0].upgrade && unitEntStates[0].upgrade.upgrades;
	},
	"setupButton": function(data)
	{
		let template = GetTemplateData(data.item.entity);
		if (!template)
			return false;

		let progressOverlay = Engine.GetGUIObjectByName("unitUpgradeProgressSlider[" + data.i + "]");
		progressOverlay.hidden = true;

		let technologyEnabled = true;

		if (data.item.requiredTechnology)
			technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
				"tech": data.item.requiredTechnology,
				"player": data.player
			});

		let limits = getEntityLimitAndCount(data.playerState, data.item.entity);
		let upgradingEntStates = data.unitEntStates.filter(state => state.upgrade.template == data.item.entity);

		let upgradableEntStates = data.unitEntStates.filter(state =>
			!state.upgrade.progress &&
			(!state.production || !state.production.queue || !state.production.queue.length));

		let neededResources = data.item.cost && Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": multiplyEntityCosts(data.item, upgradableEntStates.length),
			"player": data.player
		});

		let tooltip;
		let modifier = "";
		if (!upgradingEntStates.length && upgradableEntStates.length)
		{
			let tooltips = [];
			if (data.item.tooltip)
				tooltips.push(sprintf(translate("Upgrade to %(name)s. %(tooltip)s"), {
					"name": template.name.generic,
					"tooltip": translate(data.item.tooltip)
				}));
			else
				tooltips.push(sprintf(translate("Upgrade to %(name)s."), {
					"name": template.name.generic
				}));

			tooltips.push(
				getEntityCostTooltip(data.item, undefined, undefined, data.unitEntStates.length),
				formatLimitString(limits.entLimit, limits.entCount, limits.entLimitChangers),
				formatMatchLimitString(limits.matchLimit, limits.matchCount, limits.type),
				getRequiredTechnologyTooltip(technologyEnabled, data.item.requiredTechnology, GetSimState().players[data.player].civ),
				getNeededResourcesTooltip(neededResources),
				showTemplateViewerOnRightClickTooltip());

			tooltip = tooltips.filter(tip => tip).join("\n");

			data.button.onPress = function() {
				upgradeEntity(
				    data.item.entity,
				    upgradableEntStates.map(state => state.id));
			};

			if (!technologyEnabled || limits.canBeAddedCount == 0 &&
				!upgradableEntStates.some(state => hasSameRestrictionCategory(data.item.entity, state.template)))
			{
				data.button.enabled = false;
				modifier = "color:0 0 0 127:grayscale:";
			}
			else if (neededResources)
			{
				data.button.enabled = false;
				modifier = resourcesToAlphaMask(neededResources) + ":";
			}

			data.countDisplay.caption = upgradableEntStates.length > 1 ? upgradableEntStates.length : "";
		}
		else if (upgradingEntStates.length)
		{
			tooltip = translate("Cancel Upgrading");
			data.button.onPress = function() { cancelUpgradeEntity(); };
			data.countDisplay.caption = upgradingEntStates.length > 1 ? upgradingEntStates.length : "";

			let progress = 0;
			for (let state of upgradingEntStates)
				progress = Math.max(progress, state.upgrade.progress || 1);
			let progressOverlaySize = progressOverlay.size;
			// TODO This is bad: we assume the progressOverlay is square
			progressOverlaySize.top = progressOverlaySize.bottom + Math.round((1 - progress) * (progressOverlaySize.left - progressOverlaySize.right));
			progressOverlay.size = progressOverlaySize;
			progressOverlay.hidden = false;
		}
		else
		{
			tooltip = coloredText(translatePlural(
				"Cannot upgrade when the entity is training, researching or already upgrading.",
				"Cannot upgrade when all entities are training, researching or already upgrading.",
				data.unitEntStates.length), "red");

			data.button.onPress = function() {};

			data.button.enabled = false;
			modifier = "color:0 0 0 127:grayscale:";
		}
		data.button.enabled = controlsPlayer(data.player);
		data.button.tooltip = tooltip;

		let showTemplateFunc = () => { showTemplateDetails(data.item.entity); };
		data.button.onPressRight = showTemplateFunc;
		data.button.onPressRightDisabled = showTemplateFunc;

		data.icon.sprite = modifier + "stretched:session/" +
			(data.item.icon || "portraits/" + template.icon);

		setPanelObjectPosition(data.button, data.i + getNumberOfRightPanelButtons(), data.rowLength);
		return true;
	}
};

function initSelectionPanels()
{

	let unitBarterPanel = Engine.GetGUIObjectByName("unitBarterPanel");
	if (BarterButtonManager.IsAvailable(unitBarterPanel))
		g_SelectionPanelBarterButtonManager = new BarterButtonManager(unitBarterPanel);
}

/**
 * Pauses game and opens the template details viewer for a selected entity or technology.
 *
 * Technologies don't have a set civ, so we pass along the native civ of
 * the template of the entity that's researching it.
 *
 * @param {string} [civCode] - The template name of the entity that researches the selected technology.
 */
function showTemplateDetails(templateName, civCode)
{
	g_PauseControl.implicitPause();

	Engine.PushGuiPage(
		"page_viewer.xml",
		{
			"templateName": templateName,
			"civ": civCode
		},
		resumeGame);
}

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
	"Queue",
	"Selection",
];

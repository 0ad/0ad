/**
 * Contains the layout and button settings per selection panel
 *
 * addData is called first, and can be used to abort going any furter 
 * by returning false. Else it should always return true.
 *
 * addData is used to add data to the data object that can be used in the 
 * content setter functions.
 */

var g_SelectionPanels = {};

// COMMAND
g_SelectionPanels.Command = {
	"maxNumberOfItems": 6,
	"setTooltip": function(data)
	{
		if (data.item.tooltip)
			data.button.tooltip = data.item.tooltip
		else
			data.button.tooltip = toTitleCase(data.item.name);
	},
	"setCountDisplay": function(data)
	{
		var count = 0;
		if (data.item.name == "unload-all")
			count = data.garrisonGroups.getTotalCount();
		data.countDisplay.caption = count || "";
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
	"maxNumberOfItems": 24,
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
		return true;
	},
	"setTooltip": function(data)
	{
		var tooltip = getEntityNamesFormatted(data.template);
		tooltip += getVisibleEntityClassesFormatted(data.template);

		if (data.template.tooltip)
			tooltip += "\n[font=\"sans-13\"]" + data.template.tooltip + "[/font]";

		tooltip += "\n" + getEntityCostTooltip(data.template);
		tooltip += getPopulationBonusTooltip(data.template);

		var limits = getEntityLimitAndCount(data.playerState, data.entType);
		data.entLimit = limits[0];
		data.entCount = limits[1];
		data.canBeAddedCount = limits[2];
		data.entLimitChangers = limits[3];

		tooltip += formatLimitString(data.entLimit, data.entCount, data.entLimitChangers);
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
		if (!data.technologyEnabled || data.canBeAddedCount == 0)
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
};

// FORMATION
g_SelectionPanels.Formation = {
	"maxNumberOfItems": 16,
	"rowLength": 4,
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
	"setTooltip": function(data)
	{
		var tooltip =  translate(data.formationInfo.name);
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
	"maxNumberOfItems": 12,
	"rowLength": 4,
	"addData": function(data)
	{
		data.entType = data.item;
		data.template = GetTemplateData(data.entType);
		if (!data.template)
			return false;
		data.name = getEntityNames(data.template);
		data.count = data.garrisonGroups.getCount(data.item);
		return true;
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
		var ents = data.garrisonGroups.getEntsByName(data.item);
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
	"maxNumberOfItems": 8,
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
			gateIcon = data.template.icon ?  "portraits/" + data.template.icon : "icons/gate_closed.png";
			data.guiSelection.hidden = true;
		}

		data.icon.sprite = "stretched:session/" + gateIcon;
	},
};

// PACK
g_SelectionPanels.Pack = {
	"maxNumberOfItems": 8,
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
};

// QUEUE
g_SelectionPanels.Queue = {
	"maxNumberOfItems": 16,
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

		if (data.i == 0)
		{
			// Also set the general progress display here
			// maybe a separate method would be more appropriate
			Engine.GetGUIObjectByName("queueProgress").caption = data.progress;
			var guiObject = Engine.GetGUIObjectByName("unitQueueProgressSlider["+data.i+"]");
			var size = guiObject.size;

			// Buttons are assumed to be square, so left/right offsets can be used for top/bottom.
			size.top = size.left + Math.round(data.item.progress * (size.right - size.left));
			guiObject.size = size;
		}

	},
	"setGraphics": function(data)
	{
		if (data.template.icon)
			data.icon.sprite = "stretched:session/portraits/" + data.template.icon;
	},
};

// RESEARCH
g_SelectionPanels.Research = {
	"maxNumberOfItems": 8,
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
	"setActions": function(data)
	{
		for (var i in data.entType)
		{
			// array containing the indices other buttons
			var others = Object.keys(data.template);
			others.splice(i, 1);
			var button = data.button[i];
			button.onpress = (function(e){ return function() { data.callback(e) } })(data.entType[i]);
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
		for  (var button of data.buttonsToHide)
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
	"maxNumberOfItems": 16,
	"rowLength": 4,
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
	"setActions": function(data)
	{
		data.button.onpressright = (function(e){return function() {data.callback(e, true) } })(data.item);
		data.button.onpress = (function(e){ return function() {data.callback(e, false) } })(data.item);
	},
	"setGraphics": function(data)
	{
		if (data.template.icon)
			data.icon.sprite = "stretched:session/portraits/" + data.template.icon;
	},
};

// STANCE
g_SelectionPanels.Stance = {
	"maxNumberOfItems": 5,
	"addData": function(data)
	{
		data.stanceSelected = Engine.GuiInterfaceCall("IsStanceSelected", {
			"ents": data.selection,
			"stance": data.item
		});
		return true;
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
	"maxNumberOfItems": 24,
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
		data.trainNum = 1;
		if (Engine.HotkeyIsPressed("session.batchtrain"))
			data.trainNum = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

		if (data.template.cost)
		{
			var totalCosts = multiplyEntityCosts(data.template, data.trainNum);
			data.neededResources = Engine.GuiInterfaceCall("GetNeededResources", totalCosts);
		}

		return true;
	},
	"setCountDisplay": function(data)
	{
		var count = "";
		if (data.trainNum > 1)
			count = data.trainNum;
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

		// TODO make the getEntityLimitAndCount method return something nicer than an array
		var limits = getEntityLimitAndCount(data.playerState, data.entType);
		data.entLimit = limits[0];
		data.entCount = limits[1];
		data.canBeAddedCount = limits[2];
		data.entLimitChangers = limits[3];

		tooltip += formatLimitString(data.entLimit, data.entCount, data.entLimitChangers);
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
};


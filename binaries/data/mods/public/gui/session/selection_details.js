function layoutSelectionSingle()
{
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
}

function layoutSelectionMultiple()
{
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
}

// Updates the health bar of garrisoned units
function updateGarrisonHealthBar(entState, selection)
{
	if (!entState.garrisonHolder)
		return;

	// Summing up the Health of every single unit
	let totalGarrisonHealth = 0;
	let maxGarrisonHealth = 0;
	for (let selEnt of selection)
	{
		let selEntState = GetEntityState(selEnt);
		if (selEntState.garrisonHolder)
			for (let ent of selEntState.garrisonHolder.entities)
			{
				let state = GetEntityState(ent);
				totalGarrisonHealth += state.hitpoints || 0;
				maxGarrisonHealth += state.maxHitpoints || 0;
			}
	}

	// Configuring the health bar
	let healthGarrison = Engine.GetGUIObjectByName("healthGarrison");
	healthGarrison.hidden = totalGarrisonHealth <= 0;
	if (totalGarrisonHealth > 0)
	{
		let healthBarGarrison = Engine.GetGUIObjectByName("healthBarGarrison");
		let healthSize = healthBarGarrison.size;
		healthSize.rtop = 100 - 100 * Math.max(0, Math.min(1, totalGarrisonHealth / maxGarrisonHealth));
		healthBarGarrison.size = healthSize;

		healthGarrison.tooltip = getCurrentHealthTooltip({
			"hitpoints": totalGarrisonHealth,
			"maxHitpoints": maxGarrisonHealth
		});
	}
}

// Fills out information that most entities have
function displaySingle(entState)
{
	let template = GetTemplateData(entState.template);

	let primaryName = g_SpecificNamesPrimary ? template.name.specific : template.name.generic;
	let secondaryName;
	if (g_ShowSecondaryNames)
		secondaryName = g_SpecificNamesPrimary ? template.name.generic : template.name.specific;

	// If packed, add that to the generic name (reduces template clutter).
	if (template.pack && template.pack.state == "packed")
	{
		if (secondaryName && g_ShowSecondaryNames)
			secondaryName = sprintf(translate("%(secondaryName)s — Packed"), { "secondaryName": secondaryName });
		else
			secondaryName = sprintf(translate("Packed"));
	}
	let playerState = g_Players[entState.player];

	let civName = g_CivData[playerState.civ].Name;
	let civEmblem = g_CivData[playerState.civ].Emblem;

	let playerName = playerState.name;

	// Indicate disconnected players by prefixing their name
	if (g_Players[entState.player].offline)
		playerName = sprintf(translate("\\[OFFLINE] %(player)s"), { "player": playerName });

	// Rank
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		const rankObj = GetTechnologyData(entState.identity.rankTechName, playerState.civ);
		Engine.GetGUIObjectByName("rankIcon").tooltip = sprintf(translate("%(rank)s Rank"), {
			"rank": translateWithContext("Rank", entState.identity.rank)
		}) + (rankObj ? "\n" + rankObj.tooltip : "");
		Engine.GetGUIObjectByName("rankIcon").sprite = "stretched:session/icons/ranks/" + entState.identity.rank + ".png";
		Engine.GetGUIObjectByName("rankIcon").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("rankIcon").hidden = true;
		Engine.GetGUIObjectByName("rankIcon").tooltip = "";
	}

	if (entState.statusEffects)
	{
		let statusEffectsSection = Engine.GetGUIObjectByName("statusEffectsIcons");
		statusEffectsSection.hidden = false;
		let statusIcons = statusEffectsSection.children;
		let i = 0;
		for (let effectCode in entState.statusEffects)
		{
			let effect = entState.statusEffects[effectCode];
			statusIcons[i].hidden = false;
			statusIcons[i].sprite = "stretched:session/icons/status_effects/" + g_StatusEffectsMetadata.getIcon(effect.baseCode) + ".png";
			statusIcons[i].tooltip = getStatusEffectsTooltip(effect.baseCode, effect, false);
			let size = statusIcons[i].size;
			size.top = i * 18;
			size.bottom = i * 18 + 16;
			statusIcons[i].size = size;

			if (++i >= statusIcons.length)
				break;
		}
		for (; i < statusIcons.length; ++i)
			statusIcons[i].hidden = true;
	}
	else
		Engine.GetGUIObjectByName("statusEffectsIcons").hidden = true;

	let showHealth = entState.hitpoints;
	let showResource = entState.resourceSupply;
	let showCapture = entState.capturePoints;

	let healthSection = Engine.GetGUIObjectByName("healthSection");
	let captureSection = Engine.GetGUIObjectByName("captureSection");
	let resourceSection = Engine.GetGUIObjectByName("resourceSection");
	let sectionPosTop = Engine.GetGUIObjectByName("sectionPosTop");
	let sectionPosMiddle = Engine.GetGUIObjectByName("sectionPosMiddle");
	let sectionPosBottom = Engine.GetGUIObjectByName("sectionPosBottom");

	// Hitpoints
	healthSection.hidden = !showHealth;
	if (showHealth)
	{
		let unitHealthBar = Engine.GetGUIObjectByName("healthBar");
		let healthSize = unitHealthBar.size;
		healthSize.rright = 100 * Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;
		Engine.GetGUIObjectByName("healthStats").caption = sprintf(translate("%(hitpoints)s / %(maxHitpoints)s"), {
			"hitpoints": Math.ceil(entState.hitpoints),
			"maxHitpoints": Math.ceil(entState.maxHitpoints)
		});

		healthSection.size = sectionPosTop.size;
		captureSection.size = showResource ? sectionPosMiddle.size : sectionPosBottom.size;
		resourceSection.size = showResource ? sectionPosBottom.size : sectionPosMiddle.size;
	}
	else if (showResource)
	{
		captureSection.size = sectionPosBottom.size;
		resourceSection.size = sectionPosTop.size;
	}
	else if (showCapture)
		captureSection.size = sectionPosTop.size;

	// CapturePoints
	captureSection.hidden = !entState.capturePoints;
	if (entState.capturePoints)
	{
		let setCaptureBarPart = function(playerID, startSize) {
			let unitCaptureBar = Engine.GetGUIObjectByName("captureBar[" + playerID + "]");
			let sizeObj = unitCaptureBar.size;
			sizeObj.rleft = startSize;

			let size = 100 * Math.max(0, Math.min(1, entState.capturePoints[playerID] / entState.maxCapturePoints));
			sizeObj.rright = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color:" + g_DiplomacyColors.getPlayerColor(playerID, 128);
			unitCaptureBar.hidden = false;
			return startSize + size;
		};

		// first handle the owner's points, to keep those points on the left for clarity
		let size = setCaptureBarPart(entState.player, 0);

		for (let i in entState.capturePoints)
			if (i != entState.player)
				size = setCaptureBarPart(i, size);

		let captureText = sprintf(translate("%(capturePoints)s / %(maxCapturePoints)s"), {
			"capturePoints": Math.ceil(entState.capturePoints[entState.player]),
			"maxCapturePoints": Math.ceil(entState.maxCapturePoints)
		});

		let showSmallCapture = showResource && showHealth;
		Engine.GetGUIObjectByName("captureStats").caption = showSmallCapture ? "" : captureText;
		Engine.GetGUIObjectByName("capture").tooltip = showSmallCapture ? captureText : "";
	}

	// Experience
	Engine.GetGUIObjectByName("experience").hidden = !entState.promotion;
	if (entState.promotion)
	{
		let experienceBar = Engine.GetGUIObjectByName("experienceBar");
		let experienceSize = experienceBar.size;
		experienceSize.rtop = 100 - (100 * Math.max(0, Math.min(1, 1.0 * +entState.promotion.curr / (+entState.promotion.req || 1))));
		experienceBar.size = experienceSize;

		if (entState.promotion.curr < entState.promotion.req)
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s / %(required)s"), {
				"experience": "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				"current": Math.floor(entState.promotion.curr),
				"required": Math.ceil(entState.promotion.req)
			});
		else
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s"), {
				"experience": "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				"current": Math.floor(entState.promotion.curr)
			});
	}

	// Resource stats
	resourceSection.hidden = !showResource;
	if (entState.resourceSupply)
	{
		let resources = entState.resourceSupply.isInfinite ? translate("∞") :  // Infinity symbol
			sprintf(translate("%(amount)s / %(max)s"), {
				"amount": Math.ceil(+entState.resourceSupply.amount),
				"max": entState.resourceSupply.max
			});

		let unitResourceBar = Engine.GetGUIObjectByName("resourceBar");
		let resourceSize = unitResourceBar.size;

		resourceSize.rright = entState.resourceSupply.isInfinite ? 100 :
			100 * Math.max(0, Math.min(1, +entState.resourceSupply.amount / +entState.resourceSupply.max));
		unitResourceBar.size = resourceSize;

		Engine.GetGUIObjectByName("resourceLabel").caption = sprintf(translate("%(resource)s:"), {
			"resource": resourceNameFirstWord(entState.resourceSupply.type.generic)
		});
		Engine.GetGUIObjectByName("resourceStats").caption = resources;

	}

	let resourceCarryingIcon = Engine.GetGUIObjectByName("resourceCarryingIcon");
	let resourceCarryingText = Engine.GetGUIObjectByName("resourceCarryingText");
	resourceCarryingIcon.hidden = false;
	resourceCarryingText.hidden = false;

	// Resource carrying
	if (entState.resourceCarrying && entState.resourceCarrying.length)
	{
		// We should only be carrying one resource type at once, so just display the first
		let carried = entState.resourceCarrying[0];
		resourceCarryingIcon.sprite = "stretched:session/icons/resources/" + carried.type + ".png";
		resourceCarryingText.caption = sprintf(translate("%(amount)s / %(max)s"), { "amount": carried.amount, "max": carried.max });
		resourceCarryingIcon.tooltip = "";
	}
	// Use the same indicators for traders
	else if (entState.trader && entState.trader.goods.amount)
	{
		resourceCarryingIcon.sprite = "stretched:session/icons/resources/" + entState.trader.goods.type + ".png";
		let totalGain = entState.trader.goods.amount.traderGain;
		if (entState.trader.goods.amount.market1Gain)
			totalGain += entState.trader.goods.amount.market1Gain;
		if (entState.trader.goods.amount.market2Gain)
			totalGain += entState.trader.goods.amount.market2Gain;
		resourceCarryingText.caption = totalGain;
		resourceCarryingIcon.tooltip = sprintf(translate("Gain: %(gain)s"), {
			"gain": getTradingTooltip(entState.trader.goods.amount)
		});
	}
	// And for number of workers
	else if (entState.foundation)
	{
		resourceCarryingIcon.sprite = "stretched:session/icons/repair.png";
		resourceCarryingIcon.tooltip = getBuildTimeTooltip(entState);
		resourceCarryingText.caption = entState.foundation.numBuilders ? sprintf(translate("(%(number)s)\n%(time)s"), {
			"number": entState.foundation.numBuilders,
			"time": Engine.FormatMillisecondsIntoDateStringGMT(entState.foundation.buildTime.timeRemaining * 1000, translateWithContext("countdown format", "m:ss"))
		}) : "";
	}
	else if (entState.resourceSupply && (!entState.resourceSupply.killBeforeGather || !entState.hitpoints))
	{
		resourceCarryingIcon.sprite = "stretched:session/icons/repair.png";
		resourceCarryingText.caption = sprintf(translate("%(amount)s / %(max)s"), {
			"amount": entState.resourceSupply.numGatherers,
			"max": entState.resourceSupply.maxGatherers
		});
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Current/max gatherers");
	}
	else if (entState.repairable && entState.needsRepair)
	{
		resourceCarryingIcon.sprite = "stretched:session/icons/repair.png";
		resourceCarryingIcon.tooltip = getRepairTimeTooltip(entState);
		resourceCarryingText.caption = entState.repairable.numBuilders ? sprintf(translate("(%(number)s)\n%(time)s"), {
			"number": entState.repairable.numBuilders,
			"time": Engine.FormatMillisecondsIntoDateStringGMT(entState.repairable.buildTime.timeRemaining * 1000, translateWithContext("countdown format", "m:ss"))
		}) : "";
	}
	else
	{
		resourceCarryingIcon.hidden = true;
		resourceCarryingText.hidden = true;
	}

	Engine.GetGUIObjectByName("player").caption = playerName;

	Engine.GetGUIObjectByName("playerColorBackground").sprite =
		"color:" + g_DiplomacyColors.getPlayerColor(entState.player, 128);

	const hideSecondary = !secondaryName || primaryName == secondaryName;

	const primaryObject = Engine.GetGUIObjectByName("primary");
	primaryObject.caption = primaryName;
	const primaryObjectSize = primaryObject.size;
	primaryObjectSize.rbottom = hideSecondary ? 100 : 50;
	primaryObject.size = primaryObjectSize;

	const secondaryObject = Engine.GetGUIObjectByName("secondary");
	secondaryObject.caption = hideSecondary ? "" :
		sprintf(translate("(%(secondaryName)s)"), {
			"secondaryName": secondaryName
		});
	secondaryObject.hidden = hideSecondary;

	let isGaia = playerState.civ == "gaia";
	Engine.GetGUIObjectByName("playerCivIcon").sprite = isGaia ? "" : "cropped:1.0, 0.15625 center:grayscale:" + civEmblem;
	Engine.GetGUIObjectByName("player").tooltip = isGaia ? "" : civName;

	// TODO: we should require all entities to have icons
	Engine.GetGUIObjectByName("icon").sprite = template.icon ? ("stretched:session/portraits/" + template.icon) : "BackgroundBlack";
	if (template.icon)
		Engine.GetGUIObjectByName("iconBorder").onPressRight = () => {
			showTemplateDetails(entState.template, playerState.civ);
		};

	let detailedTooltip = [
		getAttackTooltip,
		getHealerTooltip,
		getResistanceTooltip,
		getGatherTooltip,
		getSpeedTooltip,
		getGarrisonTooltip,
		getTurretsTooltip,
		getPopulationBonusTooltip,
		getProjectilesTooltip,
		getResourceTrickleTooltip,
		getUpkeepTooltip,
		getLootTooltip
	].map(func => func(entState)).filter(tip => tip).join("\n");
	if (detailedTooltip)
	{
		Engine.GetGUIObjectByName("attackAndResistanceStats").hidden = false;
		Engine.GetGUIObjectByName("attackAndResistanceStats").tooltip = detailedTooltip;
	}
	else
		Engine.GetGUIObjectByName("attackAndResistanceStats").hidden = true;

	let iconTooltips = [];

	iconTooltips.push(setStringTags(primaryName, g_TooltipTextFormats.namePrimaryBig));
	iconTooltips = iconTooltips.concat([
		getVisibleEntityClassesFormatted,
		getAurasTooltip,
		getEntityTooltip,
		getTreasureTooltip,
		showTemplateViewerOnRightClickTooltip
	].map(func => func(template)));

	Engine.GetGUIObjectByName("iconBorder").tooltip = iconTooltips.filter(tip => tip).join("\n");

	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
}

// Fills out information for multiple entities
function displayMultiple(entStates)
{
	let averageHealth = 0;
	let maxHealth = 0;
	let maxCapturePoints = 0;
	let capturePoints = (new Array(g_MaxPlayers + 1)).fill(0);
	let playerID = 0;
	let totalCarrying = {};
	let totalLoot = {};
	let garrisonSize = 0;

	for (let entState of entStates)
	{
		playerID = entState.player; // trust that all selected entities have the same owner
		if (entState.hitpoints)
		{
			averageHealth += entState.hitpoints;
			maxHealth += entState.maxHitpoints;
		}
		if (entState.capturePoints)
		{
			maxCapturePoints += entState.maxCapturePoints;
			capturePoints = entState.capturePoints.map((v, i) => v + capturePoints[i]);
		}

		let carrying = calculateCarriedResources(
			entState.resourceCarrying || null,
			entState.trader && entState.trader.goods
		);

		if (entState.loot)
			for (let type in entState.loot)
				totalLoot[type] = (totalLoot[type] || 0) + entState.loot[type];

		for (let type in carrying)
		{
			totalCarrying[type] = (totalCarrying[type] || 0) + carrying[type];
			totalLoot[type] = (totalLoot[type] || 0) + carrying[type];
		}

		if (entState.garrisonable)
			garrisonSize += entState.garrisonable.size;

		if (entState.garrisonHolder)
			garrisonSize += entState.garrisonHolder.occupiedSlots;
	}

	Engine.GetGUIObjectByName("healthMultiple").hidden = averageHealth <= 0;
	if (averageHealth > 0)
	{
		let unitHealthBar = Engine.GetGUIObjectByName("healthBarMultiple");
		let healthSize = unitHealthBar.size;
		healthSize.rtop = 100 - 100 * Math.max(0, Math.min(1, averageHealth / maxHealth));
		unitHealthBar.size = healthSize;

		Engine.GetGUIObjectByName("healthMultiple").tooltip = getCurrentHealthTooltip({
			"hitpoints": averageHealth,
			"maxHitpoints": maxHealth
		});
	}

	Engine.GetGUIObjectByName("captureMultiple").hidden = maxCapturePoints <= 0;
	if (maxCapturePoints > 0)
	{
		let setCaptureBarPart = function(pID, startSize)
		{
			let unitCaptureBar = Engine.GetGUIObjectByName("captureBarMultiple[" + pID + "]");
			let sizeObj = unitCaptureBar.size;
			sizeObj.rtop = startSize;

			let size = 100 * Math.max(0, Math.min(1, capturePoints[pID] / maxCapturePoints));
			sizeObj.rbottom = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color:" + g_DiplomacyColors.getPlayerColor(pID, 128);
			unitCaptureBar.hidden = false;
			return startSize + size;
		};

		let size = 0;
		for (let i in capturePoints)
			if (i != playerID)
				size = setCaptureBarPart(i, size);

		// last handle the owner's points, to keep those points on the bottom for clarity
		setCaptureBarPart(playerID, size);

		Engine.GetGUIObjectByName("captureMultiple").tooltip = getCurrentHealthTooltip(
			{
				"hitpoints": capturePoints[playerID],
				"maxHitpoints": maxCapturePoints
			},
			translate("Capture Points:"));
	}

	let numberOfUnits = Engine.GetGUIObjectByName("numberOfUnits");
	numberOfUnits.caption = entStates.length;
	numberOfUnits.tooltip = "";

	if (garrisonSize)
		numberOfUnits.tooltip = sprintf(translate("%(label)s: %(details)s\n"), {
			"label": headerFont(translate("Garrison Size")),
			"details": bodyFont(garrisonSize)
		});

	if (Object.keys(totalCarrying).length)
		numberOfUnits.tooltip = sprintf(translate("%(label)s %(details)s\n"), {
			"label": headerFont(translate("Carrying:")),
			"details": bodyFont(Object.keys(totalCarrying).filter(
				res => totalCarrying[res] != 0).map(
				res => sprintf(translate("%(type)s %(amount)s"),
					{ "type": resourceIcon(res), "amount": totalCarrying[res] })).join("  "))
		});

	if (Object.keys(totalLoot).length)
		numberOfUnits.tooltip += sprintf(translate("%(label)s %(details)s"), {
			"label": headerFont(translate("Loot:")),
			"details": bodyFont(Object.keys(totalLoot).filter(
				res => totalLoot[res] != 0).map(
				res => sprintf(translate("%(type)s %(amount)s"),
					{ "type": resourceIcon(res), "amount": totalLoot[res] })).join("  "))
		});

	// Unhide Details Area
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
}

// Updates middle entity Selection Details Panel and left Unit Commands Panel
function updateSelectionDetails()
{
	let supplementalDetailsPanel = Engine.GetGUIObjectByName("supplementalSelectionDetails");
	let detailsPanel = Engine.GetGUIObjectByName("selectionDetails");
	let commandsPanel = Engine.GetGUIObjectByName("unitCommands");

	let entStates = [];

	for (let sel of g_Selection.toList())
	{
		let entState = GetEntityState(sel);
		if (!entState)
			continue;
		entStates.push(entState);
	}

	if (entStates.length == 0)
	{
		Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
		Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
		hideUnitCommands();

		supplementalDetailsPanel.hidden = true;
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	// Fill out general info and display it
	if (entStates.length == 1)
		displaySingle(entStates[0]);
	else
		displayMultiple(entStates);

	// Show basic details.
	detailsPanel.hidden = false;

	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entStates, supplementalDetailsPanel, commandsPanel);

	// Show health bar for garrisoned units if the garrison panel is visible
	if (Engine.GetGUIObjectByName("unitGarrisonPanel") && !Engine.GetGUIObjectByName("unitGarrisonPanel").hidden)
		updateGarrisonHealthBar(entStates[0], g_Selection.toList());
}

function tradingGainString(gain, owner)
{
	// Translation: Used in the trading gain tooltip
	return sprintf(translate("%(gain)s (%(player)s)"), {
		"gain": gain,
		"player": GetSimState().players[owner].name
	});
}

/**
 * Returns a message with the details of the trade gain.
 */
function getTradingTooltip(gain)
{
	if (!gain)
		return "";

	let markets = [
		{ "gain": gain.market1Gain, "owner": gain.market1Owner },
		{ "gain": gain.market2Gain, "owner": gain.market2Owner }
	];

	let primaryGain = gain.traderGain;

	for (let market of markets)
		if (market.gain && market.owner == gain.traderOwner)
			// Translation: Used in the trading gain tooltip to concatenate profits of different players
			primaryGain += translate("+") + market.gain;

	let tooltip = tradingGainString(primaryGain, gain.traderOwner);

	for (let market of markets)
		if (market.gain && market.owner != gain.traderOwner)
			tooltip +=
				translateWithContext("Separation mark in an enumeration", ", ") +
				tradingGainString(market.gain, market.owner);

	return tooltip;
}

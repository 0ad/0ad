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

function getResourceTypeDisplayName(resourceType)
{
	let resourceCode = resourceType.generic;
	if (resourceCode == "treasure")
		return getLocalizedResourceName(resourceType.specific, "firstWord");
	else
		return getLocalizedResourceName(resourceCode, "firstWord");
}

// Updates the health bar of garrisoned units
function updateGarrisionHealthBar(entState, selection)
{
	if (!entState.garrisonHolder)
		return;

	// Summing up the Health of every single unit
	let totalGarrisionHealth = 0;
	let maxGarrisionHealth = 0;
	for (let selEnt of selection)
	{
		let selEntState = GetEntityState(selEnt);
		if (selEntState.garrisonHolder)
			for (let ent of selEntState.garrisonHolder.entities)
			{
				let state = GetEntityState(ent);
				totalGarrisionHealth += state.hitpoints || 0;
				maxGarrisionHealth += state.maxHitpoints || 0;
			}
	}

	// Configuring the health bar
	let healthGarrison = Engine.GetGUIObjectByName("healthGarrison");
	healthGarrison.hidden = totalGarrisionHealth <= 0;
	if (totalGarrisionHealth > 0)
	{
		let healthBarGarrison = Engine.GetGUIObjectByName("healthBarGarrison");
		let healthSize = healthBarGarrison.size;
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, totalGarrisionHealth / maxGarrisionHealth));
		healthBarGarrison.size = healthSize;
		healthGarrison.tooltip = sprintf(translate("%(label)s %(current)s / %(max)s"), {
			"label": "[font=\"sans-bold-13\"]" + translate("Hitpoints:") + "[/font]",
			"current": Math.ceil(totalGarrisionHealth),
			"max": Math.ceil(maxGarrisionHealth)
		});
	}
}

// Fills out information that most entities have
function displaySingle(entState)
{
	// Get general unit and player data
	let template = GetTemplateData(entState.template);
	let specificName = template.name.specific;
	let genericName = template.name.generic;
	// If packed, add that to the generic name (reduces template clutter)
	if (genericName && template.pack && template.pack.state == "packed")
		genericName = sprintf(translate("%(genericName)s — Packed"), { "genericName": genericName });
	let playerState = g_Players[entState.player];

	let civName = g_CivData[playerState.civ].Name;
	let civEmblem = g_CivData[playerState.civ].Emblem;

	let playerName = playerState.name;
	let playerColor = playerState.color.r + " " + playerState.color.g + " " + playerState.color.b + " 128";

	// Indicate disconnected players by prefixing their name
	if (g_Players[entState.player].offline)
		playerName = sprintf(translate("\\[OFFLINE] %(player)s"), { "player": playerName });

	// Rank
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		Engine.GetGUIObjectByName("rankIcon").tooltip = sprintf(translate("%(rank)s Rank"), {
			"rank": translateWithContext("Rank", entState.identity.rank)
		});
		Engine.GetGUIObjectByName("rankIcon").sprite = getRankIconSprite(entState);
		Engine.GetGUIObjectByName("rankIcon").hidden = false;
	}
	else
	{
		Engine.GetGUIObjectByName("rankIcon").hidden = true;
		Engine.GetGUIObjectByName("rankIcon").tooltip = "";
	}

	// Hitpoints
	Engine.GetGUIObjectByName("healthSection").hidden = !entState.hitpoints;
	if (entState.hitpoints)
	{
		let unitHealthBar = Engine.GetGUIObjectByName("healthBar");
		let healthSize = unitHealthBar.size;
		healthSize.rright = 100*Math.max(0, Math.min(1, entState.hitpoints / entState.maxHitpoints));
		unitHealthBar.size = healthSize;

		if (entState.foundation && entState.visibility == "visible" && entState.foundation.numBuilders !== 0)
		{
			// logic comes from Foundation component.
			let speed = Math.pow(entState.foundation.numBuilders, 0.7);
			let timeLeft = (1.0 - entState.foundation.progress / 100.0) * template.cost.time;
			let timeToCompletion = Math.ceil(timeLeft/speed);
			Engine.GetGUIObjectByName("health").tooltip = sprintf(translatePlural("This foundation will be completed in %(seconds)s second.", "This foundation will be completed in %(seconds)s seconds.", timeToCompletion), { "seconds": timeToCompletion });
		}
		else
			Engine.GetGUIObjectByName("health").tooltip = "";

		Engine.GetGUIObjectByName("healthStats").caption = sprintf(translate("%(hitpoints)s / %(maxHitpoints)s"), {
			"hitpoints": Math.ceil(entState.hitpoints),
			"maxHitpoints": Math.ceil(entState.maxHitpoints)
		});
	}

	// CapturePoints
	Engine.GetGUIObjectByName("captureSection").hidden = !entState.capturePoints;
	if (entState.capturePoints)
	{
		let setCaptureBarPart = function(playerID, startSize) {
			let unitCaptureBar = Engine.GetGUIObjectByName("captureBar["+playerID+"]");
			let sizeObj = unitCaptureBar.size;
			sizeObj.rleft = startSize;

			let size = 100*Math.max(0, Math.min(1, entState.capturePoints[playerID] / entState.maxCapturePoints));
			sizeObj.rright = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color: " + rgbToGuiColor(g_Players[playerID].color, 128);
			unitCaptureBar.hidden=false;
			return startSize + size;
		};

		// first handle the owner's points, to keep those points on the left for clarity
		let size = setCaptureBarPart(entState.player, 0);

		for (let i in entState.capturePoints)
			if (i != entState.player)
				size = setCaptureBarPart(i, size);

		Engine.GetGUIObjectByName("captureStats").caption = sprintf(translate("%(capturePoints)s / %(maxCapturePoints)s"), {
			"capturePoints": Math.ceil(entState.capturePoints[entState.player]),
			"maxCapturePoints": Math.ceil(entState.maxCapturePoints)
		});
	}

	// Experience
	Engine.GetGUIObjectByName("experience").hidden = !entState.promotion;
	if (entState.promotion)
	{
		let experienceBar = Engine.GetGUIObjectByName("experienceBar");
		let experienceSize = experienceBar.size;
		experienceSize.rtop = 100 - (100 * Math.max(0, Math.min(1, 1.0 * +entState.promotion.curr / +entState.promotion.req)));
		experienceBar.size = experienceSize;
 
		if (entState.promotion.curr < entState.promotion.req)
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s / %(required)s"), {
				"experience": "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				"current": Math.floor(entState.promotion.curr),
				"required": entState.promotion.req
			});
		else
			Engine.GetGUIObjectByName("experience").tooltip = sprintf(translate("%(experience)s %(current)s"), {
				"experience": "[font=\"sans-bold-13\"]" + translate("Experience:") + "[/font]",
				"current": Math.floor(entState.promotion.curr)
			});
	}

	// Resource stats
	Engine.GetGUIObjectByName("resourceSection").hidden = !entState.resourceSupply;
	if (entState.resourceSupply)
	{
		let resources = entState.resourceSupply.isInfinite ? translate("∞") :  // Infinity symbol
			sprintf(translate("%(amount)s / %(max)s"), {
				"amount": Math.ceil(+entState.resourceSupply.amount),
				"max": entState.resourceSupply.max
			});

		let resourceType = getResourceTypeDisplayName(entState.resourceSupply.type);

		let unitResourceBar = Engine.GetGUIObjectByName("resourceBar");
		let resourceSize = unitResourceBar.size;

		resourceSize.rright = entState.resourceSupply.isInfinite ? 100 :
						100 * Math.max(0, Math.min(1, +entState.resourceSupply.amount / +entState.resourceSupply.max));
		unitResourceBar.size = resourceSize;
		Engine.GetGUIObjectByName("resourceLabel").caption = sprintf(translate("%(resource)s:"), { "resource": resourceType });
		Engine.GetGUIObjectByName("resourceStats").caption = resources;

		if (entState.hitpoints)
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("captureSection").size;
		else
			Engine.GetGUIObjectByName("resourceSection").size = Engine.GetGUIObjectByName("healthSection").size;
	}

	// Resource carrying
	if (entState.resourceCarrying && entState.resourceCarrying.length)
	{
		// We should only be carrying one resource type at once, so just display the first
		let carried = entState.resourceCarrying[0];

		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+carried.type+".png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = sprintf(translate("%(amount)s / %(max)s"), { "amount": carried.amount, "max": carried.max });
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = "";
	}
	// Use the same indicators for traders
	else if (entState.trader && entState.trader.goods.amount)
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/resources/"+entState.trader.goods.type+".png";
		let totalGain = entState.trader.goods.amount.traderGain;
		if (entState.trader.goods.amount.market1Gain)
			totalGain += entState.trader.goods.amount.market1Gain;
		if (entState.trader.goods.amount.market2Gain)
			totalGain += entState.trader.goods.amount.market2Gain;
		Engine.GetGUIObjectByName("resourceCarryingText").caption = totalGain;
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = sprintf(translate("Gain: %(gain)s"), { "gain": getTradingTooltip(entState.trader.goods.amount) });
	}
	// And for number of workers
	else if (entState.foundation && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = entState.foundation.numBuilders + "    ";
		if (entState.foundation.numBuilders !== 0)
		{
			let speedup = Math.pow((entState.foundation.numBuilders+1)/entState.foundation.numBuilders, 0.7);
			let timeLeft = (1.0 - entState.foundation.progress / 100.0) * template.cost.time;
			let timeSpeedup = Math.ceil(timeLeft - timeLeft/speedup);
			Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = sprintf(translatePlural("Number of builders.\nTasking another to this foundation would speed construction up by %(speedup)s second.", "Number of builders.\nTasking another to this foundation would speed construction up by %(speedup)s seconds.", timeSpeedup), { "speedup": timeSpeedup });
		}
		else
			Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Number of builders.");
	}
	else if (entState.repairable && entState.repairable.numBuilders > 0 && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = entState.repairable.numBuilders + "    ";
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Number of builders.");
	}
	else if (entState.resourceSupply && (!entState.resourceSupply.killBeforeGather || !entState.hitpoints) && entState.visibility == "visible")
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = false;
		Engine.GetGUIObjectByName("resourceCarryingIcon").sprite = "stretched:session/icons/repair.png";
		Engine.GetGUIObjectByName("resourceCarryingText").caption = sprintf(translate("%(amount)s / %(max)s"), {
			"amount": entState.resourceSupply.numGatherers,
			"max": entState.resourceSupply.maxGatherers
		}) + "    ";
		Engine.GetGUIObjectByName("resourceCarryingIcon").tooltip = translate("Current/max gatherers");
	}
	else
	{
		Engine.GetGUIObjectByName("resourceCarryingIcon").hidden = true;
		Engine.GetGUIObjectByName("resourceCarryingText").hidden = true;
	}

	Engine.GetGUIObjectByName("specific").caption = specificName;
	Engine.GetGUIObjectByName("player").caption = playerName;
	Engine.GetGUIObjectByName("playerColorBackground").sprite = "color: " + playerColor;
	Engine.GetGUIObjectByName("generic").caption = genericName == specificName ? "" :
		sprintf(translate("(%(genericName)s)"), {
			"genericName": genericName
		});

	let isGaia = "gaia" == playerState.civ;
	Engine.GetGUIObjectByName("playerCivIcon").sprite = isGaia ? "" : "stretched:grayscale:" + civEmblem;
	Engine.GetGUIObjectByName("player").tooltip = isGaia ? "" : civName;

	// TODO: we should require all entities to have icons
	Engine.GetGUIObjectByName("icon").sprite = template.icon ? ("stretched:session/portraits/" + template.icon) : "BackgroundBlack";

	Engine.GetGUIObjectByName("attackAndArmorStats").tooltip = [
		getAttackTooltip,
		getSplashDamageTooltip,
		getHealerTooltip,
		getArmorTooltip,
		getRepairRateTooltip,
		getBuildRateTooltip,
		getSpeedTooltip,
		getGarrisonTooltip,
		getProjectilesTooltip
	].map(func => func(entState)).filter(tip => tip).join("\n");

	let iconTooltips = [];

	if (genericName)
		iconTooltips.push("[font=\"sans-bold-16\"]" + genericName + "[/font]");

	iconTooltips = iconTooltips.concat([
		getVisibleEntityClassesFormatted,
		getAurasTooltip,
		getEntityTooltip
	].map(func => func(template)));

	Engine.GetGUIObjectByName("iconBorder").tooltip = iconTooltips.filter(tip => tip).join("\n");

	Engine.GetGUIObjectByName("detailsAreaSingle").hidden = false;
	Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
}

// Fills out information for multiple entities
function displayMultiple(selection)
{
	let averageHealth = 0;
	let maxHealth = 0;
	let maxCapturePoints = 0;
	let capturePoints = (new Array(g_MaxPlayers + 1)).fill(0);
	let playerID = 0;
	let totalResourcesCarried = {};

	for (let i = 0; i < selection.length; ++i)
	{
		let entState = GetEntityState(selection[i]);
		if (!entState)
			continue;
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

		if (entState.resourceCarrying && entState.resourceCarrying.length)
		{
			let carrying = entState.resourceCarrying[0];
			totalResourcesCarried[carrying.type] = (totalResourcesCarried[carrying.type] || 0) + carrying.amount;
		}
	}

	Engine.GetGUIObjectByName("healthMultiple").hidden = averageHealth <= 0;
	if (averageHealth > 0)
	{
		let unitHealthBar = Engine.GetGUIObjectByName("healthBarMultiple");
		let healthSize = unitHealthBar.size;
		healthSize.rtop = 100-100*Math.max(0, Math.min(1, averageHealth / maxHealth));
		unitHealthBar.size = healthSize;

		Engine.GetGUIObjectByName("healthMultiple").tooltip = sprintf(translate("%(label)s %(current)s / %(max)s"), {
			"label": "[font=\"sans-bold-13\"]" + translate("Hitpoints:") + "[/font]",
			"current": Math.ceil(averageHealth),
			"max": Math.ceil(maxHealth)
		});
	}

	Engine.GetGUIObjectByName("captureMultiple").hidden = maxCapturePoints <= 0;
	if (maxCapturePoints > 0)
	{
		let setCaptureBarPart = function(playerID, startSize)
		{
			let unitCaptureBar = Engine.GetGUIObjectByName("captureBarMultiple["+playerID+"]");
			let sizeObj = unitCaptureBar.size;
			sizeObj.rtop = startSize;

			let size = 100*Math.max(0, Math.min(1, capturePoints[playerID] / maxCapturePoints));
			sizeObj.rbottom = startSize + size;
			unitCaptureBar.size = sizeObj;
			unitCaptureBar.sprite = "color: " + rgbToGuiColor(g_Players[playerID].color, 128);
			unitCaptureBar.hidden = false;
			return startSize + size;
		};

		let size = 0;
		for (let i in capturePoints)
			if (i != playerID)
				size = setCaptureBarPart(i, size);

		// last handle the owner's points, to keep those points on the bottom for clarity
		setCaptureBarPart(playerID, size);

		Engine.GetGUIObjectByName("captureMultiple").tooltip = sprintf(translate("%(label)s %(current)s / %(max)s"), {
			"label": "[font=\"sans-bold-13\"]" + translate("Capture points:") + "[/font]",
			"current": Math.ceil(capturePoints[playerID]),
			"max": Math.ceil(maxCapturePoints)
		});
	}

	let numberOfUnits = Engine.GetGUIObjectByName("numberOfUnits");
	numberOfUnits.caption = selection.length;
	numberOfUnits.tooltip = Object.keys(totalResourcesCarried).map(res =>
		costIcon(res) + totalResourcesCarried[res]
	).join(" ");

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

	let selection = g_Selection.toList();

	if (selection.length == 0)
	{
		Engine.GetGUIObjectByName("detailsAreaMultiple").hidden = true;
		Engine.GetGUIObjectByName("detailsAreaSingle").hidden = true;
		hideUnitCommands();

		supplementalDetailsPanel.hidden = true;
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	/* If the unit has no data (e.g. it was killed), don't try displaying any
	 data for it. (TODO: it should probably be removed from the selection too;
	 also need to handle multi-unit selections) */
	let entState = GetExtendedEntityState(selection[0]);
	if (!entState)
		return;

	// Fill out general info and display it
	if (selection.length == 1)
		displaySingle(entState);
	else
		displayMultiple(selection);

	// Show basic details.
	detailsPanel.hidden = false;

	// Fill out commands panel for specific unit selected (or first unit of primary group)
	updateUnitCommands(entState, supplementalDetailsPanel, commandsPanel, selection);

	// Show health bar for garrisoned units if the garrison panel is visible
	if (Engine.GetGUIObjectByName("unitGarrisonPanel") && !Engine.GetGUIObjectByName("unitGarrisonPanel").hidden)
		updateGarrisionHealthBar(entState, selection);
}

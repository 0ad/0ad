var g_TooltipTextFormats = {
	"unit": { "font": "sans-10", "color": "orange" },
	"header": { "font": "sans-bold-13" },
	"body": { "font": "sans-13" },
	"comma": { "font": "sans-12" },
	"nameSpecificBig": { "font": "sans-bold-16" },
	"nameSpecificSmall": { "font": "sans-bold-12" },
	"nameGeneric": { "font": "sans-bold-16" }
};

var g_AttackTypes = {
	"Melee": translate("Melee Attack:"),
	"Ranged": translate("Ranged Attack:"),
	"Capture": translate("Capture Attack:")
};

var g_SplashDamageTypes = {
	"Circular": translate("Circular Splash Damage"),
	"Linear": translate("Linear Splash Damage")
};

var g_RangeTooltipString = {
	"relative": {
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 2 to 10 (+2) meters, Interval: 3 arrows / 2 seconds
		"minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(minRange)s to %(maxRange)s (%(relativeRange)s) %(rangeUnit)s, %(rate)s"),
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 10 (+2) meters, Interval: 3 arrows / 2 seconds
		"no-minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(maxRange)s (%(relativeRange)s) %(rangeUnit)s, %(rate)s"),
	},
	"non-relative": {
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 2 to 10 meters, Interval: 3 arrows / 2 seconds
		"minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(minRange)s to %(maxRange)s %(rangeUnit)s, %(rate)s"),
		// Translation: For example: Ranged Attack: 12.0 Pierce, Range: 10 meters, Interval: 3 arrows / 2 seconds
		"no-minRange": translate("%(attackLabel)s %(damageTypes)s, %(rangeLabel)s %(maxRange)s %(rangeUnit)s, %(rate)s"),
	}
};

function getCostTypes()
{
	return g_ResourceData.GetCodes().concat(["population", "populationBonus", "time"]);
}

/**
 * If true, always shows whether the splash damage deals friendly fire.
 * Otherwise display the friendly fire tooltip only if it does.
 */
var g_AlwaysDisplayFriendlyFire = false;

function resourceIcon(resource)
{
	return '[icon="icon_' + resource + '"]';
}

function resourceNameFirstWord(type)
{
	return translateWithContext("firstWord", g_ResourceData.GetNames()[type]);
}

function resourceNameWithinSentence(type)
{
	return translateWithContext("withinSentence", g_ResourceData.GetNames()[type]);
}

/**
 * Format resource amounts to proper english and translate (for example: "200 food, 100 wood and 300 metal").
 */
function getLocalizedResourceAmounts(resources)
{
	let amounts = g_ResourceData.GetCodes()
		.filter(type => !!resources[type])
		.map(type => sprintf(translate("%(amount)s %(resourceType)s"), {
			"amount": resources[type],
			"resourceType": resourceNameWithinSentence(type)
		}));

	if (amounts.length < 2)
		return amounts.join();

	let lastAmount = amounts.pop();
	return sprintf(translate("%(previousAmounts)s and %(lastAmount)s"), {
		// Translation: This comma is used for separating first to penultimate elements in an enumeration.
		"previousAmounts": amounts.join(translate(", ")),
		"lastAmount": lastAmount
	});
}

function bodyFont(text)
{
	return setStringTags(text, g_TooltipTextFormats.body);
}

function headerFont(text)
{
	return setStringTags(text, g_TooltipTextFormats.header);
}

function unitFont(text)
{
	return setStringTags(text, g_TooltipTextFormats.unit);
}

function commaFont(text)
{
	return setStringTags(text, g_TooltipTextFormats.comma);
}

function getSecondsString(seconds)
{
	return sprintf(translatePlural("%(time)s %(second)s", "%(time)s %(second)s", seconds), {
		"time": seconds,
		"second": unitFont(translatePlural("second", "seconds", seconds))
	});
}

/**
 * Entity templates have a `Tooltip` tag in the Identity component.
 * (The contents of which are copied to a `tooltip` attribute in globalscripts.)
 *
 * Technologies have a `tooltip` attribute.
 */
function getEntityTooltip(template)
{
	if (!template.tooltip)
		return "";

	return bodyFont(template.tooltip);
}

/**
 * Technologies have a `description` attribute, and Auras have an `auraDescription`
 * attribute, which becomes `description`.
 *
 * (For technologies, this happens in globalscripts.)
 *
 * (For auras, this happens either in the Auras component (for session gui) or
 * reference/common/load.js (for Reference Suite gui))
 */
function getDescriptionTooltip(template)
{
	if (!template.description)
		return "";

	return bodyFont(template.description);
}

/**
 * Entity templates have a `History` tag in the Identity component.
 * (The contents of which are copied to a `history` attribute in globalscripts.)
 */
function getHistoryTooltip(template)
{
	if (!template.history)
		return "";

	return bodyFont(template.history);
}

function getHealthTooltip(template)
{
	if (!template.health)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Health:")),
		"details": template.health
	});
}

function getCurrentHealthTooltip(entState, label)
{
	if (!entState.maxHitpoints)
		return "";

	return sprintf(translate("%(healthLabel)s %(current)s / %(max)s"), {
		"healthLabel": headerFont(label || translate("Health:")),
		"current": Math.round(entState.hitpoints),
		"max": Math.round(entState.maxHitpoints)
	});
}

function attackRateDetails(template, type)
{
	// Either one arrow shot by UnitAI,
	let timeString = getSecondsString(template.attack[type].repeatTime / 1000);

	// or multiple arrows shot by BuildingAI
	if (!template.buildingAI || type != "Ranged")
		return timeString;

	// Show either current rate from simulation or default count if the sim is not running
	let arrows = template.buildingAI.arrowCount || template.buildingAI.defaultArrowCount;
	let arrowString = sprintf(translatePlural("%(arrowcount)s %(arrows)s", "%(arrowcount)s %(arrows)s", arrows), {
		"arrowcount": arrows,
		"arrows": unitFont(translatePlural("arrow", "arrows", arrows))
	});

	return sprintf(translate("%(arrowString)s / %(timeString)s"), {
		"arrowString": arrowString,
		"timeString": timeString
	});
}

/**
 * Converts an armor level into the actual reduction percentage
 */
function armorLevelToPercentageString(level)
{
	return sprintf(translate("%(percentage)s%%"), {
		"percentage": (100 - Math.round(Math.pow(0.9, level) * 100))
	});
}

function getArmorTooltip(template)
{
	if (!template.armour)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Armor:")),
		"details":
			Object.keys(template.armour).map(
				dmgType => sprintf(translate("%(damage)s %(damageType)s %(armorPercentage)s"), {
					"damage": template.armour[dmgType].toFixed(1),
					"damageType": unitFont(translateWithContext("damage type", dmgType)),
					"armorPercentage":
						'[font="sans-10"]' +
						sprintf(translate("(%(armorPercentage)s)"), {
							"armorPercentage": armorLevelToPercentageString(template.armour[dmgType])
						}) + '[/font]'
				})
			).join(commaFont(translate(", ")))
	});
}

function damageTypesToText(dmg)
{
	if (!dmg)
		return '[font="sans-12"]' + translate("(None)") + '[/font]';

	return Object.keys(dmg).filter(dmgType => dmg[dmgType]).map(
		dmgType => sprintf(translate("%(damage)s %(damageType)s"), {
			"damage": dmg[dmgType].toFixed(1),
			"damageType": unitFont(translateWithContext("damage type", dmgType))
		})).join(commaFont(translate(", ")));
}

function getAttackTooltip(template)
{
	if (!template.attack)
		return "";

	let tooltips = [];
	for (let type in template.attack)
	{
		if (type == "Slaughter")
			continue; // Slaughter is used to kill animals, so do not show it.

		let rate = sprintf(translate("%(label)s %(details)s"), {
			"label":
				headerFont(
					template.buildingAI && type == "Ranged" ?
						translate("Interval:") :
						translate("Rate:")),
			"details": attackRateDetails(template, type)
		});

		let attackLabel = headerFont(g_AttackTypes[type]);
		if (type == "Capture" || type != "Ranged")
		{
			tooltips.push(sprintf(translate("%(attackLabel)s %(details)s, %(rate)s"), {
				"attackLabel": attackLabel,
				"details":
					type == "Capture" ?
						template.attack.Capture.Capture :
						damageTypesToText(template.attack[type].Damage),
				"rate": rate
			}));
			continue;
		}

		let minRange = Math.round(template.attack[type].minRange);
		let maxRange = Math.round(template.attack[type].maxRange);
		let realRange = template.attack[type].elevationAdaptedRange;
		let relativeRange = realRange ? Math.round(realRange - maxRange) : 0;
		tooltips.push(sprintf(g_RangeTooltipString[relativeRange ? "relative" : "non-relative"][minRange ? "minRange" : "no-minRange"], {
			"attackLabel": attackLabel,
			"damageTypes": damageTypesToText(template.attack[type].Damage),
			"rangeLabel": headerFont(translate("Range:")),
			"minRange": minRange,
			"maxRange": maxRange,
			"relativeRange": relativeRange > 0 ? sprintf(translate("+%(number)s"), { "number": relativeRange }) : relativeRange,
			"rangeUnit":
				unitFont(minRange || relativeRange ?
					// Translation: For example "0.5 to 1 meters", "1 (+1) meters" or "1 to 2 (+3) meters"
					translate("meters") :
					translatePlural("meter", "meters", maxRange)),
			"rate": rate,
		}));
	}
	return tooltips.join("\n");
}

function getSplashDamageTooltip(template)
{
	if (!template.attack)
		return "";

	let tooltips = [];
	for (let type in template.attack)
	{
		let splash = template.attack[type].splash;
		if (!splash)
			continue;

		let splashDamageTooltip = sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(g_SplashDamageTypes[splash.shape]),
			"value": damageTypesToText(splash.Damage)
		});

		if (g_AlwaysDisplayFriendlyFire || splash.friendlyFire)
			splashDamageTooltip += commaFont(translate(", ")) + sprintf(translate("Friendly Fire: %(enabled)s"), {
				"enabled": splash.friendlyFire ? translate("Yes") : translate("No")
			});

		tooltips.push(splashDamageTooltip);
	}

	// If multiple attack types deal splash damage, the attack type should be shown to differentiate.
	return tooltips.join("\n");
}

function getGarrisonTooltip(template)
{
	if (!template.garrisonHolder)
		return "";

	let tooltips = [
		sprintf(translate("%(label)s: %(garrisonLimit)s"), {
			"label": headerFont(translate("Garrison Limit")),
			"garrisonLimit": template.garrisonHolder.capacity
		})
	];

	if (template.garrisonHolder.buffHeal)
		tooltips.push(
			sprintf(translate("%(healRateLabel)s %(value)s %(health)s / %(second)s"), {
				"healRateLabel": headerFont(translate("Heal:")),
				"value": Math.round(template.garrisonHolder.buffHeal),
				"health": unitFont(translate("Health")),
				"second": unitFont(translate("second")),
			})
		);

	return tooltips.join(commaFont(translate(", ")));
}

function getProjectilesTooltip(template)
{
	if (!template.garrisonHolder || !template.buildingAI)
		return "";

	let limit = Math.min(
		template.buildingAI.maxArrowCount || Infinity,
		template.buildingAI.defaultArrowCount +
			Math.round(template.buildingAI.garrisonArrowMultiplier *
			template.garrisonHolder.capacity)
	);

	if (!limit)
		return "";

	return [
		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translate("Projectile Limit")),
			"value": limit
		}),

		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translateWithContext("projectiles", "Default")),
			"value": template.buildingAI.defaultArrowCount
		}),

		sprintf(translate("%(label)s: %(value)s"), {
			"label": headerFont(translateWithContext("projectiles", "Per Unit")),
			"value": +template.buildingAI.garrisonArrowMultiplier.toFixed(2)
		})
	].join(commaFont(translate(", ")));
}

function getRepairTimeTooltip(entState)
{
	return sprintf(translate("%(label)s %(details)s"), {
			"label": headerFont(translate("Number of repairers:")),
			"details": entState.repairable.numBuilders
		}) + "\n" + (entState.repairable.numBuilders ?
		sprintf(translatePlural(
			"Add another worker to speed up the repairs by %(second)s second.",
			"Add another worker to speed up the repairs by %(second)s seconds.",
			Math.round(entState.repairable.buildTime.timeRemaining - entState.repairable.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.repairable.buildTime.timeRemaining - entState.repairable.buildTime.timeRemainingNew)
		}) :
		sprintf(translatePlural(
			"Add a worker to finish the repairs in %(second)s second.",
			"Add a worker to finish the repairs in %(second)s seconds.",
			Math.round(entState.repairable.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.repairable.buildTime.timeRemainingNew)
		}));
}

function getBuildTimeTooltip(entState)
{
	return sprintf(translate("%(label)s %(details)s"), {
			"label": headerFont(translate("Number of builders:")),
			"details": entState.foundation.numBuilders
		}) + "\n" + (entState.foundation.numBuilders ?
		sprintf(translatePlural(
			"Add another worker to speed up the construction by %(second)s second.",
			"Add another worker to speed up the construction by %(second)s seconds.",
			Math.round(entState.foundation.buildTime.timeRemaining - entState.foundation.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.foundation.buildTime.timeRemaining - entState.foundation.buildTime.timeRemainingNew)
		}) :
		sprintf(translatePlural(
			"Add a worker to finish the construction in %(second)s second.",
			"Add a worker to finish the construction in %(second)s seconds.",
			Math.round(entState.foundation.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.foundation.buildTime.timeRemainingNew)
		}));
}

/**
 * Multiplies the costs for a template by a given batch size.
 */
function multiplyEntityCosts(template, trainNum)
{
	let totalCosts = {};
	for (let r of getCostTypes())
		if (template.cost[r])
			totalCosts[r] = Math.floor(template.cost[r] * trainNum);

	return totalCosts;
}

/**
 * Helper function for getEntityCostTooltip.
 */
function getEntityCostComponentsTooltipString(template, entity, buildingsCountToTrainFullBatch = 1, fullBatchSize = 1, remainderBatch = 0)
{
	let totalCosts = multiplyEntityCosts(template, buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch);
	if (template.cost.time)
		totalCosts.time = Math.ceil(template.cost.time * (entity ? Engine.GuiInterfaceCall("GetBatchTime", {
			"entity": entity,
			"batchSize": buildingsCountToTrainFullBatch > 0 ? fullBatchSize : remainderBatch
		}) : 1));

	let costs = [];
	for (let type of getCostTypes())
		// Population bonus is shown in the tooltip
		if (type != "populationBonus" && totalCosts[type])
			costs.push(sprintf(translate("%(component)s %(cost)s"), {
				"component": resourceIcon(type),
				"cost": totalCosts[type]
			}));

	return costs;
}

function getGatherTooltip(template)
{
	if (!template.resourceGatherRates)
		return "";

	// Average the resource rates (TODO: distinguish between subtypes)
	let rates = {};
	for (let resource of g_ResourceData.GetResources())
	{
		let types = [resource.code];
		for (let subtype in resource.subtypes)
			// We ignore ruins as those are not that common and skew the results
			if (subtype !== "ruins")
				types.push(resource.code + "." + subtype);

		let [rate, count] = types.reduce((sum, t) => {
			let r = template.resourceGatherRates[t];
			return [sum[0] + (r > 0 ? r : 0), sum[1] + (r > 0 ? 1 : 0)];
		}, [0, 0]);

		if (rate > 0)
			rates[resource.code] = +(rate / count).toFixed(1);
	}

	if (!Object.keys(rates).length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Gather Rates:")),
		"details":
			Object.keys(rates).map(
				type => sprintf(translate("%(resourceIcon)s %(rate)s"), {
					"resourceIcon": resourceIcon(type),
					"rate": rates[type]
				})
			).join("  ")
	});
}

function getResourceTrickleTooltip(template)
{
	if (!template.resourceTrickle)
		return "";

	let resCodes = g_ResourceData.GetCodes().filter(res => !!template.resourceTrickle.rates[res]);
	if (!resCodes.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Resource Trickle:")),
		"details": sprintf(translate("%(resources)s / %(time)s"), {
			"resources":
				resCodes.map(
					res => sprintf(translate("%(resourceIcon)s %(rate)s"), {
						"resourceIcon": resourceIcon(res),
						"rate": template.resourceTrickle.rates[res]
					})
				).join("  "),
			"time": getSecondsString(template.resourceTrickle.interval / 1000)
		})
	});
}

/**
 * Returns an array of strings for a set of wall pieces. If the pieces share
 * resource type requirements, output will be of the form '10 to 30 Stone',
 * otherwise output will be, e.g. '10 Stone, 20 Stone, 30 Stone'.
 */
function getWallPieceTooltip(wallTypes)
{
	let out = [];
	let resourceCount = {};
	for (let resource of getCostTypes())
		if (wallTypes[0].cost[resource])
			resourceCount[resource] = [wallTypes[0].cost[resource]];

	let sameTypes = true;
	for (let i = 1; i < wallTypes.length; ++i)
	{
		for (let resource in wallTypes[i].cost)
			// Break out of the same-type mode if this wall requires
			// resource types that the first didn't.
			if (wallTypes[i].cost[resource] && !resourceCount[resource])
			{
				sameTypes = false;
				break;
			}

		for (let resource in resourceCount)
			if (wallTypes[i].cost[resource])
				resourceCount[resource].push(wallTypes[i].cost[resource]);
			else
			{
				sameTypes = false;
				break;
			}
	}

	if (sameTypes)
		for (let resource in resourceCount)
			// Translation: This string is part of the resources cost string on
			// the tooltip for wall structures.
			out.push(sprintf(translate("%(resourceIcon)s %(minimum)s to %(resourceIcon)s %(maximum)s"), {
				"resourceIcon": resourceIcon(resource),
				"minimum": Math.min.apply(Math, resourceCount[resource]),
				"maximum": Math.max.apply(Math, resourceCount[resource])
			}));
	else
		for (let i = 0; i < wallTypes.length; ++i)
			out.push(getEntityCostComponentsTooltipString(wallTypes[i]).join(", "));

	return out;
}

/**
 * Returns the cost information to display in the specified entity's construction button tooltip.
 */
function getEntityCostTooltip(template, entity, buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
{
	// Entities with a wallset component are proxies for initiating wall placement and as such do not have a cost of
	// their own; the individual wall pieces within it do.
	if (template.wallSet)
	{
		let templateLong = GetTemplateData(template.wallSet.templates.long);
		let templateMedium = GetTemplateData(template.wallSet.templates.medium);
		let templateShort = GetTemplateData(template.wallSet.templates.short);
		let templateTower = GetTemplateData(template.wallSet.templates.tower);

		let wallCosts = getWallPieceTooltip([templateShort, templateMedium, templateLong]);
		let towerCosts = getEntityCostComponentsTooltipString(templateTower);

		return sprintf(translate("Walls:  %(costs)s"), { "costs": wallCosts.join("  ") }) + "\n" +
		       sprintf(translate("Towers:  %(costs)s"), { "costs": towerCosts.join("  ") });
	}

	if (template.cost)
	{
		let costs = getEntityCostComponentsTooltipString(template, entity, buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch).join("  ");
		if (costs)
			// Translation: Label in tooltip showing cost of a unit, structure or technology.
			return sprintf(translate("%(label)s %(costs)s"), {
				"label": headerFont(translate("Cost:")),
				"costs": costs
			});
	}

	return "";
}

function getRequiredTechnologyTooltip(technologyEnabled, requiredTechnology, civ)
{
	if (technologyEnabled)
		return "";

	return sprintf(translate("Requires %(technology)s"), {
		"technology": getEntityNames(GetTechnologyData(requiredTechnology, civ))
	});
}

/**
 * Returns the population bonus information to display in the specified entity's construction button tooltip.
 */
function getPopulationBonusTooltip(template)
{
	let popBonus = "";
	if (template.cost && template.cost.populationBonus)
		popBonus = sprintf(translate("%(label)s %(populationBonus)s"), {
			"label": headerFont(translate("Population Bonus:")),
			"populationBonus": template.cost.populationBonus
		});
	return popBonus;
}

/**
 * Returns a message with the amount of each resource needed to create an entity.
 */
function getNeededResourcesTooltip(resources)
{
	if (!resources)
		return "";

	let formatted = [];
	for (let resource in resources)
		formatted.push(sprintf(translate("%(component)s %(cost)s"), {
			"component": '[font="sans-12"]' + resourceIcon(resource) + '[/font]',
			"cost": resources[resource]
		}));
	return coloredText(
		'[font="sans-bold-13"]' + translate("Insufficient resources:") + '[/font]',
		"red") + " " +
		formatted.join("  ");
}

function getSpeedTooltip(template)
{
	if (!template.speed)
		return "";

	let walk = template.speed.walk.toFixed(1);
	let run = template.speed.run.toFixed(1);

	if (walk == 0 && run == 0)
		return "";

	return sprintf(translate("%(label)s %(speeds)s"), {
		"label": headerFont(translate("Speed:")),
		"speeds":
			sprintf(translate("%(speed)s %(movementType)s"), {
				"speed": walk,
				"movementType": unitFont(translate("Walk"))
			}) +
			commaFont(translate(", ")) +
			sprintf(translate("%(speed)s %(movementType)s"), {
				"speed": run,
				"movementType": unitFont(translate("Run"))
			})
	});
}

function getHealerTooltip(template)
{
	if (!template.heal)
		return "";

	let hp = +(template.heal.hp.toFixed(1));
	let range = +(template.heal.range.toFixed(0));
	let rate = +((template.heal.rate / 1000).toFixed(1));

	return [
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", hp), {
			"label": headerFont(translate("Heal:")),
			"val": hp,
			// Translation: Short for hit points (or health points) that are healed in one healing action
			"unit": unitFont(translatePlural("HP", "HP", hp))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", range), {
			"label": headerFont(translate("Range:")),
			"val": range,
			"unit": unitFont(translatePlural("meter", "meters", range))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", rate), {
			"label": headerFont(translate("Rate:")),
			"val": rate,
			"unit": unitFont(translatePlural("second", "seconds", rate))
		})
	].join(translate(", "));
}

function getAurasTooltip(template)
{
	let auras = template.auras || template.wallSet && GetTemplateData(template.wallSet.templates.long).auras;
	if (!auras)
		return "";

	let tooltips = [];
	for (let auraID in auras)
	{
		let tooltip = sprintf(translate("%(auralabel)s %(aurainfo)s"), {
			"auralabel": headerFont(sprintf(translate("%(auraname)s:"), {
				"auraname": translate(auras[auraID].name)
			})),
			"aurainfo": bodyFont(translate(auras[auraID].description))
		});
		let radius = +auras[auraID].radius;
		if (radius)
			tooltip += " " + sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", radius), {
				"label": translateWithContext("aura", "Range:"),
				"val": radius,
				"unit": unitFont(translatePlural("meter", "meters", radius))
			});
		tooltips.push(tooltip);
	}
	return tooltips.join("\n");
}

function getEntityNames(template)
{
	if (!template.name.specific)
		return template.name.generic;

	if (template.name.specific == template.name.generic)
		return template.name.specific;

	return sprintf(translate("%(specificName)s (%(genericName)s)"), {
		"specificName": template.name.specific,
		"genericName": template.name.generic
	});

}
function getEntityNamesFormatted(template)
{
	if (!template.name.specific)
		return setStringTags(template.name.generic, g_TooltipTextFormats.nameSpecificBig);

	// Translation: Example: "Epibátēs Athēnaîos [font="sans-bold-16"](Athenian Marine)[/font]"
	return sprintf(translate("%(specificName)s %(fontStart)s(%(genericName)s)%(fontEnd)s"), {
		"specificName":
			setStringTags(template.name.specific[0], g_TooltipTextFormats.nameSpecificBig) +
			setStringTags(template.name.specific.slice(1).toUpperCase(), g_TooltipTextFormats.nameSpecificSmall),
		"genericName": template.name.generic,
		"fontStart": '[font="' + g_TooltipTextFormats.nameGeneric.font + '"]',
		"fontEnd": '[/font]'
	});
}

function getVisibleEntityClassesFormatted(template)
{
	if (!template.visibleIdentityClasses || !template.visibleIdentityClasses.length)
		return "";

	return headerFont(translate("Classes:")) + ' ' +
		bodyFont(template.visibleIdentityClasses.map(c => translate(c)).join(translate(", ")));
}

function getLootTooltip(template)
{
	if (!template.loot && !template.resourceCarrying)
		return "";

	let resourcesCarried = [];
	if (template.resourceCarrying)
		resourcesCarried = calculateCarriedResources(
			template.resourceCarrying,
			template.trader && template.trader.goods
		);

	let lootLabels = [];
	for (let type of g_ResourceData.GetCodes().concat(["xp"]))
	{
		let loot =
			(template.loot && template.loot[type] || 0) +
			(resourcesCarried[type] || 0);

		if (!loot)
			continue;

		// Translation: %(component) will be the icon for the loot type and %(loot) will be the value.
		lootLabels.push(sprintf(translate("%(component)s %(loot)s"), {
			"component": resourceIcon(type),
			"loot": loot
		}));
	}

	if (!lootLabels.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Loot:")),
		"details": lootLabels.join("  ")
	});
}

function showTemplateViewerOnRightClickTooltip()
{
	// Translation: Appears in a tooltip to indicate that right-clicking the corresponding GUI element will open the Template Details GUI page.
	return translate("Right-click to view more information.");
}

function showTemplateViewerOnClickTooltip()
{
	// Translation: Appears in a tooltip to indicate that clicking the corresponding GUI element will open the Template Details GUI page.
	return translate("Click to view more information.");
}

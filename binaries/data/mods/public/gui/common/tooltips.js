var g_TooltipTextFormats = {
	"unit": { "font": "sans-10", "color": "orange" },
	"header": { "font": "sans-bold-13" },
	"body": { "font": "sans-13" },
	"comma": { "font": "sans-12" },
	"namePrimaryBig": { "font": "sans-bold-16" },
	"namePrimarySmall": { "font": "sans-bold-12" },
	"nameSecondary": { "font": "sans-bold-16" }
};

var g_SpecificNamesPrimary = Engine.ConfigDB_GetValue("user", "gui.session.howtoshownames") == 0 || Engine.ConfigDB_GetValue("user", "gui.session.howtoshownames") == 2;
var g_ShowSecondaryNames = Engine.ConfigDB_GetValue("user", "gui.session.howtoshownames") == 0 || Engine.ConfigDB_GetValue("user", "gui.session.howtoshownames") == 1;

function initDisplayedNames()
{
	registerConfigChangeHandler(changes => {
		if (changes.has("gui.session.howtoshownames"))
			updateDisplayedNames();
	});
}

/**
 * String of four spaces to be used as indentation in gui strings.
 */
var g_Indent = "    ";

var g_DamageTypesMetadata = new DamageTypesMetadata();
var g_StatusEffectsMetadata = new StatusEffectsMetadata();

/**
 * If true, always shows whether the splash damage deals friendly fire.
 * Otherwise display the friendly fire tooltip only if it does.
 */
var g_AlwaysDisplayFriendlyFire = false;

function getCostTypes()
{
	return g_ResourceData.GetCodes().concat(["population", "time"]);
}

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

function getCurrentCaptureTooltip(entState, label)
{
	if (!entState.maxCapturePoints)
		return "";

	return sprintf(translate("%(captureLabel)s %(current)s / %(max)s"), {
		"captureLabel": headerFont(label || translate("Capture points:")),
		"current": Math.round(entState.capturePoints[entState.player]),
		"max": Math.round(entState.maxCapturePoints)
	});
}

/**
 * Converts an resistance level into the actual reduction percentage.
 */
function resistanceLevelToPercentageString(level)
{
	return sprintf(translate("%(percentage)s%%"), {
		"percentage": (100 - Math.round(Math.pow(0.9, level) * 100))
	});
}

function getResistanceTooltip(template)
{
	if (!template.resistance)
		return "";

	let details = [];
	if (template.resistance.Damage)
		details.push(getDamageResistanceTooltip(template.resistance.Damage));

	if (template.resistance.Capture)
		details.push(getCaptureResistanceTooltip(template.resistance.Capture));

	if (template.resistance.ApplyStatus)
		details.push(getStatusEffectsResistanceTooltip(template.resistance.ApplyStatus));

	return details.length ? sprintf(translate("%(label)s\n%(details)s"), {
		"label": headerFont(translate("Resistance:")),
		"details": g_Indent + details.join("\n" + g_Indent)
	}) : "";
}

function getDamageResistanceTooltip(resistanceTypeTemplate)
{
	if (!resistanceTypeTemplate)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Damage:")),
		"details":
			g_DamageTypesMetadata.sort(Object.keys(resistanceTypeTemplate)).map(
				dmgType => sprintf(translate("%(damage)s %(damageType)s %(resistancePercentage)s"), {
					"damage": resistanceTypeTemplate[dmgType].toFixed(1),
					"damageType": unitFont(translateWithContext("damage type", g_DamageTypesMetadata.getName(dmgType))),
					"resistancePercentage":
						'[font="sans-10"]' +
						sprintf(translate("(%(resistancePercentage)s)"), {
							"resistancePercentage": resistanceLevelToPercentageString(resistanceTypeTemplate[dmgType])
						}) + '[/font]'
				})
			).join(commaFont(translate(", ")))
	});
}

function getCaptureResistanceTooltip(resistanceTypeTemplate)
{
	if (!resistanceTypeTemplate)
		return "";
	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Capture:")),
		"details":
			sprintf(translate("%(damage)s %(damageType)s %(resistancePercentage)s"), {
				"damage": resistanceTypeTemplate.toFixed(1),
				"damageType": unitFont(translateWithContext("damage type", "Capture")),
				"resistancePercentage":
					'[font="sans-10"]' +
					sprintf(translate("(%(resistancePercentage)s)"), {
						"resistancePercentage": resistanceLevelToPercentageString(resistanceTypeTemplate)
					}) + '[/font]'
			})
	});
}

function getStatusEffectsResistanceTooltip(resistanceTypeTemplate)
{
	if (!resistanceTypeTemplate)
		return "";
	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Status Effects:")),
		"details":
			Object.keys(resistanceTypeTemplate).map(
				statusEffect => {
					if (resistanceTypeTemplate[statusEffect].blockChance == 1)
						return sprintf(translate("Blocks %(name)s"), {
							"name": unitFont(translateWithContext("status effect", g_StatusEffectsMetadata.getName(statusEffect)))
						});

					if (resistanceTypeTemplate[statusEffect].blockChance == 0)
						return sprintf(translate("%(name)s %(details)s"), {
							"name": unitFont(translateWithContext("status effect", g_StatusEffectsMetadata.getName(statusEffect))),
							"details": sprintf(translate("Duration reduction: %(durationReduction)s%%"), {
								"durationReduction": (100 - resistanceTypeTemplate[statusEffect].duration * 100)
							})
						});

					if (resistanceTypeTemplate[statusEffect].duration == 1)
						return sprintf(translate("%(name)s %(details)s"), {
							"name": unitFont(translateWithContext("status effect", g_StatusEffectsMetadata.getName(statusEffect))),
							"details": sprintf(translate("Blocks: %(blockPercentage)s%%"), {
								"blockPercentage": resistanceTypeTemplate[statusEffect].blockChance * 100
							})
						});

					return sprintf(translate("%(name)s %(details)s"), {
						"name": unitFont(translateWithContext("status effect", g_StatusEffectsMetadata.getName(statusEffect))),
						"details": sprintf(translate("Blocks: %(blockPercentage)s%%, Duration reduction: %(durationReduction)s%%"), {
							"blockPercentage": resistanceTypeTemplate[statusEffect].blockChance * 100,
							"durationReduction": (100 - resistanceTypeTemplate[statusEffect].duration * 100)
						})
					});
				}
			).join(commaFont(translate(", ")))
	});
}

function attackRateDetails(interval, projectiles)
{
	if (!interval)
		return "";

	if (projectiles === 0)
		return translate("Garrison to fire arrows");

	let attackRateString = getSecondsString(interval / 1000);
	let header = headerFont(translate("Interval:"));

	if (projectiles && +projectiles > 1)
	{
		header = headerFont(translate("Rate:"));
		let projectileString = sprintf(translatePlural("%(projectileCount)s %(projectileName)s", "%(projectileCount)s %(projectileName)s", projectiles), {
			"projectileCount": projectiles,
			"projectileName": unitFont(translatePlural("arrow", "arrows", projectiles))
		});

		attackRateString = sprintf(translate("%(projectileString)s / %(attackRateString)s"), {
			"projectileString": projectileString,
			"attackRateString": attackRateString
		});
	}

	return sprintf(translate("%(label)s %(details)s"), {
		"label": header,
		"details": attackRateString
	});
}

function rangeDetails(attackTypeTemplate)
{
	if (!attackTypeTemplate.maxRange)
		return "";

	let rangeTooltipString = {
		"relative": {
			// Translation: For example: Range: 2 to 10 (+2) meters
			"minRange": translate("%(rangeLabel)s %(minRange)s to %(maxRange)s (%(relativeRange)s) %(rangeUnit)s"),
			// Translation: For example: Range: 10 (+2) meters
			"no-minRange": translate("%(rangeLabel)s %(maxRange)s (%(relativeRange)s) %(rangeUnit)s"),
		},
		"non-relative": {
			// Translation: For example: Range: 2 to 10 meters
			"minRange": translate("%(rangeLabel)s %(minRange)s to %(maxRange)s %(rangeUnit)s"),
			// Translation: For example: Range: 10 meters
			"no-minRange": translate("%(rangeLabel)s %(maxRange)s %(rangeUnit)s"),
		}
	};

	let minRange = Math.round(attackTypeTemplate.minRange);
	let maxRange = Math.round(attackTypeTemplate.maxRange);
	let realRange = attackTypeTemplate.elevationAdaptedRange;
	let relativeRange = realRange ? Math.round(realRange - maxRange) : 0;

	return sprintf(rangeTooltipString[relativeRange ? "relative" : "non-relative"][minRange ? "minRange" : "no-minRange"], {
		"rangeLabel": headerFont(translate("Range:")),
		"minRange": minRange,
		"maxRange": maxRange,
		"relativeRange": relativeRange > 0 ? sprintf(translate("+%(number)s"), { "number": relativeRange }) : relativeRange,
		"rangeUnit":
			unitFont(minRange || relativeRange ?
				// Translation: For example "0.5 to 1 meters", "1 (+1) meters" or "1 to 2 (+3) meters"
				translate("meters") :
				translatePlural("meter", "meters", maxRange))
	});
}

function damageDetails(damageTemplate)
{
	if (!damageTemplate)
		return "";

	return g_DamageTypesMetadata.sort(Object.keys(damageTemplate).filter(dmgType => damageTemplate[dmgType])).map(
		dmgType => sprintf(translate("%(damage)s %(damageType)s"), {
			"damage": (+damageTemplate[dmgType]).toFixed(1),
			"damageType": unitFont(translateWithContext("damage type", g_DamageTypesMetadata.getName(dmgType)))
		})).join(commaFont(translate(", ")));
}

function captureDetails(captureTemplate)
{
	if (!captureTemplate)
		return "";

	return sprintf(translate("%(amount)s %(name)s"), {
		"amount": (+captureTemplate).toFixed(1),
		"name": unitFont(translateWithContext("damage type", "Capture"))
	});
}

function splashDetails(splashTemplate)
{
	let splashLabel = sprintf(headerFont(translate("%(splashShape)s Splash")), {
		"splashShape": translate(splashTemplate.shape)
	});
	let splashDamageTooltip = sprintf(translate("%(label)s: %(effects)s"), {
		"label": splashLabel,
		"effects": attackEffectsDetails(splashTemplate)
	});

	if (g_AlwaysDisplayFriendlyFire || splashTemplate.friendlyFire)
		splashDamageTooltip += commaFont(translate(", ")) + sprintf(translate("Friendly Fire: %(enabled)s"), {
			"enabled": splashTemplate.friendlyFire ? translate("Yes") : translate("No")
		});

	return splashDamageTooltip;
}

function applyStatusDetails(applyStatusTemplate)
{
	if (!applyStatusTemplate)
		return "";

	return sprintf(translate("gives %(name)s"), {
		"name": Object.keys(applyStatusTemplate).map(x =>
			unitFont(translateWithContext("status effect", g_StatusEffectsMetadata.getName(x)))
		).join(commaFont(translate(", "))),
	});
}

function attackEffectsDetails(attackTypeTemplate)
{
	if (!attackTypeTemplate)
		return "";

	let effects = [
		captureDetails(attackTypeTemplate.Capture || undefined),
		damageDetails(attackTypeTemplate.Damage || undefined),
		applyStatusDetails(attackTypeTemplate.ApplyStatus || undefined)
	];
	return effects.filter(effect => effect).join(commaFont(translate(", ")));
}

function getAttackTooltip(template)
{
	if (!template.attack)
		return "";

	let tooltips = [];
	for (let attackType in template.attack)
	{
		// Slaughter is used to kill animals, so do not show it.
		if (attackType == "Slaughter")
			continue;

		let attackTypeTemplate = template.attack[attackType];
		let attackLabel = sprintf(headerFont(translate("%(attackType)s")), {
			"attackType": translateWithContext(attackTypeTemplate.attackName.context || "Name of an attack, usually the weapon.", attackTypeTemplate.attackName.name)
		});

		let projectiles;
		// Use either current rate from simulation or default count if the sim is not running.
		// TODO: This ought to be extended to include units which fire multiple projectiles.
		if (template.buildingAI)
			projectiles = template.buildingAI.arrowCount || template.buildingAI.defaultArrowCount;

		let splashTemplate = attackTypeTemplate.splash;

		// Show the effects of status effects below.
		let statusEffectsDetails = [];
		if (attackTypeTemplate.ApplyStatus)
			for (let status in attackTypeTemplate.ApplyStatus)
				statusEffectsDetails.push("\n" + g_Indent + g_Indent + getStatusEffectsTooltip(status, attackTypeTemplate.ApplyStatus[status], true));
		statusEffectsDetails = statusEffectsDetails.join("");

		tooltips.push(sprintf(translate("%(attackLabel)s: %(effects)s, %(range)s, %(rate)s%(statusEffects)s%(splash)s"), {
			"attackLabel": attackLabel,
			"effects": attackEffectsDetails(attackTypeTemplate),
			"range": rangeDetails(attackTypeTemplate),
			"rate": attackRateDetails(attackTypeTemplate.repeatTime, projectiles),
			"splash": splashTemplate ? "\n" + g_Indent + g_Indent + splashDetails(splashTemplate) : "",
			"statusEffects": statusEffectsDetails
		}));
	}

	return sprintf(translate("%(label)s\n%(details)s"), {
		"label": headerFont(translate("Attack:")),
		"details": g_Indent + tooltips.join("\n" + g_Indent)
	});
}

/**
 * @param applier - if true, return the tooltip for the Applier. If false, Receiver is returned.
 */
function getStatusEffectsTooltip(statusCode, template, applier)
{
	let tooltipAttributes = [];
	let statusData = g_StatusEffectsMetadata.getData(statusCode);
	if (template.Damage || template.Capture)
		tooltipAttributes.push(attackEffectsDetails(template));

	if (template.Interval)
		tooltipAttributes.push(attackRateDetails(+template.Interval));

	if (template.Duration)
		tooltipAttributes.push(getStatusEffectDurationTooltip(template));

	if (applier && statusData.applierTooltip)
		tooltipAttributes.push(translateWithContext("status effect", statusData.applierTooltip));
	else if (!applier && statusData.receiverTooltip)
		tooltipAttributes.push(translateWithContext("status effect", statusData.receiverTooltip));

	if (applier)
		return sprintf(translate("%(statusName)s: %(statusInfo)s %(stackability)s"), {
			"statusName": headerFont(translateWithContext("status effect", statusData.statusName)),
			"statusInfo": tooltipAttributes.join(commaFont(translate(", "))),
			"stackability": getStatusEffectStackabilityTooltip(template)
		});
	return sprintf(translate("%(statusName)s: %(statusInfo)s"), {
		"statusName": headerFont(translateWithContext("status effect", statusData.statusName)),
		"statusInfo": tooltipAttributes.join(commaFont(translate(", ")))
	});
}

function getStatusEffectDurationTooltip(template)
{
	if (!template.Duration)
		return "";

	return sprintf(translate("%(durName)s: %(duration)s"), {
		"durName": headerFont(translate("Duration")),
		"duration": getSecondsString((template._timeElapsed ?
			+template.Duration - template._timeElapsed :
			+template.Duration) / 1000)
	});
}

function getStatusEffectStackabilityTooltip(template)
{
	if (!template.Stackability || template.Stackability == "Ignore")
		return "";

	let stackabilityString = "";
	if (template.Stackability === "Extend")
		stackabilityString = translateWithContext("status effect stackability", "(extends)");
	else if (template.Stackability === "Replace")
		stackabilityString = translateWithContext("status effect stackability", "(replaces)");
	else if (template.Stackability === "Stack")
		stackabilityString = translateWithContext("status effect stackability", "(stacks)");

	return sprintf(translate("%(stackability)s"), {
		"stackability": stackabilityString
	});
}

function getGarrisonTooltip(template)
{
	let tooltips = [];
	if (template.garrisonHolder)
	{
		tooltips.push (
			sprintf(translate("%(label)s: %(garrisonLimit)s"), {
				"label": headerFont(translate("Garrison Limit")),
				"garrisonLimit": template.garrisonHolder.capacity
			})
		);

		if (template.garrisonHolder.buffHeal)
			tooltips.push(
				sprintf(translate("%(healRateLabel)s %(value)s %(health)s / %(second)s"), {
					"healRateLabel": headerFont(translate("Heal:")),
					"value": Math.round(template.garrisonHolder.buffHeal),
					"health": unitFont(translateWithContext("garrison tooltip", "Health")),
					"second": unitFont(translate("second")),
				})
			);

		tooltips.join(commaFont(translate(", ")));
	}
	if (template.garrisonable)
	{
		let extraSize;
		if (template.garrisonHolder)
			extraSize = template.garrisonHolder.occupiedSlots;
		if (template.garrisonable.size > 1 || extraSize)
			tooltips.push (
				sprintf(translate("%(label)s: %(garrisonSize)s %(extraSize)s"), {
					"label": headerFont(translate("Garrison Size")),
					"garrisonSize": template.garrisonable.size,
					"extraSize": extraSize ?
						translateWithContext("nested garrison", "+ ") + extraSize : ""
				})
			);
	}

	return tooltips.join("\n");
}

function getTurretsTooltip(template)
{
	if (!template.turretHolder)
		return "";
	return sprintf(translate("%(label)s: %(turretsLimit)s"), {
		"label": headerFont(translate("Turret Positions")),
		"turretsLimit": Object.keys(template.turretHolder.turretPoints).length
	});
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
	let result = [];
	result.push(sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Number of repairers:")),
		"details": entState.repairable.numBuilders
	}));
	if (entState.repairable.numBuilders)
	{
		result.push(sprintf(translate("%(label)s %(details)s"), {
			"label": headerFont(translate("Remaining repair time:")),
			"details": getSecondsString(Math.floor(entState.repairable.buildTime.timeRemaining))
		}));
		let timeReduction = Math.round(entState.repairable.buildTime.timeRemaining - entState.repairable.buildTime.timeRemainingNew);
		result.push(sprintf(translatePlural(
			"Add another worker to speed up the repairs by %(second)s second.",
			"Add another worker to speed up the repairs by %(second)s seconds.",
			timeReduction),
		{
			"second": timeReduction
		}));
	}
	else
		result.push(sprintf(translatePlural(
			"Add a worker to finish the repairs in %(second)s second.",
			"Add a worker to finish the repairs in %(second)s seconds.",
			Math.round(entState.repairable.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.repairable.buildTime.timeRemainingNew)
		}));

	return result.join("\n");
}

function getBuildTimeTooltip(entState)
{
	let result = [];
	result.push(sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Number of builders:")),
		"details": entState.foundation.numBuilders
	}));
	if (entState.foundation.numBuilders)
	{
		result.push(sprintf(translate("%(label)s %(details)s"), {
			"label": headerFont(translate("Remaining build time:")),
			"details": getSecondsString(Math.floor(entState.foundation.buildTime.timeRemaining))
		}));
		let timeReduction = Math.round(entState.foundation.buildTime.timeRemaining - entState.foundation.buildTime.timeRemainingNew);
		result.push(sprintf(translatePlural(
			"Add another worker to speed up the construction by %(second)s second.",
			"Add another worker to speed up the construction by %(second)s seconds.",
			timeReduction),
		{
			"second": timeReduction
		}));
	}
	else
		result.push(sprintf(translatePlural(
			"Add a worker to finish the construction in %(second)s second.",
			"Add a worker to finish the construction in %(second)s seconds.",
			Math.round(entState.foundation.buildTime.timeRemainingNew)),
		{
			"second": Math.round(entState.foundation.buildTime.timeRemainingNew)
		}));

	return result.join("\n");
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
	if (!template.cost)
		return [];
	let totalCosts = multiplyEntityCosts(template, buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch);
	if (template.cost.time)
		totalCosts.time = Math.ceil(template.cost.time * (entity ? Engine.GuiInterfaceCall("GetBatchTime", {
			"entity": entity,
			"batchSize": buildingsCountToTrainFullBatch > 0 ? fullBatchSize : remainderBatch
		}) : 1));

	let costs = [];
	for (let type of getCostTypes())
		if (totalCosts[type])
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

	let rates = {};
	for (let resource of g_ResourceData.GetResources())
	{
		let types = [resource.code];
		for (let subtype in resource.subtypes)
		{
			// We ignore ruins as those are not that common
			if (subtype == "ruins")
				continue;

			let rate = template.resourceGatherRates[resource.code + "." + subtype];
			if (rate > 0)
				rates[resource.code + "_" + subtype] = rate;
		}
	}

	if (!Object.keys(rates).length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Gather Rates:")),
		"details":
			Object.keys(rates).map(
				type => sprintf(translate("%(resourceIcon)s %(rate)s"), {
					"resourceIcon": resourceIcon(type),
					"rate": rates[type].toFixed(2)
				})
			).join("  ")
	});
}

/**
 * Returns the resources this entity supplies in the specified entity's tooltip
 */
function getResourceSupplyTooltip(template)
{
	if (!template.supply)
		return "";

	let supply = template.supply;
	// Translation: Label in tooltip showing the resource type and quantity of a given resource supply.
	return sprintf(translate("%(label)s %(component)s %(amount)s"), {
		"label": headerFont(translate("Resource Supply:")),
		"component": resourceIcon(supply.type[0]),
		// Translation: Marks that a resource supply entity has an unending, infinite, supply of its resource.
		"amount": Number.isFinite(+supply.amount) ? supply.amount : translate("∞")
	});
}

/**
 * @param {Object} template - The entity's template.
 * @return {string} - The resources this entity rewards to a collecter.
 */
function getTreasureTooltip(template)
{
	if (!template.treasure)
		return "";

	let resources = {};
	for (let resource of g_ResourceData.GetResources())
	{
		let type = resource.code;
		if (template.treasure.resources[type])
			resources[type] = template.treasure.resources[type];
	}

	let resourceNames = Object.keys(resources);
	if (!resourceNames.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Reward:")),
		"details":
			resourceNames.map(
				type => sprintf(translate("%(resourceIcon)s %(reward)s"), {
					"resourceIcon": resourceIcon(type),
					"reward": resources[type]
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

function getUpkeepTooltip(template)
{
	if (!template.upkeep)
		return "";

	let resCodes = g_ResourceData.GetCodes().filter(res => !!template.upkeep.rates[res]);
	if (!resCodes.length)
		return "";

	return sprintf(translate("%(label)s %(details)s"), {
		"label": headerFont(translate("Upkeep:")),
		"details": sprintf(translate("%(resources)s / %(time)s"), {
			"resources":
				resCodes.map(
					res => sprintf(translate("%(resourceIcon)s %(rate)s"), {
						"resourceIcon": resourceIcon(res),
						"rate": template.upkeep.rates[res]
					})
				).join("  "),
			"time": getSecondsString(template.upkeep.interval / 1000)
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
function getEntityCostTooltip(template, player, entity, buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
{
	// Entities with a wallset component are proxies for initiating wall placement and as such do not have a cost of
	// their own; the individual wall pieces within it do.
	if (template.wallSet)
	{
		let templateLong = GetTemplateData(template.wallSet.templates.long, player);
		let templateMedium = GetTemplateData(template.wallSet.templates.medium, player);
		let templateShort = GetTemplateData(template.wallSet.templates.short, player);
		let templateTower = GetTemplateData(template.wallSet.templates.tower, player);

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
	if (!template.population || !template.population.bonus)
		return "";

	return sprintf(translate("%(label)s %(bonus)s"), {
		"label": headerFont(translate("Population Bonus:")),
		"bonus": template.population.bonus
	});
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
			"cost": Math.ceil(resources[resource])
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

	const walk = template.speed.walk.toFixed(1);
	const run = template.speed.run.toFixed(1);

	if (walk == 0 && run == 0)
		return "";

	const acceleration = template.speed.acceleration.toFixed(1);
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
			}) +
			commaFont(translate(", ")) +
			sprintf(translate("%(speed)s %(movementType)s"), {
				"speed": acceleration,
				"movementType": unitFont(translate("Acceleration"))
			})
	});
}

function getHealerTooltip(template)
{
	if (!template.heal)
		return "";

	let health = +(template.heal.health.toFixed(1));
	let range = +(template.heal.range.toFixed(0));
	let interval = +((template.heal.interval / 1000).toFixed(1));

	return [
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", health), {
			"label": headerFont(translate("Heal:")),
			"val": health,
			"unit": unitFont(translatePlural("Health", "Health", health))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", range), {
			"label": headerFont(translate("Range:")),
			"val": range,
			"unit": unitFont(translatePlural("meter", "meters", range))
		}),
		sprintf(translatePlural("%(label)s %(val)s %(unit)s", "%(label)s %(val)s %(unit)s", interval), {
			"label": headerFont(translate("Interval:")),
			"val": interval,
			"unit": unitFont(translatePlural("second", "seconds", interval))
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
				"auraname": getEntityNames(auras[auraID])
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

	let primaryName = g_SpecificNamesPrimary ? template.name.specific : template.name.generic;
	let secondaryName;
	if (g_ShowSecondaryNames)
		secondaryName = g_SpecificNamesPrimary ? template.name.generic : template.name.specific;

	if (secondaryName)
		return sprintf(translate("%(primaryName)s (%(secondaryName)s)"), {
			"primaryName": primaryName,
			"secondaryName": secondaryName
		});
	return sprintf(translate("%(primaryName)s"), {
		"primaryName": primaryName
	});

}

function getEntityNamesFormatted(template)
{
	if (!template.name.specific)
		return setStringTags(template.name.generic, g_TooltipTextFormats.namePrimaryBig);

	let primaryName = g_SpecificNamesPrimary ? template.name.specific : template.name.generic;
	let secondaryName;
	if (g_ShowSecondaryNames)
		secondaryName = g_SpecificNamesPrimary ? template.name.generic : template.name.specific;

	if (!secondaryName || primaryName == secondaryName)
		return sprintf(translate("%(primaryName)s"), {
			"primaryName":
			setStringTags(primaryName[0], g_TooltipTextFormats.namePrimaryBig) +
			setStringTags(primaryName.slice(1).toUpperCase(), g_TooltipTextFormats.namePrimarySmall)
		});

	// Translation: Example: "Epibátēs Athēnaîos [font="sans-bold-16"](Athenian Marine)[/font]"
	return sprintf(translate("%(primaryName)s (%(secondaryName)s)"), {
		"primaryName":
			setStringTags(primaryName[0], g_TooltipTextFormats.namePrimaryBig) +
			setStringTags(primaryName.slice(1).toUpperCase(), g_TooltipTextFormats.namePrimarySmall),
		"secondaryName": setStringTags(secondaryName, g_TooltipTextFormats.nameSecondary)
	});
}

function getEntityPrimaryNameFormatted(template)
{
	let primaryName = g_SpecificNamesPrimary ? template.name.specific : template.name.generic;
	if (!primaryName)
		return setStringTags(g_SpecificNamesPrimary ? template.name.generic : template.name.specific, g_TooltipTextFormats.namePrimaryBig);

	return setStringTags(primaryName[0], g_TooltipTextFormats.namePrimaryBig) +
		setStringTags(primaryName.slice(1).toUpperCase(), g_TooltipTextFormats.namePrimarySmall);
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

function getResourceDropsiteTooltip(template)
{
	if (!template || !template.resourceDropsite || !template.resourceDropsite.types)
		return "";

	return sprintf(translate("%(label)s %(icons)s"), {
		"label": headerFont(translate("Dropsite for:")),
		"icons": template.resourceDropsite.types.map(type => resourceIcon(type)).join("  ")
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

/**
 * @param {number} number - A number to shorten using SI prefix.
 */
function abbreviateLargeNumbers(number)
{
     if (number >= 1e6)
        return Math.floor(number / 1e6) + translateWithContext("One letter abbreviation for million", 'M');

     if (number >= 1e5)
        return Math.floor(number / 1e3) + translateWithContext("One letter abbreviation for thousand", 'k');

     if (number >= 1e4)
        return (number / 1e3).toFixed(1).replace(/\.0$/, '') + translateWithContext("One letter abbreviation for thousand", 'k');

     return number;
}

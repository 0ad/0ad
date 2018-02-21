/**
 * GUI limits. Populated if needed by a predraw() function.
 */
var g_DrawLimits = {};

/**
 * List of functions that get the statistics of any template or entity,
 * formatted in such a way as to appear in a tooltip.
 *
 * The functions listed are defined in gui/common/tooltips.js
 */
var g_StatsFunctions = [
	getHealthTooltip,
	getHealerTooltip,
	getAttackTooltip,
	getSplashDamageTooltip,
	getArmorTooltip,
	getGarrisonTooltip,
	getProjectilesTooltip,
	getSpeedTooltip,
	getGatherTooltip,
	getResourceSupplyTooltip,
	getPopulationBonusTooltip,
	getResourceTrickleTooltip,
	getLootTooltip
];

/**
 * Concatanates the return values of the array of passed functions.
 *
 * @param {object} template
 * @param {array} textFunctions
 * @param {string} joiner
 * @return {string} The built text.
 */
function buildText(template, textFunctions=[], joiner="\n")
{
	return textFunctions.map(func => func(template)).filter(tip => tip).join(joiner);
}

/**
 * Creates text in the following format:
 *  Header: value1, value2, ..., valueN
 */
function buildListText(headerString, arrayOfValues)
{
	// Translation: Label followed by a list of values.
	return sprintf(translate("%(listHeader)s %(listOfValues)s"), {
		"listHeader": headerFont(headerString),
		// Translation: List separator.
		"listOfValues": bodyFont(arrayOfValues.join(translate(", ")))
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
	let type = supply.type[0] == "treasure" ? supply.type[1] : supply.type[0];

	// Translation: Label in tooltip showing the resource type and quantity of a given resource supply.
	return sprintf(translate("%(label)s %(component)s %(amount)s"), {
		"label": headerFont(translate("Resource Supply:")),
		"component": resourceIcon(type),
		// Translation: Marks that a resource supply entity has an unending, infinite, supply of its resource.
		"amount": Number.isFinite(+supply.amount) ? supply.amount : translate("∞")
	});
}

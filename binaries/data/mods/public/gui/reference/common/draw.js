/**
 * GUI limits. Populated if needed by a predraw() function.
 */
var g_DrawLimits = {};

/**
 * These functions are defined in gui/common/tooltips.js
 */
var g_TooltipFunctions = [
	getEntityNamesFormatted,
	getEntityCostTooltip,
	getEntityTooltip,
	getAurasTooltip,
	getHealthTooltip,
	getHealerTooltip,
	getAttackTooltip,
	getSplashDamageTooltip,
	getArmorTooltip,
	getGarrisonTooltip,
	getProjectilesTooltip,
	getSpeedTooltip,
	getGatherTooltip,
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

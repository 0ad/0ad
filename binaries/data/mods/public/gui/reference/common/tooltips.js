/**
 * Creates text in the following format:
 *  Header: value1, value2, ..., valueN
 *
 * This function is only used below, nowhere else.
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
 * The following functions in this file work on the same basis as those in gui/common/tooltips.js
 *
 * Note: Due to quirks in loading order, this file might not be loaded before ReferencePage.js.
 *       Do not put anything in here that you wish to access static'ly there.
 */

function getBuiltByText(template)
{
	// Translation: Label before a list of the names of units that build the structure selected.
	return template.builtByListOfNames ? buildListText(translate("Built by:"), template.builtByListOfNames) : "";
}

function getTrainedByText(template)
{
	// Translation: Label before a list of the names of structures or units that train the unit selected.
	return template.trainedByListOfNames ? buildListText(translate("Trained by:"), template.trainedByListOfNames) : "";
}

function getResearchedByText(template)
{
	// Translation: Label before a list of names of structures or units that research the technology selected.
	return template.researchedByListOfNames ? buildListText(translate("Researched at:"), template.researchedByListOfNames) : "";
}

/**
 * @return {string} List of the names of the buildings the selected unit can build.
 */
function getBuildText(template)
{
	// Translation: Label before a list of the names of structures the selected unit can construct or build.
	return template.buildListOfNames ? buildListText(translate("Builds:"), template.buildListOfNames) : "";
}

/**
 * @return {string} List of the names of the technologies the selected structure/unit can research.
 */
function getResearchText(template)
{
	// Translation: Label before a list of the names of technologies the selected unit or structure can research.
	return template.researchListOfNames ? buildListText(translate("Researches:"), template.researchListOfNames) : "";
}

/**
 * @return {string} List of the names of the units the selected unit can train.
 */
function getTrainText(template)
{
	// Translation: Label before a list of the names of units the selected unit or structure can train.
	return template.trainListOfNames ? buildListText(translate("Trains:"), template.trainListOfNames) : "";
}

/**
 * @return {string} List of the names of the buildings/units the selected structure/unit can upgrade to.
 */
function getUpgradeText(template)
{
	// Translation: Label before a list of the names of units or structures the selected unit or structure can be upgradable to.
	return template.upgradeListOfNames ? buildListText(translate("Upgradable to:"), template.upgradeListOfNames) : "";
}

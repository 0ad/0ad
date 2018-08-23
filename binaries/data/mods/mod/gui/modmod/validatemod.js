const g_ModProperties = {
	// example: "0ad"
	"name": {
		"required": true,
		"type": "string",
		"validate": validateName
	},
	// example: "0.0.23"
	"version": {
		"required": true,
		"type": "string",
		"validate": validateVersion
	},
	// example: ["0ad<=0.0.16", "rote"]
	"dependencies": {
		"required": true,
		"type": "object",
		"validate": validateDependencies
	},
	// example: "0 A.D. - Empires Ascendant"
	"label": {
		"require": true,
		"type": "string",
		"validate": validateLabel
	},
	// example: "A free, open-source, historical RTS game."
	"description": {
		"required": true,
		"type": "string"
	},
	// example: "https://wildfiregames.com/"
	"url": {
		"required": false,
		"type": "string"
	}
};

/**
 * Tests if the string only contains alphanumeric characters and _ -
 */
const g_RegExpName = /[a-zA-Z0-9\-\_]+/;

/**
 * Tests if the version string consists only of numbers and at most two periods.
 */
const g_RegExpVersion= /[0-9]+(\.[0-9]+){0,2}/;

/**
 * Version checks in mod dependencies can use these operators.
 */
const g_RegExpComparisonOperator = /(<=|>=|<|>|=)/;

/**
 * Tests if a dependency compares a mod version against another, for instance "0ad<=0.0.16".
 */
const g_RegExpComparison = globalRegExp(new RegExp(g_RegExpName.source + g_RegExpComparisonOperator.source + g_RegExpVersion.source));

/**
 * The label may not be empty.
 */
const g_RegExpLabel = /.*\S.*/;

function globalRegExp(regexp)
{
	return new RegExp("^" + regexp.source + "$");
}

/**
 * Returns whether the mod defines all required properties and whether all properties are valid.
 * Shows a notification if not.
 */
function validateMod(folder, modData, notify)
{
	let valid = true;

	for (let propertyName in g_ModProperties)
	{
		let property = g_ModProperties[propertyName];

		if (modData[propertyName] === undefined)
		{
			if (!property.required)
				continue;

			if (notify)
				warn("Mod '" + folder + "' does not define '" + propertyName + "'!");

			valid = false;
		}

		if (typeof modData[propertyName] != property.type)
		{
			if (notify)
				warn(propertyName + " in mod '" + folder + "' is not of the type '" + property.type + "'!");

			valid = false;

			continue;
		}

		if (property.validate && !property.validate(folder, modData, notify))
			valid = false;
	}

	return valid;
}

function validateName(folder, modData, notify)
{
	let valid = modData.name.match(globalRegExp(g_RegExpName));

	if (!valid && notify)
		warn("mod name of " + folder + " may only contain alphanumeric characters, but found '" + modData.name + "'!");

	return valid;
}

function validateVersion(folder, modData, notify)
{
	let valid = modData.version.match(globalRegExp(g_RegExpVersion));

	if (!valid && notify)
		warn("mod version of " + folder + " may only contain numbers and at most 2 periods, but found '" + modData.version + "'!");

	return valid;
}

function validateDependencies(folder, modData, notify)
{
	let valid = true;

	for (let dependency of modData.dependencies)
	{
		valid = valid && (
			dependency.match(globalRegExp(g_RegExpName)) ||
			dependency.match(globalRegExp(g_RegExpComparison)));

		if (!valid && notify)
			warn("mod folder " + folder + " requires an invalid dependency '" + dependency + "'!");
	}

	return valid;
}

function validateLabel(folder, modData, notify)
{
	let valid = modData.label.match(g_RegExpLabel);

	if (!valid && notify)
		warn("mod label of " + folder + " may not be empty!");

	return valid;
}

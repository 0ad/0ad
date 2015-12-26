const localisedResourceNames = {
	"firstWord": {
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"food": translateWithContext("firstWord", "Food"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"meat": translateWithContext("firstWord", "Meat"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"metal": translateWithContext("firstWord", "Metal"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"ore": translateWithContext("firstWord", "Ore"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"rock": translateWithContext("firstWord", "Rock"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"ruins": translateWithContext("firstWord", "Ruins"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"stone": translateWithContext("firstWord", "Stone"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"treasure": translateWithContext("firstWord", "Treasure"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"tree": translateWithContext("firstWord", "Tree"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"wood": translateWithContext("firstWord", "Wood"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"fruit": translateWithContext("firstWord", "Fruit"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"grain": translateWithContext("firstWord", "Grain"),
		// Translation: Word as used at the beginning of a sentence or as a single-word sentence.
		"fish": translateWithContext("firstWord", "Fish"),
	},
	"withinSentence": {
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"food": translateWithContext("withinSentence", "Food"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"meat": translateWithContext("withinSentence", "Meat"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"metal": translateWithContext("withinSentence", "Metal"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"ore": translateWithContext("withinSentence", "Ore"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"rock": translateWithContext("withinSentence", "Rock"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"ruins": translateWithContext("withinSentence", "Ruins"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"stone": translateWithContext("withinSentence", "Stone"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"treasure": translateWithContext("withinSentence", "Treasure"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"tree": translateWithContext("withinSentence", "Tree"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"wood": translateWithContext("withinSentence", "Wood"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"fruit": translateWithContext("withinSentence", "Fruit"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"grain": translateWithContext("withinSentence", "Grain"),
		// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
		"fish": translateWithContext("withinSentence", "Fish"),
	}
};

function getLocalizedResourceName(resourceCode, context)
{
	if (!localisedResourceNames[context])
	{
		warn("Internationalization: Unexpected context for resource type localization found: ‘" + context + "’. This context is not supported.");
		return resourceCode;
	}
	if (!localisedResourceNames[context][resourceCode])
	{
		warn("Internationalization: Unexpected resource type found with code ‘" + resourceCode + ". This resource type must be internationalized.");
		return resourceCode;
	}
	return localisedResourceNames[context][resourceCode];
}

/**
 * Format resource amounts to proper english and translate (for example: "200 food, 100 wood, and 300 metal").
 */
function getLocalizedResourceAmounts(resources)
{
	let amounts = Object.keys(resources)
		.filter(type => resources[type] > 0)
		.map(type => sprintf(translate("%(amount)s %(resourceType)s"), {
			"amount": resources[type],
			"resourceType": getLocalizedResourceName(type, "withinSentence")
		}));

	if (amounts.length > 1)
	{
		let lastAmount = amounts.pop();
		amounts = sprintf(translate("%(previousAmounts)s and %(lastAmount)s"), {
			// Translation: This comma is used for separating first to penultimate elements in an enumeration.
			"previousAmounts": amounts.join(translate(", ")),
			"lastAmount": lastAmount
		});
	}

	return amounts;
}

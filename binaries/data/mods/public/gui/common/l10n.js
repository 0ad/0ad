function getLocalizedResourceName(resourceName, context)
{
	return translateWithContext(context, resourceName);
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
			"resourceType": getLocalizedResourceName(g_ResourceData.GetResource(type).name, "withinSentence")
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

/**
 * Format resource amounts to proper english and translate (for example: "200 food, 100 wood, and 300 metal").
 */
function getLocalizedResourceAmounts(resources)
{
	let amounts = g_ResourceData.GetCodes()
		.filter(type => !!resources[type])
		.map(type => sprintf(translate("%(amount)s %(resourceType)s"), {
			"amount": resources[type],
			"resourceType": resourceNameWithinSentence(type)
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

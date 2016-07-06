const BARTER_RESOURCE_AMOUNT_TO_SELL = 100;
const BARTER_BUNCH_MULTIPLIER = 5;
const BARTER_RESOURCES = ["food", "wood", "stone", "metal"];
const BARTER_ACTIONS = ["Sell", "Buy"];
const GATE_ACTIONS = ["lock", "unlock"];

// upgrade constants
const UPGRADING_NOT_STARTED = -2;
const UPGRADING_CHOSEN_OTHER = -1;

// ==============================================
// BARTER HELPERS
// Resources to sell on barter panel
var g_BarterSell = "food";

function canMoveSelectionIntoFormation(formationTemplate)
{
	if (!(formationTemplate in g_canMoveIntoFormation))
	{
		g_canMoveIntoFormation[formationTemplate] = Engine.GuiInterfaceCall("CanMoveEntsIntoFormation", {
			"ents": g_Selection.toList(),
			"formationTemplate": formationTemplate
		});
	}
	return g_canMoveIntoFormation[formationTemplate];
}

function getStanceDisplayName(name)
{
	switch (name)
	{
	case "violent":
		return translateWithContext("stance", "Violent");
	case "aggressive":
		return translateWithContext("stance", "Aggressive");
	case "defensive":
		return translateWithContext("stance", "Defensive");
	case "passive":
		return translateWithContext("stance", "Passive");
	case "standground":
		return translateWithContext("stance", "Standground");
	default:
		warn("Internationalization: Unexpected stance found: " + name);
		return name;
	}
}

function getStanceTooltip(name)
{
	switch (name)
	{
	case "violent":
		return translateWithContext("stance", "Attack nearby opponents, focus on attackers and chase while visible");
	case "aggressive":
		return translateWithContext("stance", "Attack nearby opponents");
	case "defensive":
		return translateWithContext("stance", "Attack nearby opponents, chase a short distance and return to the original location");
	case "passive":
		return translateWithContext("stance", "Flee if attacked");
	case "standground":
		return translateWithContext("stance", "Attack opponents in range, but don't move");
	default:
		return "";
	}
}

/**
 * Format entity count/limit message for the tooltip
 */
function formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers)
{
	if (trainEntLimit == undefined)
		return "";

	var text = "\n" + sprintf(translate("Current Count: %(count)s, Limit: %(limit)s."), {
		"count": trainEntCount,
		"limit": trainEntLimit
	});

	if (trainEntCount >= trainEntLimit)
		text = "[color=\"red\"]" + text + "[/color]";

	for (var c in trainEntLimitChangers)
	{
		if (!trainEntLimitChangers[c])
			continue;

		let string = trainEntLimitChangers[c] > 0 ?
			translate("%(changer)s enlarges the limit with %(change)s.") :
			translate("%(changer)s lessens the limit with %(change)s.");

		text += "\n" + sprintf(string, {
			"changer": translate(c),
			"change": trainEntLimitChangers[c]
		});
	}
	return text;
}

/**
 * Format batch training string for the tooltip
 * Examples:
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 5"
 * buildingsCountToTrainFullBatch = 2, fullBatchSize = 5, remainderBatch = 0:
 * "Shift-click to train 10 (2*5)"
 * buildingsCountToTrainFullBatch = 1, fullBatchSize = 15, remainderBatch = 12:
 * "Shift-click to train 27 (15 + 12)"
 */
function formatBatchTrainingString(buildingsCountToTrainFullBatch, fullBatchSize, remainderBatch)
{
	var totalBatchTrainingCount = buildingsCountToTrainFullBatch * fullBatchSize + remainderBatch;

	// Don't show the batch training tooltip if either units of this type can't be trained at all
	// or only one unit can be trained
	if (totalBatchTrainingCount < 2)
		return "";

	var fullBatchesString = "";
	if (buildingsCountToTrainFullBatch > 0)
	{
		if (buildingsCountToTrainFullBatch > 1)
			fullBatchesString = sprintf(translate("%(buildings)s*%(batchSize)s"), {
				"buildings": buildingsCountToTrainFullBatch,
				"batchSize": fullBatchSize
			});
		else
			fullBatchesString = fullBatchSize;
	}

	// We need to display the batch details part if there is either more than
	// one building with full batch or one building with the full batch and
	// another with a partial batch
	let batchString;
	if (buildingsCountToTrainFullBatch > 1 ||
	    buildingsCountToTrainFullBatch == 1 && remainderBatch > 0)
	{
		if (remainderBatch > 0)
			batchString = translate("%(action)s to train %(number)s (%(fullBatch)s + %(remainderBatch)s).");
		else
			batchString = translate("%(action)s to train %(number)s (%(fullBatch)s).");
	}
	else
		batchString = translate("%(action)s to train %(number)s.");

	return "[font=\"sans-13\"]" + sprintf(batchString, {
		"action": "[font=\"sans-bold-13\"]" + translate("Shift-click") + "[/font]",
		"number": totalBatchTrainingCount,
		"fullBatch": fullBatchesString,
		"remainderBatch": remainderBatch
	}) + "[/font]";
}



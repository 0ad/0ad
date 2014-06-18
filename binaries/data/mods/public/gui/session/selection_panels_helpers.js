// Constants
// Barter constants
const BARTER_RESOURCE_AMOUNT_TO_SELL = 100;
const BARTER_BUNCH_MULTIPLIER = 5;
const BARTER_RESOURCES = ["food", "wood", "stone", "metal"];
const BARTER_ACTIONS = ["Sell", "Buy"];

// Gate constants
const GATE_ACTIONS = ["lock", "unlock"];

// ==============================================
// BARTER HELPERS
// Resources to sell on barter panel
var g_barterSell = "food";

// FORMATION HELPERS
// Check if the selection can move into formation, and cache the result
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

// ==============================================
// STANCE HELPERS

function getStanceDisplayName(name)
{
	var displayName;
	switch(name)
	{
		case "violent":
			displayName = translateWithContext("stance", "Violent");
			break;
		case "aggressive":
			displayName = translateWithContext("stance", "Aggressive");
			break;
		case "passive":
			displayName = translateWithContext("stance", "Passive");
			break;
		case "defensive":
			displayName = translateWithContext("stance", "Defensive");
			break;
		case "standground":
			displayName = translateWithContext("stance", "Standground");
			break;
		default:
			warn(sprintf("Internationalization: Unexpected stance found with code ‘%(stance)s’. This stance must be internationalized.", { stance: name }));
			displayName = name;
			break;
	}
	return displayName;
}

// ==============================================
// TRAINING / CONSTRUCTION HELPERS
/**
 * Format entity count/limit message for the tooltip
 */
function formatLimitString(trainEntLimit, trainEntCount, trainEntLimitChangers)
{
	if (trainEntLimit == undefined)
		return "";
	var text = "\n\n" + sprintf(translate("Current Count: %(count)s, Limit: %(limit)s."), { count: trainEntCount, limit: trainEntLimit });
	if (trainEntCount >= trainEntLimit)
		text = "[color=\"red\"]" + text + "[/color]";
	for (var c in trainEntLimitChangers)
	{
		if (trainEntLimitChangers[c] > 0)
			text += "\n" + sprintf(translate("%(changer)s enlarges the limit with %(change)s."), { changer: translate(c), change: trainEntLimitChangers[c] });
		else if (trainEntLimitChangers[c] < 0)
			text += "\n" + sprintf(translate("%(changer)s lessens the limit with %(change)s."), { changer: translate(c), change: (-trainEntLimitChangers[c]) });
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
	var batchTrainingString = "";
	var fullBatchesString = "";
	if (buildingsCountToTrainFullBatch > 0)
	{
		if (buildingsCountToTrainFullBatch > 1)
			fullBatchesString = sprintf(translate("%(buildings)s*%(batchSize)s"), {
				buildings: buildingsCountToTrainFullBatch,
				batchSize: fullBatchSize
			});
		else
			fullBatchesString = fullBatchSize;
	}
	var remainderBatchString = remainderBatch > 0 ? remainderBatch : "";
	var batchDetailsString = "";
	var action = "[font=\"sans-bold-13\"]" + translate("Shift-click") + "[/font][font=\"sans-13\"]"

	// We need to display the batch details part if there is either more than
	// one building with full batch or one building with the full batch and
	// another with a partial batch
	if (buildingsCountToTrainFullBatch > 1 ||
		(buildingsCountToTrainFullBatch == 1 && remainderBatch > 0))
	{
		if (remainderBatch > 0)
			return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s + %(remainderBatch)s)."), {
				action: action,
				number: totalBatchTrainingCount,
				fullBatch: fullBatchesString,
				remainderBatch: remainderBatch
			}) + "[/font]";

		return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s (%(fullBatch)s)."), {
			action: action,
			number: totalBatchTrainingCount,
			fullBatch: fullBatchesString
		}) + "[/font]";
	}

	return "\n[font=\"sans-13\"]" + sprintf(translate("%(action)s to train %(number)s."), {
		action: action,
		number: totalBatchTrainingCount
	}) + "[/font]";
}



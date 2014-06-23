if (!g_VictoryConditions)
	var g_VictoryConditions = {};

var vc = {};

vc.name = translate("Conquest");
vc.description = translate("Defeat all opponents");

/*
NOT SUPPORTED YET
vc.requirementsCheck = function(mapSettings)
{
	// nothing required
	return true;
};
*/

vc.scripts = ["scripts/Conquest.js"];

g_VictoryConditions.conquest = vc;
vc = null;

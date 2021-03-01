let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
let playerEnt = cmpPlayerManager.GetPlayerByID('1');
let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
cmpModifiersManager.AddModifiers("cheat/fastactions", {
	"Cost/BuildTime": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
	"ResourceGatherer/BaseSpeed": [{ "affects": [["Structure"], ["Unit"]], "multiply": 1000 }],
	"Pack/Time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
	"Upgrade/Time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
	"ProductionQueue/TechCostMultiplier/time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }]
}, playerEnt);

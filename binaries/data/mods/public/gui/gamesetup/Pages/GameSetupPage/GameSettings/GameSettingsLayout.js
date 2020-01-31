/**
 * This array determines the order in which the GUI controls are shown in the GameSettingTabs panel.
 * The names correspond to property names of the GameSettingControls prototype.
 */
var g_GameSettingsLayout = [
	{
		"label": translateWithContext("Match settings tab name", "Map"),
		"settings": [
			"MapType",
			"MapFilter",
			"MapSelection",
			"MapSize",
			"TeamPlacement",
			"Landscape",
			"Biome",
			"SeaLevelRiseTime",
			"Daytime",
			"TriggerDifficulty",
			"Nomad",
			"Treasures",
			"ExploredMap",
			"RevealedMap"
		]
	},
	{
		"label": translateWithContext("Match settings tab name", "Player"),
		"settings": [
			"PlayerCount",
			"PopulationCap",
			"StartingResources",
			"Spies",
			"Cheats"
		]
	},
	{
		"label": translateWithContext("Match settings tab name", "Game Type"),
		"settings": [
			...g_VictoryConditions.map(victoryCondition => victoryCondition.Name),
			"RelicCount",
			"RelicDuration",
			"WonderDuration",
			"RegicideGarrison",
			"GameSpeed",
			"Ceasefire",
			"LockedTeams",
			"LastManStanding",
			"Rating"
		]
	}
];

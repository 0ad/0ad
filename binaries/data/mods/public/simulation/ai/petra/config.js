// These integers must be sequential
PETRA.DIFFICULTY_SANDBOX = 0;
PETRA.DIFFICULTY_VERY_EASY = 1;
PETRA.DIFFICULTY_EASY = 2;
PETRA.DIFFICULTY_MEDIUM = 3;
PETRA.DIFFICULTY_HARD = 4;
PETRA.DIFFICULTY_VERY_HARD = 5;

PETRA.Config = function(difficulty = PETRA.DIFFICULTY_MEDIUM, behavior)
{
	this.difficulty = difficulty;

	// for instance "balanced", "aggressive" or "defensive"
	this.behavior = behavior || "random";

	// debug level: 0=none, 1=sanity checks, 2=debug, 3=detailed debug, -100=serializatio debug
	this.debug = 0;

	this.chat = true;	// false to prevent AI's chats

	this.popScaling = 1;	// scale factor depending on the max population

	this.Military = {
		"towerLapseTime": 90,	// Time to wait between building 2 towers
		"fortressLapseTime": 390,	// Time to wait between building 2 fortresses
		"popForBarracks1": 25,
		"popForBarracks2": 95,
		"popForForge": 65,
		"numSentryTowers": 1
	};

	this.DamageTypeImportance = {
		"Hack": 0.085,
		"Pierce": 0.075,
		"Crush": 0.065,
		"Fire": 0.095
	};

	this.Economy = {
		"popPhase2": 38,	// How many units we want before aging to phase2.
		"workPhase3": 65,	// How many workers we want before aging to phase3.
		"workPhase4": 80,	// How many workers we want before aging to phase4 or higher.
		"popForDock": 25,
		"targetNumWorkers": 40,	// dummy, will be changed later
		"targetNumTraders": 5,	// Target number of traders
		"targetNumFishers": 1,	// Target number of fishers per sea
		"supportRatio": 0.35,	// fraction of support workers among the workforce
		"provisionFields": 2
	};

	// Note: attack settings are set directly in attack_plan.js
	// defense
	this.Defense =
	{
		"defenseRatio": { "ally": 1.4, "neutral": 1.8, "own": 2 },	// ratio of defenders/attackers.
		"armyCompactSize": 2000,	// squared. Half-diameter of an army.
		"armyBreakawaySize": 3500,	// squared.
		"armyMergeSize": 1400	// squared.
	};

	// Additional buildings that the AI does not yet know when to build
	// and that it will try to build on phase 3 when enough resources.
	this.buildings =
	{
		"default": [],
		"athen": [
			"structures/{civ}/gymnasium",
			"structures/{civ}/prytaneion",
			"structures/{civ}/theater"
		],
		"brit": [],
		"cart": [
			"structures/{civ}/embassy_celtic",
			"structures/{civ}/embassy_iberian",
			"structures/{civ}/embassy_italic"
		],
		"gaul": [
			"structures/{civ}/assembly"
		],
		"han": [
			"structures/{civ}/academy"
		],
		"iber": [
			"structures/{civ}/monument"
		],
		"kush": [
			"structures/{civ}/camp_blemmye",
			"structures/{civ}/camp_noba",
			"structures/{civ}/pyramid_large",
			"structures/{civ}/pyramid_small",
			"structures/{civ}/temple_amun"
		],
		"mace": [
			"structures/{civ}/theater"
		],
		"maur": [
			"structures/{civ}/palace",
			"structures/{civ}/pillar_ashoka"
		],
		"pers": [
			"structures/{civ}/apadana"
		],
		"ptol": [
			"structures/{civ}/library",
			"structures/{civ}/theater"
		],
		"rome": [
			"structures/{civ}/army_camp",
			"structures/{civ}/temple_vesta"
		],
		"sele": [
			"structures/{civ}/theater"
		],
		"spart": [
			"structures/{civ}/syssiton",
			"structures/{civ}/theater"
		]
	};

	this.priorities =
	{
		"villager": 30,      // should be slightly lower than the citizen soldier one to not get all the food
		"citizenSoldier": 60,
		"trader": 50,
		"healer": 20,
		"ships": 70,
		"house": 350,
		"dropsites": 200,
		"field": 400,
		"dock": 90,
		"corral": 100,
		"economicBuilding": 90,
		"militaryBuilding": 130,
		"defenseBuilding": 70,
		"civilCentre": 950,
		"majorTech": 700,
		"minorTech": 250,
		"wonder": 1000,
		"emergency": 1000    // used only in emergency situations, should be the highest one
	};

	// Default personality (will be updated in setConfig)
	this.personality =
	{
		"aggressive": 0.5,
		"cooperative": 0.5,
		"defensive": 0.5
	};

	// See PETRA.QueueManager.prototype.wantedGatherRates()
	this.queues =
	{
		"firstTurn": {
			"food": 10,
			"wood": 10,
			"default": 0
		},
		"short": {
			"food": 200,
			"wood": 200,
			"default": 100
		},
		"medium": {
			"default": 0
		},
		"long": {
			"default": 0
		}
	};

	this.garrisonHealthLevel = { "low": 0.4, "medium": 0.55, "high": 0.7 };

	this.unusedNoAllyTechs = [
		"Player/sharedLos",
		"Market/InternationalBonus",
		"Player/sharedDropsites"
	];

	this.criticalPopulationFactors = [
		0.8,
		0.8,
		0.7,
		0.6,
		0.5,
		0.35
	];

	this.criticalStructureFactors = [
		0.8,
		0.8,
		0.7,
		0.6,
		0.5,
		0.35
	];

	this.criticalRootFactors = [
		0.8,
		0.8,
		0.67,
		0.5,
		0.35,
		0.2
	];
};

PETRA.Config.prototype.setConfig = function(gameState)
{
	if (this.difficulty > PETRA.DIFFICULTY_SANDBOX)
	{
		// Setup personality traits according to the user choice:
		// The parameter used to define the personality is basically the aggressivity or (1-defensiveness)
		// as they are anticorrelated, although some small smearing to decorelate them will be added.
		// And for each user choice, this parameter can vary between min and max
		let personalityList = {
			"random": { "min": 0, "max": 1 },
			"defensive": { "min": 0, "max": 0.27 },
			"balanced": { "min": 0.37, "max": 0.63 },
			"aggressive": { "min": 0.73, "max": 1 }
		};
		let behavior = randFloat(-0.5, 0.5);
		// make agressive and defensive quite anticorrelated (aggressive ~ 1 - defensive) but not completelety
		let variation = 0.15 * randFloat(-1, 1) * Math.sqrt(Math.square(0.5) - Math.square(behavior));
		let aggressive = Math.max(Math.min(behavior + variation, 0.5), -0.5) + 0.5;
		let defensive = Math.max(Math.min(-behavior + variation, 0.5), -0.5) + 0.5;
		let min = personalityList[this.behavior].min;
		let max = personalityList[this.behavior].max;
		this.personality = {
			"aggressive": min + aggressive * (max - min),
			"defensive": 1 - max + defensive * (max - min),
			"cooperative": randFloat(0, 1)
		};
	}
	// Petra usually uses the continuous values of personality.aggressive and personality.defensive
	// to define its behavior according to personality. But when discontinuous behavior is needed,
	// it uses the following personalityCut which should be set such that:
	// behavior="aggressive" => personality.aggressive > personalityCut.strong &&
	//                          personality.defensive  < personalityCut.weak
	// and inversely for behavior="defensive"
	this.personalityCut = { "weak": 0.3, "medium": 0.5, "strong": 0.7 };

	if (gameState.playerData.teamsLocked)
		this.personality.cooperative = Math.min(1, this.personality.cooperative + 0.30);
	else if (gameState.getAlliedVictory())
		this.personality.cooperative = Math.min(1, this.personality.cooperative + 0.15);

	// changing settings based on difficulty or personality
	this.Military.towerLapseTime = Math.round(this.Military.towerLapseTime * (1.1 - 0.2 * this.personality.defensive));
	this.Military.fortressLapseTime = Math.round(this.Military.fortressLapseTime * (1.1 - 0.2 * this.personality.defensive));
	this.priorities.defenseBuilding = Math.round(this.priorities.defenseBuilding * (0.9 + 0.2 * this.personality.defensive));

	if (this.difficulty < PETRA.DIFFICULTY_EASY)
	{
		this.popScaling = 0.5;
		this.Economy.supportRatio = 0.5;
		this.Economy.provisionFields = 1;
		this.Military.numSentryTowers = this.personality.defensive > this.personalityCut.strong ? 1 : 0;
	}
	else if (this.difficulty < PETRA.DIFFICULTY_MEDIUM)
	{
		this.popScaling = 0.7;
		this.Economy.supportRatio = 0.4;
		this.Economy.provisionFields = 1;
		this.Military.numSentryTowers = this.personality.defensive > this.personalityCut.strong ? 1 : 0;
	}
	else
	{
		if (this.difficulty == PETRA.DIFFICULTY_MEDIUM)
			this.Military.numSentryTowers = 1;
		else
			this.Military.numSentryTowers = 2;
		if (this.personality.defensive > this.personalityCut.strong)
			++this.Military.numSentryTowers;
		else if (this.personality.defensive < this.personalityCut.weak)
			--this.Military.numSentryTowers;

		if (this.personality.aggressive > this.personalityCut.strong)
		{
			this.Military.popForBarracks1 = 12;
			this.Economy.popPhase2 = 50;
			this.priorities.healer = 10;
		}
	}

	let maxPop = gameState.getPopulationMax();
	if (this.difficulty < PETRA.DIFFICULTY_EASY)
		this.Economy.targetNumWorkers = Math.max(1, Math.min(40, maxPop));
	else if (this.difficulty < PETRA.DIFFICULTY_MEDIUM)
		this.Economy.targetNumWorkers = Math.max(1, Math.min(60, Math.floor(maxPop/2)));
	else
		this.Economy.targetNumWorkers = Math.max(1, Math.min(120, Math.floor(maxPop/3)));
	this.Economy.targetNumTraders = 2 + this.difficulty;


	if (gameState.getVictoryConditions().has("wonder"))
	{
		this.Economy.workPhase3 = Math.floor(0.9 * this.Economy.workPhase3);
		this.Economy.workPhase4 = Math.floor(0.9 * this.Economy.workPhase4);
	}

	if (maxPop < 300)
		this.popScaling *= Math.sqrt(maxPop / 300);

	this.Military.popForBarracks1 = Math.min(Math.max(Math.floor(this.Military.popForBarracks1 * this.popScaling), 12), Math.floor(maxPop/5));
	this.Military.popForBarracks2 = Math.min(Math.max(Math.floor(this.Military.popForBarracks2 * this.popScaling), 45), Math.floor(maxPop*2/3));
	this.Military.popForForge = Math.min(Math.max(Math.floor(this.Military.popForForge * this.popScaling), 30), Math.floor(maxPop/2));
	this.Economy.popPhase2 = Math.min(Math.max(Math.floor(this.Economy.popPhase2 * this.popScaling), 20), Math.floor(maxPop/2));
	this.Economy.workPhase3 = Math.min(Math.max(Math.floor(this.Economy.workPhase3 * this.popScaling), 40), Math.floor(maxPop*2/3));
	this.Economy.workPhase4 = Math.min(Math.max(Math.floor(this.Economy.workPhase4 * this.popScaling), 45), Math.floor(maxPop*2/3));
	this.Economy.targetNumTraders = Math.round(this.Economy.targetNumTraders * this.popScaling);
	this.Economy.targetNumWorkers = Math.max(this.Economy.targetNumWorkers, this.Economy.popPhase2);
	this.Economy.workPhase3 = Math.min(this.Economy.workPhase3, this.Economy.targetNumWorkers);
	this.Economy.workPhase4 = Math.min(this.Economy.workPhase4, this.Economy.targetNumWorkers);
	if (this.difficulty < PETRA.DIFFICULTY_EASY)
		this.Economy.workPhase3 = Infinity;	// prevent the phasing to city phase

	this.emergencyValues = {
		"population": this.criticalPopulationFactors[this.difficulty],
		"structures": this.criticalStructureFactors[this.difficulty],
		"roots": this.criticalRootFactors[this.difficulty],
	};

	this.Cheat(gameState);

	if (this.debug < 2)
		return;
	API3.warn(" >>>  Petra bot: personality = " + uneval(this.personality));
};

PETRA.Config.prototype.Cheat = function(gameState)
{
	// Sandbox, Very Easy, Easy, Medium, Hard, Very Hard
	// rate apply on resource stockpiling as gathering and trading
	// time apply on building, upgrading, packing, training and technologies
	const rate = [ 0.42, 0.56, 0.75, 1.00, 1.25, 1.56 ];
	const time = [ 1.40, 1.25, 1.10, 1.00, 1.00, 1.00 ];
	const AIDiff = Math.min(this.difficulty, rate.length - 1);
	SimEngine.QueryInterface(Sim.SYSTEM_ENTITY, Sim.IID_ModifiersManager).AddModifiers("AI Bonus", {
		"ResourceGatherer/BaseSpeed": [{ "affects": ["Unit", "Structure"], "multiply": rate[AIDiff] }],
		"Trader/GainMultiplier": [{ "affects": ["Unit", "Structure"], "multiply": rate[AIDiff] }],
		"Cost/BuildTime": [{ "affects": ["Unit", "Structure"], "multiply": time[AIDiff] }],
	}, gameState.playerData.entity);
};

PETRA.Config.prototype.Serialize = function()
{
	var data = {};
	for (let key in this)
		if (this.hasOwnProperty(key) && key != "debug")
			data[key] = this[key];
	return data;
};

PETRA.Config.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

var PETRA = function(m)
{

m.launchAttackMessages = {
	"hugeAttack": [
		markForTranslation("I am starting a massive military campaign against %(_player_)s, come and join me."),
		markForTranslation("I have set up an huge army to crush %(_player_)s. Join me and you will have your share of the loot.")
	],
	"other": [
		markForTranslation("I am launching an attack against %(_player_)s."),
		markForTranslation("I have just sent an army against %(_player_)s.")
	]
};

m.answerRequestAttackMessages = {
	"join": [
		markForTranslation("Let me regroup my army and I am with you against %(_player_)s."),
		markForTranslation("I am doing the final preparation and I will attack %(_player_)s.")
	],
	"decline": [
		markForTranslation("Sorry, I do not have enough soldiers currently, but my next attack will target %(_player_)s.")
	],
	"other": [
		markForTranslation("I cannot help you against %(_player_)s for the time being, as I have another attack foreseen against %(_player_2)s.")
	]
};

m.sentTributeMessages = [
	markForTranslation("Here is a gift for %(_player_)s, make a good use of it."),
	markForTranslation("I see you are in a bad situation %(_player_)s, I hope this will help."),
	markForTranslation("I can help you this time %(_player_)s, but try to assemble more resources in the future.")
];

m.requestTributeMessages = [
	markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you."),
	markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s."),
	markForTranslation("If you have some %(resource)s excess, that would help me strengthen my army.")
];

m.newTradeRouteMessages = [
	markForTranslation("I have set up a new route with %(_player_)s. Trading will be profitable for all of us."),
	markForTranslation("A new trade route is set up with %(_player_)s. Take your share of the profits.")
];

m.newPhaseMessages = {
	"started": [
		markForTranslation("I am advancing to the %(phase)s.")
	],
	"completed": [
		markForTranslation("I have reached the %(phase)s.")
	]
};

m.newDiplomacyMessages = {
	"ally": [
		markForTranslation("%(_player_)s and I are now allies.")
	],
	"neutral": [
		markForTranslation("%(_player_)s and I are now neutral.")
	],
	"enemy": [
		markForTranslation("%(_player_)s and I are now enemies.")
	]
};

m.chatLaunchAttack = function(gameState, player, type)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.launchAttackMessages[type === "HugeAttack" ? type : "other"]),
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": { "_player_": player }
	});
};

m.chatAnswerRequestAttack = function(gameState, player, answer, other)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.answerRequestAttackMessages[answer]),
		"translateMessage": true,
		"translateParameters": answer != "other" ? ["_player_"] : ["_player_", "_player_2"],
		"parameters": answer != "other" ? { "_player_": player } : { "_player_": player, "_player2_": other }
	});
};

m.chatSentTribute = function(gameState, player)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.sentTributeMessages),
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": { "_player_": player }
	});
};

m.chatRequestTribute = function(gameState, resource)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.requestTributeMessages),
		"translateMessage": true,
		"translateParameters": { "resource": "withinSentence" },
		"parameters": { "resource": gameState.sharedScript.resourceInfo.names[resource] }
	});
};

m.chatNewTradeRoute = function(gameState, player)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.newTradeRouteMessages),
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": { "_player_": player }
	});
};

m.chatNewPhase = function(gameState, phase, status)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.newPhaseMessages[status]),
		"translateMessage": true,
		"translateParameters": ["phase"],
		"parameters": { "phase": phase }
	});
};

m.chatNewDiplomacy = function(gameState, player, newDiplomaticStance)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": pickRandom(this.newDiplomacyMessages[newDiplomaticStance]),
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": { "_player_": player }
	});
};

return m;
}(PETRA);

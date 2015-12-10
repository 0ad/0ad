var PETRA = function(m)
{

const resourceNames = {
	"food": markForTranslation("Food"),
	"wood": markForTranslation("Wood"),
	"metal": markForTranslation("Metal"),
	"stone": markForTranslation("Stone")
};

m.chatLaunchAttack = function(gameState, player)
{
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I am launching an attack against %(_player_)s.");
	else
		var message = "/team " + markForTranslation("I have just sent an army against %(_player_)s.");

	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	};
	Engine.PostCommand(PlayerID, chat);
};

m.chatAnswerRequestAttack = function(gameState, player, answer, other)
{
	if (answer)
	{
		var proba = Math.random();
		if (proba < 0.5)
			var message = "/allies " + markForTranslation("Let me regroup my army and I am with you against %(_player_)s.");
		else
			var message = "/allies " + markForTranslation("I am doing the final preparation and I will attack %(_player_)s.");
	}
	else
	{
		if (other !== undefined)
			var message = "/allies " + markForTranslation("I cannot help you against %(_player_)s for the time being, as I have another attack foreseen against %(_player_2)s.");
		else
			var message = "/allies " + markForTranslation("Sorry, I do not have enough soldiers currently, but my next attack will target %(_player_)s.");
	}

	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	};
	if (other !== undefined)
	{
		chat.translateParameters.push("_player_2");
		chat.parameters._player_2 = other;
	}
	Engine.PostCommand(PlayerID, chat);
};

m.chatSentTribute = function(gameState, player)
{
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("Here is a gift for %(_player_)s, make a good use of it.");
	else
		var message = "/team " + markForTranslation("I see you are in a bad situation %(_player_)s, I hope this will help."); 

	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	};
	Engine.PostCommand(PlayerID, chat);
};

m.chatRequestTribute = function(gameState, resource)
{
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you.");
	else
		var message = "/team " + markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s.");

	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": {"resource": "withinSentence"},
		"parameters": {"resource": resourceNames[resource]}
	};
	Engine.PostCommand(PlayerID, chat);
};

m.chatNewTradeRoute = function(gameState, player)
{
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I have set up a new route with %(_player_)s. Trading will be profitable for all of us.");
	else
		var message = "/team " + markForTranslation("A new trade route is set up with %(_player_)s. Take your share of the profits");

	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	};
	Engine.PostCommand(PlayerID, chat);
};

m.chatNewPhase = function(gameState, phase, started)
{
	if (started)
		var message = "/allies " + markForTranslation("I am advancing to the %(phase)s.");
	else
		var message = "/allies " + markForTranslation("I have reached the %(phase)s.");
	
	var chat = {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["phase"],
		"parameters": { "phase": phase }
	};
	Engine.PostCommand(PlayerID, chat);
};


return m;
}(PETRA);

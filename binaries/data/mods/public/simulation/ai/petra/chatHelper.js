var PETRA = function(m)
{

// Keep in sync with gui/common/l10n.js
const resourceNames = {
	// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
	"food": markForTranslationWithContext("withinSentence", "Food"),
	// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
	"wood": markForTranslationWithContext("withinSentence", "Wood"),
	// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
	"metal": markForTranslationWithContext("withinSentence", "Metal"),
	// Translation: Word as used in the middle of a sentence (which may require using lowercase for your language).
	"stone": markForTranslationWithContext("withinSentence", "Stone"),
};

m.chatLaunchAttack = function(gameState, player, type)
{
	var message;
	var proba = Math.random();
	if (type === "HugeAttack" && proba > 0.25 && proba < 0.75)
		message = "/team " + markForTranslation("I am starting a massive military campaign against %(_player_)s, come and join me.");
	else if (proba < 0.5)
		message = "/team " + markForTranslation("I am launching an attack against %(_player_)s.");
	else
		message = "/team " + markForTranslation("I have just sent an army against %(_player_)s.");

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
	var message;
	if (answer)
	{
		var proba = Math.random();
		if (proba < 0.5)
			message = "/allies " + markForTranslation("Let me regroup my army and I am with you against %(_player_)s.");
		else
			message = "/allies " + markForTranslation("I am doing the final preparation and I will attack %(_player_)s.");
	}
	else
	{
		if (other !== undefined)
			message = "/allies " + markForTranslation("I cannot help you against %(_player_)s for the time being, as I have another attack foreseen against %(_player_2)s.");
		else
			message = "/allies " + markForTranslation("Sorry, I do not have enough soldiers currently, but my next attack will target %(_player_)s.");
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
	var message;
	var proba = Math.random();
	if (proba < 0.33)
		message = "/team " + markForTranslation("Here is a gift for %(_player_)s, make a good use of it.");
	else if (proba < 0.66)
		message = "/team " + markForTranslation("I see you are in a bad situation %(_player_)s, I hope this will help."); 
	else
		message = "/team " + markForTranslation("I can help you this time %(_player_)s, but try to assemble more resources in the future.");

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
	var message;
	var proba = Math.random();
	if (proba < 0.33)
		message = "/team " + markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you.");
	else if (proba < 0.66)
		message = "/team " + markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s.");
	else
		message = "/team " + markForTranslation("If you have some %(resource)s excess, that would help me strengthen my army.");

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
	var message;
	var proba = Math.random();
	if (proba < 0.5)
		message = "/team " + markForTranslation("I have set up a new route with %(_player_)s. Trading will be profitable for all of us.");
	else
		message = "/team " + markForTranslation("A new trade route is set up with %(_player_)s. Take your share of the profits");

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
	var message;
	if (started)
		message = "/allies " + markForTranslation("I am advancing to the %(phase)s.");
	else
		message = "/allies " + markForTranslation("I have reached the %(phase)s.");
	
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

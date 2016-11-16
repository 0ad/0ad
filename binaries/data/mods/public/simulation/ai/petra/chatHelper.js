var PETRA = function(m)
{

m.chatLaunchAttack = function(gameState, player, type)
{
	let message;
	let proba = Math.random();
	if (type === "HugeAttack" && proba > 0.25 && proba < 0.75)
		message = markForTranslation("I am starting a massive military campaign against %(_player_)s, come and join me.");
	else if (proba < 0.5)
		message = markForTranslation("I am launching an attack against %(_player_)s.");
	else
		message = markForTranslation("I have just sent an army against %(_player_)s.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies "+ message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	});
};

m.chatAnswerRequestAttack = function(gameState, player, answer, other)
{
	let message;
	if (answer)
	{
		let proba = Math.random();
		if (proba < 0.5)
			message = markForTranslation("Let me regroup my army and I am with you against %(_player_)s.");
		else
			message = markForTranslation("I am doing the final preparation and I will attack %(_player_)s.");
	}
	else
	{
		if (other !== undefined)
			message = markForTranslation("I cannot help you against %(_player_)s for the time being, as I have another attack foreseen against %(_player_2)s.");
		else
			message =  markForTranslation("Sorry, I do not have enough soldiers currently, but my next attack will target %(_player_)s.");
	}

	let chat = {
		"type": "aichat",
		"message": "/allies " + message,
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
	let message;
	let proba = Math.random();
	if (proba < 0.33)
		message = markForTranslation("Here is a gift for %(_player_)s, make a good use of it.");
	else if (proba < 0.66)
		message = markForTranslation("I see you are in a bad situation %(_player_)s, I hope this will help.");
	else
		message = markForTranslation("I can help you this time %(_player_)s, but try to assemble more resources in the future.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	});
};

m.chatRequestTribute = function(gameState, resource)
{
	let message;
	let proba = Math.random();
	if (proba < 0.33)
		message = markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you.");
	else if (proba < 0.66)
		message = markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s.");
	else
		message = markForTranslation("If you have some %(resource)s excess, that would help me strengthen my army.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + message,
		"translateMessage": true,
		"translateParameters": {"resource": "withinSentence"},
		"parameters": {"resource": gameState.sharedScript.resourceNames[resource]}
	});
};

m.chatNewTradeRoute = function(gameState, player)
{
	let message;
	let proba = Math.random();
	if (proba < 0.5)
		message = markForTranslation("I have set up a new route with %(_player_)s. Trading will be profitable for all of us.");
	else
		message = markForTranslation("A new trade route is set up with %(_player_)s. Take your share of the profits.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	});
};

m.chatNewPhase = function(gameState, phase, started)
{
	let message = started ?
		markForTranslation("I am advancing to the %(phase)s.") :
		markForTranslation("I have reached the %(phase)s.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + message,
		"translateMessage": true,
		"translateParameters": ["phase"],
		"parameters": { "phase": phase }
	});
};

m.chatNewDiplomacy = function(gameState, player, enemy)
{
	let message = enemy ?
		markForTranslation("%(_player_)s and I are now enemies.") :
		markForTranslation("%(_player_)s and I are now allies.");

	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": {"_player_": player}
	});
};

return m;
}(PETRA);

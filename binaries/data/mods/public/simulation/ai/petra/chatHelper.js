var PETRA = function(m)
{

m.launchAttackMessages = {
	"hugeAttack": [
		markForTranslation("I am starting a massive military campaign against %(_player_)s, come and join me."),
		markForTranslation("I have set up a huge army to crush %(_player_)s. Join me and you will have your share of the loot.")
	],
	"other": [
		markForTranslation("I am launching an attack against %(_player_)s."),
		markForTranslation("I have just sent an army against %(_player_)s.")
	]
};

m.answerRequestAttackMessages = {
	"join": [
		markForTranslation("Let me regroup my army and I will then join you against %(_player_)s."),
		markForTranslation("I am finishing preparations to attack %(_player_)s.")
	],
	"decline": [
		markForTranslation("Sorry, I do not have enough soldiers currently; but my next attack will target %(_player_)s."),
		markForTranslation("Sorry, I still need to strengthen my army. However, I will attack %(_player_)s next.")
	],
	"other": [
		markForTranslation("I cannot help you against %(_player_)s for the time being, I am planning to attack %(_player_2)s first.")
	]
};

m.sentTributeMessages = [
	markForTranslation("Here is a gift for you, %(_player_)s. Make good use of it."),
	markForTranslation("I see you are in a bad situation, %(_player_)s. I hope this helps."),
	markForTranslation("I can help you this time, %(_player_)s, but you should manage your resources more carefully in the future.")
];

m.requestTributeMessages = [
	markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you."),
	markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s."),
	markForTranslation("If you can spare me some %(resource)s, I will be able to strengthen my army.")
];

m.newTradeRouteMessages = [
	markForTranslation("I have set up a new route with %(_player_)s. Trading will be profitable for all of us."),
	markForTranslation("A new trade route is set up with %(_player_)s. Take your share of the profits.")
];

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

m.answerDiplomacyRequestMessages = {
	"ally": {
		"decline": [
			markForTranslation("I cannot accept your offer to become allies, %(_player_)s.")
		],
		"declineSuggestNeutral": [
			markForTranslation("I will not be your ally, %(_player_)s. However, I will consider a neutrality pact."),
			markForTranslation("I reject your request for alliance, %(_player_)s, but we could become neutral."),
			markForTranslation("%(_player_)s, only a neutrality agreement is conceivable to me.")
		],
		"declineRepeatedOffer": [
			markForTranslation("Our previous alliance did not work out, %(_player_)s. I must decline your offer."),
			markForTranslation("I won’t ally you again, %(_player_)s!"),
			markForTranslation("No more alliances between us, %(_player_)s!"),
			markForTranslation("Your request for peace means nothing to me anymore, %(_player_)s!"),
			markForTranslation("My answer to your repeated peace proposal will remain war, %(_player_)s!")
		],
		"accept": [
			markForTranslation("I will accept your offer to become allies, %(_player_)s. We will both benefit from this partnership."),
			markForTranslation("An alliance between us is a good idea, %(_player_)s."),
			markForTranslation("Let both of our people prosper from a peaceful association, %(_player_)s."),
			markForTranslation("We have found common ground, %(_player_)s. I accept the alliance."),
			markForTranslation("%(_player_)s, consider us allies from now on.")
		],
		"acceptWithTribute": [
			markForTranslation("I will ally with you, %(_player_)s, but only if you send me a tribute of %(_amount_)s %(_resource_)s."),
			markForTranslation("%(_player_)s, you must send me a tribute of %(_amount_)s %(_resource_)s before I accept an alliance with you."),
			markForTranslation("Unless you send me %(_amount_)s %(_resource_)s, an alliance won’t be formed, %(_player_)s,")
		],
		"waitingForTribute": [
			markForTranslation("%(_player_)s, my offer still stands. I will ally with you only if you send me a tribute of %(_amount_)s %(_resource_)s."),
			markForTranslation("I’m still waiting for %(_amount_)s %(_resource_)s before accepting your alliance, %(_player_)s."),
			markForTranslation("%(_player_)s, if you do not send me part of the %(_amount_)s %(_resource_)s tribute soon, I will break off our negotiations.")
		]
	},
	"neutral": {
		"decline": [
			markForTranslation("I will not become neutral with you, %(_player_)s."),
			markForTranslation("%(_player_)s, I must decline your request for a neutrality pact.")
		],
		"declineRepeatedOffer": [
			markForTranslation("Our previous neutrality agreement ended in failure, %(_player_)s; I will not consider another one.")
		],
		"accept": [
			markForTranslation("I welcome your request for peace between our civilizations, %(_player_)s. I will accept."),
			markForTranslation("%(_player_)s, I will accept your neutrality request. May both our civilizations benefit.")
		],
		"acceptWithTribute": [
			markForTranslation("If you send me a tribute of %(_amount_)s %(_resource_)s, I will accept your neutrality request, %(_player_)s."),
			markForTranslation("%(_player_)s, if you send me %(_amount_)s %(_resource_)s, I will accept a neutrality pact.")
		],
		"waitingForTribute": [
			markForTranslation("%(_player_)s, I will not accept your neutrality request unless you tribute me %(_amount_)s %(_resource_)s soon."),
			markForTranslation("%(_player_)s, if you do not send me part of the %(_amount_)s %(_resource_)s tribute soon, I will break off our negotiations.")
		]
	}
};

m.sendDiplomacyRequestMessages = {
	"ally": {
		"sendRequest": [
			markForTranslation("%(_player_)s, it would help both of our civilizations if we formed an alliance. If you become allies with me, I will respond in kind.")
		],
		"requestExpired": [
			markForTranslation("%(_player_)s, my offer for an alliance has expired."),
			markForTranslation("%(_player_)s, I have rescinded my previous offer for an alliance between us."),
		]
	},
	"neutral": {
		"sendRequest": [
			markForTranslation("%(_player_)s, I would like to request a neutrality pact between our civilizations. If you become neutral with me, I will respond in kind."),
			markForTranslation("%(_player_)s, it would be both to our benefit if we negotiated a neutrality pact. I will become neutral with you if you do the same.")
		],
		"requestExpired": [
			markForTranslation("%(_player_)s, I have decided to revoke my offer for a neutrality pact."),
			markForTranslation("%(_player_)s, as you have failed to respond to my request for peace between us, I have abrogated my offer."),
		]
	}
};

m.chatLaunchAttack = function(gameState, player, type)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/allies " + pickRandom(this.launchAttackMessages[type === "HugeAttack" ? "hugeAttack" : "other"]),
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
		"parameters": answer != "other" ? { "_player_": player } : { "_player_": player, "_player_2": other }
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
		"parameters": { "resource": Resources.GetNames()[resource] }
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

m.chatAnswerRequestDiplomacy = function(gameState, player, requestType, response, requiredTribute)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/msg " + gameState.sharedScript.playersData[player].name + " " +
			pickRandom(this.answerDiplomacyRequestMessages[requestType][response]),
		"translateMessage": true,
		"translateParameters": requiredTribute ? ["_amount_", "_resource_", "_player_"] : ["_player_"],
		"parameters": requiredTribute ?
			{ "_amount_": requiredTribute.wanted, "_resource_": requiredTribute.type, "_player_": player } :
			{ "_player_": player }
	});
};

m.chatNewRequestDiplomacy = function(gameState, player, requestType, status)
{
	Engine.PostCommand(PlayerID, {
		"type": "aichat",
		"message": "/msg " + gameState.sharedScript.playersData[player].name + " " +
			pickRandom(this.sendDiplomacyRequestMessages[requestType][status]),
		"translateMessage": true,
		"translateParameters": ["_player_"],
		"parameters": { "_player_": player }
	});
};

return m;
}(PETRA);

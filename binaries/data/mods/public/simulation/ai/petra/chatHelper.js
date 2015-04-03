var PETRA = function(m)
{

m.chatLaunchAttack = function(gameState, player)
{
	var name = gameState.sharedScript.playersData[player].name;
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I am launching an attack against %(name)s.");
	else
		var message = "/team " + markForTranslation("I have just sent an army against %(name)s.");

	var chat = { "type": "aichat", "message": message, "translateMessage": true, "translateParameters": ["name"], "parameters": { "name": name } };
	Engine.PostCommand(PlayerID, chat);
};

m.chatSentTribute = function(gameState, player)
{
	var name = gameState.sharedScript.playersData[player].name;
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("Here is a gift for %(name)s, make a good use of it.");
	else
		var message = "/team " + markForTranslation("I see you are in a bad situation %(name)s, I hope this will help."); 

	var chat = { "type": "aichat", "message": message, "translateMessage": true, "translateParameters": ["name"], "parameters": { "name": name } };
	Engine.PostCommand(PlayerID, chat);
};

m.chatRequestTribute = function(gameState, resource)
{
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I am in need of %(resource)s, can you help? I will make it up to you.");
	else
		var message = "/team " + markForTranslation("I would participate more efficiently in our common war effort if you could provide me some %(resource)s.");

	var chat = { "type": "aichat", "message": message, "translateMessage": true, "translateParameters": ["resource"], "parameters": { "resource": resource } };
	Engine.PostCommand(PlayerID, chat);
};

m.chatNewTradeRoute = function(gameState, player)
{
	var name = gameState.sharedScript.playersData[player].name;
	var proba = Math.random();
	if (proba < 0.5)
		var message = "/team " + markForTranslation("I have set up a new route with %(name)s. Trading will be profitable for all of us.");
	else
		var message = "/team " + markForTranslation("A new trade route is set up with %(name)s. Take your share of the profits");

	var chat = { "type": "aichat", "message": message, "translateMessage": true, "translateParameters": ["name"], "parameters": { "name": name } };
	Engine.PostCommand(PlayerID, chat);
};

return m;
}(PETRA);

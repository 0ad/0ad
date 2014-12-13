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

return m;
}(PETRA);

function ScaredyBotAI(settings)
{
	warn("Constructing ScaredyBotAI for player "+settings.player);

	this.player = settings.player;
	this.turn = 0;
	this.suicideTurn = 20;
}

ScaredyBotAI.prototype.HandleMessage = function(state)
{
//	print("### HandleMessage("+uneval(state)+")\n\n");
//	print(uneval(this)+"\n\n");

	if (this.turn == 0)
	{
		Engine.PostCommand({"type": "chat", "message": "Good morning."});
	}

	if (this.turn == this.suicideTurn)
	{
		Engine.PostCommand({"type": "chat", "message": "I quake in my boots! My troops cannot hope to survive against a power such as yours."});

		var myEntities = [];
		for (var ent in state.entities)
			if (state.entities[ent].player == this.player)
				myEntities.push(+ent);
		Engine.PostCommand({"type": "delete-entities", "entities": myEntities});
	}

	this.turn++;
};

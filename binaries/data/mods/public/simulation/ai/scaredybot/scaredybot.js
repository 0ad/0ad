Engine.IncludeModule("common-api");

function ScaredyBotAI(settings)
{
//	warn("Constructing ScaredyBotAI for player "+settings.player);

	BaseAI.call(this, settings);

	this.turn = 0;
	this.suicideTurn = 20;
}

ScaredyBotAI.prototype = new BaseAI();

ScaredyBotAI.prototype.OnUpdate = function()
{
	if (this.turn == 0)
		this.chat("Good morning.");

	if (this.turn == this.suicideTurn)
	{
		this.chat("I quake in my boots! My troops cannot hope to survive against a power such as yours.");

		this.entities.filter(function(ent) { return ent.isOwn(); }).destroy();
	}

	this.turn++;
};

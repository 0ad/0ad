/**
 * Shows an overlay if the game is lagging behind the net server.
 */
class NetworkDelayOverlay
{
	constructor()
	{
		this.netDelayOverlay = Engine.GetGUIObjectByName("netDelayOverlay");

		this.netDelayOverlay.caption="toto";
		this.caption = translate(this.Caption);
		this.sprintfData = {};

		this.initialSimRate = Engine.GetSimRate();
		this.currentSimRate = this.initialSimRate;

		setTimeout(() => this.CheckDelay(), 1000);
	}

	CheckDelay()
	{
		setTimeout(() => this.CheckDelay(), 1000);
		let delay = +(Engine.HasNetClient() && Engine.GetPendingTurns());

		if (g_IsObserver && Engine.ConfigDB_GetValue("user", "network.autocatchup"))
		{
			if (delay > this.MAX_LIVE_DELAY && this.currentSimRate <= this.initialSimRate)
			{
				this.currentSimRate = this.initialSimRate * 1.1;
				Engine.SetSimRate(this.currentSimRate);
			}
			else if (delay <= this.NORMAL_DELAY && this.currentSimRate > this.initialSimRate)
			{
				this.currentSimRate = this.initialSimRate;
				Engine.SetSimRate(this.currentSimRate);
			}
		}

		if (delay < this.MAX_LIVE_DELAY)
		{
			this.netDelayOverlay.hidden = true;
			return;
		}
		this.netDelayOverlay.hidden = false;
		this.sprintfData.delay = (delay / this.TURNS_PER_SECOND);
		this.sprintfData.delay = this.sprintfData.delay.toFixed(this.sprintfData.delay < 5 ? 1 : 0);
		this.netDelayOverlay.caption = sprintf(this.caption, this.sprintfData);
	}
}

/**
 * Because of command delay, we can still be several turns behind the 'ready' turn and not
 * particularly late. This should be kept in sync with the command delay.
 */
NetworkDelayOverlay.prototype.NORMAL_DELAY = 3;
NetworkDelayOverlay.prototype.MAX_LIVE_DELAY = 6;

/**
 * This needs to be kept in sync with the turn length.
 */
NetworkDelayOverlay.prototype.TURNS_PER_SECOND = 5;

NetworkDelayOverlay.prototype.Caption = translate("Delay to live stream: %(delay)ss");

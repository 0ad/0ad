/**
 * The prototype of this class is extended by subclasses in according files.
 */
class DiplomacyDialogPlayerControl
{
}

class DiplomacyDialogPlayerControlManager
{
	constructor()
	{
		this.controls = {};
		for (let name in DiplomacyDialogPlayerControl.prototype)
		{
			this.controls[name] = [];

			// Exclude gaia
			for (let playerID = 1; playerID < g_Players.length; ++playerID)
				this.controls[name][playerID] = new DiplomacyDialogPlayerControl.prototype[name](playerID);
		}
	}

	isInactive(playerID)
	{
		return playerID == g_ViewedPlayer ||
			isPlayerObserver(g_ViewedPlayer) ||
			isPlayerObserver(playerID);
	}

	update()
	{
		for (let playerID = 1; playerID < g_Players.length; ++playerID)
		{
			let isInactive = this.isInactive(playerID);
			for (let name in this.controls)
				this.controls[name][playerID].update(isInactive);
		}
	}

	onSpyResponse(notification, player)
	{
		for (let name in this.controls)
			for (let playerID = 1; playerID < g_Players.length; ++playerID)
			{
				if (!this.controls[name][playerID].onSpyResponse)
					break;

				this.controls[name][playerID].onSpyResponse(notification, player);
			}
	}
}

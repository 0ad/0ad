/**
 * This class will spawn a dialog asking to exit the game in case the user was an assigned player who has been defeated.
 */
class QuitConfirmationDefeat extends QuitConfirmation
{
	constructor()
	{
		super();

		if (Engine.IsAtlasRunning())
			return;

		this.confirmHandler = undefined;
		registerPlayersFinishedHandler(this.onPlayersFinished.bind(this));
	}

	onPlayersFinished(players, won)
	{
		if (players.indexOf(Engine.GetPlayerID()) == -1)
			return;

		// Defer simulation result until
		// 1. the loading screen finished for all networked clients (g_IsNetworkedActive)
		// 2. all messages modifying g_Players victory state were processed (next turn)
		this.confirmHandler = this.confirmExit.bind(this, won);
		registerSimulationUpdateHandler(this.confirmHandler);
	}

	confirmExit(won)
	{
		if (g_IsNetworked && !g_IsNetworkedActive)
			return;

		unregisterSimulationUpdateHandler(this.confirmHandler);

		// Don't ask for exit if other humans are still playing.
		let askExit = !Engine.HasNetServer() || g_Players.every((player, i) =>
			i == 0 ||
			player.state != "active" ||
			g_GameAttributes.settings.PlayerData[i].AI != "");

		this.Title = won ? this.TitleVictory : this.TitleDefeated;

		this.Caption =
			g_PlayerStateMessages[won ? "won" : "defeated"] +
			(askExit ? "\n" + this.Question : "");

		this.Buttons = askExit ? super.Buttons : undefined;

		this.display();
	}
}

QuitConfirmationDefeat.prototype.TitleVictory = translate("VICTORIOUS!");
QuitConfirmationDefeat.prototype.TitleDefeated = translate("DEFEATED!");

QuitConfirmationDefeat.prototype.Question = translate("Do you want to quit?");

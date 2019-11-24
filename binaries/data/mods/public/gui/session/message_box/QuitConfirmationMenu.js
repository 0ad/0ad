/**
 * The class that is enabled() will be triggered when the user clicks on the Exit menu button.
 */
class QuitConfirmationMenu
{
}

/**
 * In single-player mode, replay mode and for observers in multiplayer matches that
 * aren't the host, exit the match instantly.
 */
QuitConfirmationMenu.prototype.Singleplayer = class extends QuitConfirmation
{
	enabled()
	{
		return !g_IsNetworked || (!g_IsController && g_IsObserver);
	}
};

/**
 * If the current player is the host of a networked match, have the player
 * confirm intent to end the game for the remote players as well.
 */
QuitConfirmationMenu.prototype.MultiplayerHost = class extends QuitConfirmation
{
	enabled()
	{
		return g_IsNetworked && g_IsController;
	}
};

QuitConfirmationMenu.prototype.MultiplayerHost.prototype.Caption =
	translate("Are you sure you want to quit? Leaving will disconnect all other players.");

/**
 * Active players that aren't the host will be asked if they want to resign before leaving.
 */
QuitConfirmationMenu.prototype.MultiplayerClient = class extends QuitConfirmation
{
	enabled()
	{
		return g_IsNetworked && !g_IsController && !g_IsObserver;
	}
};

QuitConfirmationMenu.prototype.MultiplayerClient.prototype.Buttons =
[
	{
		"caption": translate("No")
	},
	{
		"caption": translate("Yes"),
		"onPress": 	() => {
			(new ReturnQuestion()).display();
		}
	}
];

class ReturnQuestion extends SessionMessageBox
{
}

ReturnQuestion.prototype.Title = translate("Confirmation");
ReturnQuestion.prototype.Caption = translate("Do you want to resign or will you return soon?");
ReturnQuestion.prototype.Buttons = [
	{
		"caption": translate("I will return"),
		"onPress": endGame
	},
	{
		"caption": translate("I resign"),
		"onPress": () => {
			Engine.PostNetworkCommand({
				"type": "resign"
			});
		}
	}
];

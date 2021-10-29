/**
 * This class is concerned with opening a message box if the game is in replaymode and that replay ended.
 */
class QuitConfirmationReplay extends SessionMessageBox
{
	constructor()
	{
		super();
		Engine.GetGUIObjectByName("session").onReplayFinished = this.display.bind(this);
	}
}

QuitConfirmationReplay.prototype.Title =
	translateWithContext("replayFinished", "Confirmation");

QuitConfirmationReplay.prototype.Caption =
	translateWithContext("replayFinished", "The replay has finished. What do you want to do?");

QuitConfirmationReplay.prototype.Buttons =
[
	{
		// Translation: Shown in the Dialog that shows up when a replay finishes
		"caption": translate("Stay")
	},
	{
		// Translation: Shown in the Dialog that shows up when a replay finishes
		"caption": translate("Quit and View Summary"),
		"onPress": () => { endGame(true); }
	},
	{
		// Translation: Shown in the Dialog that shows up when a replay finishes
		"caption": translate("Quit"),
		"onPress": () => { endGame(false); }
	}
];

QuitConfirmationReplay.prototype.Width = 600;
QuitConfirmationReplay.prototype.Height = 200;

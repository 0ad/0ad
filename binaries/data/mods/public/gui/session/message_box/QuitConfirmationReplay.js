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
	translateWithContext("replayFinished", "The replay has finished. Do you want to quit?");

QuitConfirmationReplay.prototype.Buttons =
[
	{
		"caption": translateWithContext("replayFinished", "No")
	},
	{
		"caption": translateWithContext("replayFinished", "Yes"),
		"onPress": endGame
	}
];

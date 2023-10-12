class QuitConfirmation extends SessionMessageBox
{
}

QuitConfirmation.prototype.Title =
	translate("Confirmation");

QuitConfirmation.prototype.Caption =
	translate("The game has finished, what do you want to do?");

QuitConfirmation.prototype.Buttons =
[
	{
		// Translation: Shown in the Dialog that shows up when the game finishes
		"caption": translate("Stay"),
		"onPress": resumeGame
	},
	{
		// Translation: Shown in the Dialog that shows up when the game finishes
		"caption": translate("Quit and View Summary"),
		"onPress": () => { endGame(true); }
	},
	{
		// Translation: Shown in the Dialog that shows up when the game finishes
		"caption": translate("Quit"),
		"onPress": () => { endGame(false); }
	}
];

QuitConfirmation.prototype.Width = 600;
QuitConfirmation.prototype.Height = 200;

QuitConfirmation.prototype.ResumeOnClose = false;

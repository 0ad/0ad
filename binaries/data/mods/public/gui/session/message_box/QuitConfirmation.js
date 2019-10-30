class QuitConfirmation extends SessionMessageBox
{
}

QuitConfirmation.prototype.Title =
	translate("Confirmation");

QuitConfirmation.prototype.Caption =
	translate("Are you sure you want to quit?");

QuitConfirmation.prototype.Buttons =
[
	{
		"caption": translate("No")
	},
	{
		"caption": translate("Yes"),
		"onPress": endGame
	}
];

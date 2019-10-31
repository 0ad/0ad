class ResignConfirmation extends SessionMessageBox
{
}

ResignConfirmation.prototype.Title = translate("Confirmation");
ResignConfirmation.prototype.Caption = translate("Are you sure you want to resign?");
ResignConfirmation.prototype.Buttons = [
	{
		"caption": translate("No")
	},
	{
		"caption": translate("Yes"),
		"onPress": () => {
			Engine.PostNetworkCommand({
				"type": "resign"
			});
		}
	}
];

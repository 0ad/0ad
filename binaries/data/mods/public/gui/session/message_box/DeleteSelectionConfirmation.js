class DeleteSelectionConfirmation extends SessionMessageBox
{
	constructor(deleteSelection)
	{
		super();
		this.deleteSelection = deleteSelection;
	}
}

DeleteSelectionConfirmation.prototype.Title = translate("Delete");
DeleteSelectionConfirmation.prototype.Caption = translate("Destroy everything currently selected?");
DeleteSelectionConfirmation.prototype.Buttons = [
	{
		"caption": translate("No")
	},
	{
		"caption": translate("Yes"),
		"onPress": function() { this.deleteSelection();  }
	}
];

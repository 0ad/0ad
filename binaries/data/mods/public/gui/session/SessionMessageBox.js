/**
 * This is the same as a regular MessageBox, but it pauses if it is
 * a single-player game, until the player answered the dialog.
 */
class SessionMessageBox
{
	display()
	{
		this.onPageOpening();

		Engine.PushGuiPage(
			"page_msgbox.xml",
			{
				"width": this.Width,
				"height": this.Height,
				"title": this.Title,
				"message": this.Caption,
				"buttonCaptions": this.Buttons ? this.Buttons.map(button => button.caption) : undefined,
			},
			this.onPageClosed.bind(this));
	}

	onPageOpening()
	{
		closeOpenDialogs();
		g_PauseControl.implicitPause();
	}

	onPageClosed(buttonId)
	{
		if (this.Buttons && this.Buttons[buttonId].onPress)
			this.Buttons[buttonId].onPress.call(this);

		if (Engine.IsGameStarted())
			resumeGame();
	}
}

SessionMessageBox.prototype.Width = 400;
SessionMessageBox.prototype.Height = 200;

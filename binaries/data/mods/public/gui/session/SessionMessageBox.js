/**
 * This is the same as a regular MessageBox, but it pauses if it is
 * a single-player game, until the player answered the dialog.
 */
class SessionMessageBox
{
	async display()
	{
		closeOpenDialogs();
		g_PauseControl.implicitPause();

		const buttonId = await Engine.PushGuiPage(
			"page_msgbox.xml",
			{
				"width": this.Width,
				"height": this.Height,
				"title": this.Title,
				"message": this.Caption,
				"buttonCaptions": this.Buttons ? this.Buttons.map(button => button.caption) : undefined,
			});

		if (this.Buttons && this.Buttons[buttonId].onPress)
			this.Buttons[buttonId].onPress.call(this);

		if (this.ResumeOnClose)
			resumeGame();
	}
}

SessionMessageBox.prototype.Width = 400;
SessionMessageBox.prototype.Height = 200;

SessionMessageBox.prototype.ResumeOnClose = true;

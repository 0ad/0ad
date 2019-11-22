/**
 * The purpose of this class is to display the room subject in a panel and to
 * update it when a new one was received.
 */
// TODO: It should be easily possible to view the subject after a game has been selected
class Subject
{
	constructor(dialog, xmppMessages, gameList)
	{
		this.subjectPanel = Engine.GetGUIObjectByName("subjectPanel");
		this.subjectText = Engine.GetGUIObjectByName("subjectText");
		this.subjectBox = Engine.GetGUIObjectByName("subjectBox");
		this.logoTop = Engine.GetGUIObjectByName("logoTop");
		this.logoCenter = Engine.GetGUIObjectByName("logoCenter");

		this.updateSubject(Engine.LobbyGetRoomSubject());

		xmppMessages.registerXmppMessageHandler("chat", "subject", this.onSubject.bind(this));
		gameList.registerSelectionChangeHandler(this.onGameListSelectionChange.bind(this));

		let bottom = Engine.GetGUIObjectByName(dialog ? "leaveButton" : "hostButton").size.top - 5;
		let size = this.subjectPanel.size;
		size.bottom = bottom;
		this.subjectPanel.size = size;
	}

	onGameListSelectionChange(game)
	{
		this.subjectPanel.hidden = !!game;
	}

	onSubject(message)
	{
		this.updateSubject(message.subject);
	}

	updateSubject(subject)
	{
		subject = subject.trim();
		this.subjectBox.hidden = !subject;
		this.subjectText.caption = subject;
		this.logoTop.hidden = !subject;
		this.logoCenter.hidden = !!subject;
	}
}

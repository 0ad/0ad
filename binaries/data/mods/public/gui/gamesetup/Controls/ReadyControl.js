/**
 * Ready system:
 *
 * The ready mechanism protects the players from being assigned to a match with settings they didn't explicitly agree with.
 * It shall be technically possible to start a networked game until all participating players formally agree with the chosen settings.
 *
 * Therefore assume the readystate from the user interface rather than trusting the server whether the current player is ready.
 * The server may set readiness to false but not to true.
 *
 * The ReadyControl class stores the ready state of the current player and fires an event if the agreed settings changed.
 */
class ReadyControl
{
	constructor(netMessages, gameSettingsControl, startGameControl, playerAssignmentsControl)
	{
		this.startGameControl = startGameControl;
		this.playerAssignmentsControl = playerAssignmentsControl;

		this.resetReadyHandlers = new Set();
		this.previousAssignments = {};

		// This variable keeps track whether the local player is ready
		// As part of cheat prevention, the server may set this to NotReady, but
		// only the UI may set it to Ready or StayReady.
		this.readyState = this.NotReady;

		netMessages.registerNetMessageHandler("ready", this.onReadyMessage.bind(this));
		gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
		playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
		playerAssignmentsControl.registerClientLeaveHandler(this.onClientLeave.bind(this));
	}

	registerResetReadyHandler(handler)
	{
		this.resetReadyHandlers.add(handler);
	}

	onClientJoin(newGUID, newAssignments)
	{
		if (newAssignments[newGUID].player != -1)
			this.resetReady();
	}

	onClientLeave(guid)
	{
		if (g_PlayerAssignments[guid].player != -1)
			this.resetReady();
	}

	onReadyMessage(message)
	{
		let playerAssignment = g_PlayerAssignments[message.guid];
		if (playerAssignment)
		{
			playerAssignment.status = message.status;
			this.playerAssignmentsControl.updatePlayerAssignments();
		}
	}

	onPlayerAssignmentsChange()
	{
		// Don't let the host tell you that you're ready when you're not.
		let playerAssignment = g_PlayerAssignments[Engine.GetPlayerGUID()];
		if (playerAssignment && playerAssignment.status > this.readyState)
			playerAssignment.status = this.readyState;

		for (let guid in g_PlayerAssignments)
			if (this.previousAssignments[guid] &&
				this.previousAssignments[guid].player != g_PlayerAssignments[guid].player)
			{
				this.resetReady();
				return;
			}
	}

	onGameAttributesBatchChange()
	{
		this.resetReady();
	}

	setReady(ready, sendMessage)
	{
		this.readyState = ready;

		if (sendMessage)
			Engine.SendNetworkReady(ready);

		// Update GUI objects instantly if relevant settingchange was detected
		let playerAssignment = g_PlayerAssignments[Engine.GetPlayerGUID()];
		if (playerAssignment)
		{
			playerAssignment.status = ready;
			this.playerAssignmentsControl.updatePlayerAssignments();
		}
	}

	resetReady()
	{
		// The gameStarted check is only necessary to allow the host to
		// determine the Seed and random items after clicking start
		if (!g_IsNetworked || this.startGameControl.gameStarted)
			return;

		for (let handler of this.resetReadyHandlers)
			handler();

		if (g_IsController)
		{
			Engine.ClearAllPlayerReady();
			this.playerAssignmentsControl.updatePlayerAssignments();
		}
		else if (this.readyState != this.StayReady)
			this.setReady(this.NotReady, false);
	}

	getLocalReadyState()
	{
		return this.readyState;
	}
}

ReadyControl.prototype.NotReady = 0;

ReadyControl.prototype.Ready = 1;

ReadyControl.prototype.StayReady = 2;

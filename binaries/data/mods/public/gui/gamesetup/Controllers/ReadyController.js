/**
 * Ready system:
 *
 * The ready mechanism protects the players from being assigned to a match with settings they didn't explicitly agree with.
 * It shall be technically possible to start a networked game until all participating players formally agree with the chosen settings.
 *
 * Therefore assume the readystate from the user interface rather than trusting the server whether the current player is ready.
 * The server may set readiness to false but not to true.
 *
 * The ReadyController class stores the ready state of the current player and fires an event if the agreed settings changed.
 */
class ReadyController
{
	constructor(netMessages, gameSettingsController, playerAssignmentsController)
	{
		this.playerAssignmentsController = playerAssignmentsController;
		this.gameSettingsController = gameSettingsController;

		this.resetReadyHandlers = new Set();
		this.previousAssignments = {};

		// This variable keeps track whether the local player is ready
		// As part of cheat prevention, the server may set this to NotReady, but
		// only the UI may set it to Ready or StayReady.
		this.readyState = this.NotReady;

		netMessages.registerNetMessageHandler("ready", this.onReadyMessage.bind(this));
		gameSettingsController.registerSettingsChangeHandler(this.onSettingsChange.bind(this));
		playerAssignmentsController.registerClientJoinHandler(this.onClientJoin.bind(this));
		playerAssignmentsController.registerClientLeaveHandler(this.onClientLeave.bind(this));
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
			this.playerAssignmentsController.updatePlayerAssignments();
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

	onSettingsChange()
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
			this.playerAssignmentsController.updatePlayerAssignments();
		}
	}

	resetReady()
	{
		// The gameStarted check is only necessary to allow the host to
		// determine random items after clicking start.
		if (!g_IsNetworked || this.gameSettingsController.gameStarted)
			return;

		for (let handler of this.resetReadyHandlers)
			handler();

		if (g_IsController)
		{
			Engine.ClearAllPlayerReady();
			this.playerAssignmentsController.updatePlayerAssignments();
		}
		else if (this.readyState != this.StayReady)
			this.setReady(this.NotReady, false);
	}

	getLocalReadyState()
	{
		return this.readyState;
	}
}

ReadyController.prototype.NotReady = 0;

ReadyController.prototype.Ready = 1;

ReadyController.prototype.StayReady = 2;

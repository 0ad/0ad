/**
 * This class provides a property independent interface to g_PlayerAssignment events and actions.
 */
class PlayerAssignmentsControl
{
	constructor(setupWindow, netMessages)
	{
		this.clientJoinHandlers = new Set();
		this.clientLeaveHandlers = new Set();
		this.playerAssignmentsChangeHandlers = new Set();

		if (!g_IsNetworked)
		{
			let name = singleplayerName();

			// Replace empty player name when entering a single-player match for the first time.
			Engine.ConfigDB_CreateAndWriteValueToFile("user", this.ConfigNameSingleplayer, name, "config/user.cfg");

			g_PlayerAssignments = {
				"local": {
					"name": name,
					"player": -1
				}
			};
		}

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.registerGetHotloadDataHandler(this.onGetHotloadData.bind(this));
		netMessages.registerNetMessageHandler("players", this.onPlayerAssignmentMessage.bind(this));
	}

	registerPlayerAssignmentsChangeHandler(handler)
	{
		this.playerAssignmentsChangeHandlers.add(handler);
	}

	unregisterPlayerAssignmentsChangeHandler(handler)
	{
		this.playerAssignmentsChangeHandlers.delete(handler);
	}

	registerClientJoinHandler(handler)
	{
		this.clientJoinHandlers.add(handler);
	}

	unregisterClientJoinHandler(handler)
	{
		this.clientJoinHandlers.delete(handler);
	}

	registerClientLeaveHandler(handler)
	{
		this.clientLeaveHandlers.add(handler);
	}

	unregisterClientLeaveHandler(handler)
	{
		this.clientLeaveHandlers.delete(handler);
	}

	onLoad(initData, hotloadData)
	{
		if (hotloadData)
		{
			g_PlayerAssignments = hotloadData.playerAssignments;
			this.updatePlayerAssignments();
		}
	}

	onGetHotloadData(object)
	{
		object.playerAssignments = g_PlayerAssignments;
	}

	/**
	 * To be called when g_PlayerAssignments is modified.
	 */
	updatePlayerAssignments()
	{
		Engine.ProfileStart("updatePlayerAssignments");
		for (let handler of this.playerAssignmentsChangeHandlers)
			handler();
		Engine.ProfileStop();
	}

	/**
	 * Called whenever a client joins/leaves or any gamesetting is changed.
	 */
	onPlayerAssignmentMessage(message)
	{
		let newAssignments = message.newAssignments;
		for (let guid in newAssignments)
			if (!g_PlayerAssignments[guid])
				for (let handler of this.clientJoinHandlers)
					handler(guid, message.newAssignments);

		for (let guid in g_PlayerAssignments)
			if (!newAssignments[guid])
				for (let handler of this.clientLeaveHandlers)
					handler(guid);

		g_PlayerAssignments = newAssignments;
		this.updatePlayerAssignments();
	}

	assignClient(guid, playerIndex)
	{
		if (g_IsNetworked)
			Engine.AssignNetworkPlayer(playerIndex, guid);
		else
		{
			g_PlayerAssignments[guid].player = playerIndex;
			this.updatePlayerAssignments();
		}
	}

	/**
	 * If both clients are assigned players, this will swap their assignments.
	 */
	assignPlayer(guidToAssign, playerIndex)
	{
		if (g_PlayerAssignments[guidToAssign].player != -1)
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player == playerIndex + 1)
				{
					this.assignClient(guid, g_PlayerAssignments[guidToAssign].player);
					break;
				}

		this.assignClient(guidToAssign, playerIndex + 1);

		if (!g_IsNetworked)
			this.updatePlayerAssignments();
	}

	unassignClient(playerID)
	{
		if (g_IsNetworked)
			Engine.AssignNetworkPlayer(playerID, "");
		else if (g_PlayerAssignments.local.player == playerID)
		{
			g_PlayerAssignments.local.player = -1;
			this.updatePlayerAssignments();
		}
	}

	unassignInvalidPlayers()
	{
		if (g_IsNetworked)
			for (let playerID = g_GameAttributes.settings.PlayerData.length + 1; playerID <= g_MaxPlayers; ++playerID)
				// Remove obsolete playerIDs from the servers playerassignments copy
				Engine.AssignNetworkPlayer(playerID, "");

		else if (g_PlayerAssignments.local.player > g_GameAttributes.settings.PlayerData.length)
		{
			g_PlayerAssignments.local.player = -1;
			this.updatePlayerAssignments();
		}
	}
}

PlayerAssignmentsControl.prototype.ConfigNameSingleplayer =
	"playername.singleplayer";

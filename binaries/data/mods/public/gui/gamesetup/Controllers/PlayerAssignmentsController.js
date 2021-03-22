/**
 * This class provides a property independent interface to g_PlayerAssignment events and actions.
 */
class PlayerAssignmentsController
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

			// By default, assign the player to the first slot.
			g_PlayerAssignments = {
				"local": {
					"name": name,
					"player": 1
				}
			};
		}

		g_GameSettings.playerCount.watch(() => this.unassignInvalidPlayers(), ["nbPlayers"]);

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.registerGetHotloadDataHandler(this.onGetHotloadData.bind(this));
		netMessages.registerNetMessageHandler("players", this.onPlayerAssignmentMessage.bind(this));

		this.registerClientJoinHandler(this.onClientJoin.bind(this));
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
		else if (!g_IsNetworked)
		{
			// Simulate a net message for the local player to keep a common path.
			this.onPlayerAssignmentMessage({
				"newAssignments": g_PlayerAssignments
			});
		}
	}

	onGetHotloadData(object)
	{
		object.playerAssignments = g_PlayerAssignments;
	}

	/**
	 * On client join, try to assign them to a free slot.
	 * (This is called before g_PlayerAssignments is updated).
	 */
	onClientJoin(newGUID, newAssignments)
	{
		if (!g_IsController || newAssignments[newGUID].player != -1)
			return;

		// Assign the client (or only buddies if prefered) to a free slot
		if (newGUID != Engine.GetPlayerGUID())
		{
			let assignOption = Engine.ConfigDB_GetValue("user", this.ConfigAssignPlayers);
			if (assignOption == "disabled" ||
				assignOption == "buddies" && g_Buddies.indexOf(splitRatingFromNick(newAssignments[newGUID].name).nick) == -1)
				return;
		}

		// Find a player slot that no other player is assigned to.
		let firstFreeSlot = [...Array(g_MaxPlayers).keys()];
		firstFreeSlot = firstFreeSlot.find(i => {
			for (let guid in newAssignments)
				if (newAssignments[guid].player == i + 1)
					return false;
			return true;
		});
		if (firstFreeSlot === -1)
			return;

		this.assignClient(newGUID, firstFreeSlot + 1);
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
	 * Called whenever a client joins or leaves or any game setting is changed.
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
		{
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player == playerIndex + 1)
				{
					this.assignClient(guid, g_PlayerAssignments[guidToAssign].player);
					break;
				}
		}
		this.assignClient(guidToAssign, playerIndex + 1);
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
		{
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player > g_GameSettings.playerCount.nbPlayers)
					Engine.AssignNetworkPlayer(g_PlayerAssignments[guid].player, "");
		}
		else if (g_PlayerAssignments.local.player > g_GameSettings.playerCount.nbPlayers)
		{
			g_PlayerAssignments.local.player = -1;
			this.updatePlayerAssignments();
		}
	}
}

PlayerAssignmentsController.prototype.ConfigNameSingleplayer =
	"playername.singleplayer";

PlayerAssignmentsController.prototype.ConfigAssignPlayers =
	"gui.gamesetup.assignplayers";

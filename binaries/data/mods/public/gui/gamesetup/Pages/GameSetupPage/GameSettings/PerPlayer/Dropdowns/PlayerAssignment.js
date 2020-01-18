PlayerSettingControls.PlayerAssignment = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.clientItemFactory = new PlayerAssignmentItem.Client();
		this.aiItemFactory = new PlayerAssignmentItem.AI();
		this.unassignedItem = new PlayerAssignmentItem.Unassigned().createItem();

		this.aiItems =
			g_Settings.AIDescriptions.filter(ai => !ai.data.hidden).map(
				this.aiItemFactory.createItem.bind(this.aiItemFactory));

		this.values = undefined;
		this.assignedGUID = undefined;
		this.fixedAI = undefined;

		this.playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
	}

	setControl()
	{
		this.dropdown = Engine.GetGUIObjectByName("playerAssignment[" + this.playerIndex + "]");
		this.label = Engine.GetGUIObjectByName("playerAssignmentText[" + this.playerIndex + "]");
	}

	onLoad(initData, hotloadData)
	{
		if (!hotloadData && !g_IsNetworked)
			this.onClientJoin("local", g_PlayerAssignments);
	}

	onClientJoin(newGUID, newAssignments)
	{
		if (!g_IsController || this.fixedAI || newAssignments[newGUID].player != -1)
			return;

		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		// Assign the client (or only buddies if prefered) to a free slot
		if (newGUID != Engine.GetPlayerGUID())
		{
			let assignOption = Engine.ConfigDB_GetValue("user", this.ConfigAssignPlayers);
			if (assignOption == "disabled" ||
			    assignOption == "buddies" && g_Buddies.indexOf(splitRatingFromNick(newAssignments[newGUID].name).nick) == -1)
				return;
		}

		for (let guid in newAssignments)
			if (newAssignments[guid].player == this.playerIndex + 1)
				return;

		if (pData.AI)
		{
			pData.AI = false;
			this.gameSettingsControl.updateGameAttributes();
			this.gameSettingsControl.setNetworkGameAttributes();
		}

		newAssignments[newGUID].player = this.playerIndex + 1;
		this.playerAssignmentsControl.assignClient(newGUID, this.playerIndex + 1);
	}

	onPlayerAssignmentsChange()
	{
		this.assignedGUID = undefined;
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
			{
				this.assignedGUID = guid;
				break;
			}

		this.playerItems = sortGUIDsByPlayerID().map(
			this.clientItemFactory.createItem.bind(this.clientItemFactory));

		this.rebuildList();
		this.updateSelection();
	}

	onMapChange(mapData)
	{
		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		this.fixedAI = mapPData && mapPData.AI || undefined;
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		if (this.fixedAI && pData.AI != this.fixedAI)
		{
			pData.AI = this.fixedAI;
			this.gameSettingsControl.updateGameAttributes();
			this.playerAssignmentsControl.unassignClient(this.playerIndex + 1);
		}
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		this.setEnabled(!this.fixedAI);
		this.updateSelection();
	}

	updateSelection()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (pData && this.values)
			this.setSelectedValue(
				this.values.Value.findIndex((value, i) =>
					this.values.Handler[i].isSelected(pData, this.assignedGUID, value)));
	}

	rebuildList()
	{
		Engine.ProfileStart("updatePlayerAssignmentsList");
		this.values = prepareForDropdown([
			...this.playerItems,
			...this.aiItems,
			this.unassignedItem
		]);

		this.dropdown.list = this.values.Caption;
		this.dropdown.list_data = this.values.Value.map((value, i) => i);
		Engine.ProfileStop();
	}

	onSelectionChange(itemIdx)
	{
		this.values.Handler[itemIdx].onSelectionChange(
			this.gameSettingsControl,
			this.playerAssignmentsControl,
			this.playerIndex,
			this.values.Value[itemIdx]);
	}

	getAutocompleteEntries()
	{
		return this.values.Autocomplete;
	}
};

PlayerSettingControls.PlayerAssignment.prototype.Tooltip =
	translate("Select player.");

PlayerSettingControls.PlayerAssignment.prototype.AutocompleteOrder = 100;

PlayerSettingControls.PlayerAssignment.prototype.ConfigAssignPlayers =
	"gui.gamesetup.assignplayers";

class PlayerAssignmentItem
{
}

{
	PlayerAssignmentItem.Client = class
	{
		createItem(guid)
		{
			return {
				"Handler": this,
				"Value": guid,
				"Autocomplete": g_PlayerAssignments[guid].name,
				"Caption": setStringTags(
					g_PlayerAssignments[guid].name,
					g_PlayerAssignments[guid].player == -1 ? this.ObserverTags : this.PlayerTags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex, guidToAssign)
		{
			let sourcePlayer = g_PlayerAssignments[guidToAssign].player - 1;
			playerAssignmentsControl.assignPlayer(guidToAssign, playerIndex);
			gameSettingsControl.assignPlayer(sourcePlayer, playerIndex);
		}

		isSelected(pData, guid, value)
		{
			return guid !== undefined && guid == value;
		}
	};

	PlayerAssignmentItem.Client.prototype.PlayerTags =
		{ "color": "white" };

	PlayerAssignmentItem.Client.prototype.ObserverTags =
		{ "color": "170 170 250" };
}

{
	PlayerAssignmentItem.AI = class
	{
		createItem(ai)
		{
			let aiName = translate(ai.data.name);
			return {
				"Handler": this,
				"Value": ai.id,
				"Autocomplete": aiName,
				"Caption": setStringTags(sprintf(this.Label, { "ai": aiName }), this.Tags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex, value)
		{
			playerAssignmentsControl.unassignClient(playerIndex + 1);

			g_GameAttributes.settings.PlayerData[playerIndex].AI = value;

			gameSettingsControl.updateGameAttributes();
			gameSettingsControl.setNetworkGameAttributes();
		}

		isSelected(pData, guid, value)
		{
			return !guid && pData.AI && pData.AI == value;
		}
	};

	PlayerAssignmentItem.AI.prototype.Label =
		translate("AI: %(ai)s");

	PlayerAssignmentItem.AI.prototype.Tags =
		{ "color": "70 150 70" };
}

{
	PlayerAssignmentItem.Unassigned = class
	{
		createItem()
		{
			return {
				"Handler": this,
				"Value": undefined,
				"Autocomplete": this.Label,
				"Caption": setStringTags(this.Label, this.Tags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex)
		{
			playerAssignmentsControl.unassignClient(playerIndex + 1);

			g_GameAttributes.settings.PlayerData[playerIndex].AI = false;
			gameSettingsControl.updateGameAttributes();
			gameSettingsControl.setNetworkGameAttributes();
		}

		isSelected(pData, guid, value)
		{
			return !guid && !pData.AI;
		}
	};

	PlayerAssignmentItem.Unassigned.prototype.Label =
		translate("Unassigned");

	PlayerAssignmentItem.Unassigned.prototype.Tags =
		{ "color": "140 140 140" };
}

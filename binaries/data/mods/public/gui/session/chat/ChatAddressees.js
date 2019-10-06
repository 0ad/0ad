/**
 * This class is concerned with building and propagating the chat addressee selection.
 */
class ChatAddressees
{
	constructor()
	{
		this.selectionChangeHandlers = [];

		this.chatAddressee = Engine.GetGUIObjectByName("chatAddressee");
		this.chatAddressee.onSelectionChange = this.onSelectionChange.bind(this);
	}

	registerSelectionChangeHandler(handler)
	{
		this.selectionChangeHandlers.push(handler);
	}

	onSelectionChange()
	{
		let selection = this.getSelection();
		for (let handler of this.selectionChangeHandlers)
			handler(selection);
	}

	getSelection()
	{
		return this.chatAddressee.list_data[this.chatAddressee.selected] || "";
	}

	select(command)
	{
		this.chatAddressee.selected = this.chatAddressee.list_data.indexOf(command);
	}

	onUpdatePlayers()
	{
		// Remember previously selected item
		let selectedName = this.getSelection();
		selectedName = selectedName.startsWith("/msg ") && selectedName.substr("/msg ".length);

		let addressees = this.AddresseeTypes.filter(
			addresseeType => addresseeType.isSelectable()).map(
				addresseeType => ({
					"label": translateWithContext("chat addressee", addresseeType.label),
					"cmd": addresseeType.command
				}));

		// Add playernames for private messages
		let guids = sortGUIDsByPlayerID();
		for (let guid of guids)
		{
			if (guid == Engine.GetPlayerGUID())
				continue;

			let playerID = g_PlayerAssignments[guid].player;

			// Don't provide option for PM from observer to player
			if (g_IsObserver && !isPlayerObserver(playerID))
				continue;

			let colorBox = isPlayerObserver(playerID) ? "" : colorizePlayernameHelper("■", playerID) + " ";

			addressees.push({
				"cmd": "/msg " + g_PlayerAssignments[guid].name,
				"label": colorBox + g_PlayerAssignments[guid].name
			});
		}

		// Select mock item if the selected addressee went offline
		if (selectedName && guids.every(guid => g_PlayerAssignments[guid].name != selectedName))
			addressees.push({
				"cmd": "/msg " + selectedName,
				"label": sprintf(translate("\\[OFFLINE] %(player)s"), { "player": selectedName })
			});

		let oldChatAddressee = this.getSelection();
		this.chatAddressee.list = addressees.map(adressee => adressee.label);
		this.chatAddressee.list_data = addressees.map(adressee => adressee.cmd);
		this.chatAddressee.selected = Math.max(0, this.chatAddressee.list_data.indexOf(oldChatAddressee));
	}
}

ChatAddressees.prototype.AddresseeTypes = [
	{
		"command": "",
		"isSelectable": () => true,
		"label": markForTranslationWithContext("chat addressee", "Everyone"),
		"isAddressee": () => true
	},
	{
		"command": "/allies",
		"isSelectable": () => !g_IsObserver,
		"label": markForTranslationWithContext("chat addressee", "Allies"),
		"context": markForTranslationWithContext("chat message context", "Ally"),
		"isAddressee":
			senderID =>
				g_Players[senderID] &&
				g_Players[Engine.GetPlayerID()] &&
				g_Players[senderID].isMutualAlly[Engine.GetPlayerID()],
	},
	{
		"command": "/enemies",
		"isSelectable": () => !g_IsObserver,
		"label": markForTranslationWithContext("chat addressee", "Enemies"),
		"context": markForTranslationWithContext("chat message context", "Enemy"),
		"isAddressee":
			senderID =>
				g_Players[senderID] &&
				g_Players[Engine.GetPlayerID()] &&
				g_Players[senderID].isEnemy[Engine.GetPlayerID()],
	},
	{
		"command": "/observers",
		"isSelectable": () => true,
		"label": markForTranslationWithContext("chat addressee", "Observers"),
		"context": markForTranslationWithContext("chat message context", "Observer"),
		"isAddressee": senderID => g_IsObserver
	},
	{
		"command": "/msg",
		"isSelectable": () => false,
		"label": undefined,
		"context": markForTranslationWithContext("chat message context", "Private"),
		"isAddressee": (senderID, addresseeGUID) => addresseeGUID == Engine.GetPlayerGUID()
	}
];

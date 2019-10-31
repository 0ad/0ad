class OutOfSyncNetwork extends SessionMessageBox
{
	constructor()
	{
		super();
		registerNetworkOutOfSyncHandler(this.onNetworkOutOfSync.bind(this));
	}

	/**
	 * The message object is constructed in CNetClientTurnManager::OnSyncError.
	 */
	onNetworkOutOfSync(msg)
	{
		let txt = [
			sprintf(translate("Out-Of-Sync error on turn %(turn)s."), {
				"turn": msg.turn
			}),

			sprintf(translateWithContext("Out-Of-Sync", "Players: %(players)s"), {
				"players": msg.players.join(translateWithContext("Separator for a list of players", ", "))
			}),

			msg.hash == msg.expectedHash ?
				translateWithContext("Out-Of-Sync", "Your game state is identical to the hosts game state.") :
				translateWithContext("Out-Of-Sync", "Your game state differs from the hosts game state."),

			""
		];

		if (msg.turn > 1 && g_GameAttributes.settings.PlayerData.some(pData => pData && pData.AI))
			txt.push(translateWithContext("Out-Of-Sync", "Rejoining Multiplayer games with AIs is not supported yet!"));
		else
			txt.push(
				translateWithContext("Out-Of-Sync", "Ensure all players use the same mods."),
				translateWithContext("Out-Of-Sync", 'Click on "Report a Bug" in the main menu to help fix this.'),
				sprintf(translateWithContext("Out-Of-Sync", "Replay saved to %(filepath)s"), {
					"filepath": escapeText(msg.path_replay)
				}),
				sprintf(translateWithContext("Out-Of-Sync", "Dumping current state to %(filepath)s"), {
					"filepath": escapeText(msg.path_oos_dump)
				}));

		this.Caption = txt.join("\n");
		this.display();
	}
}

OutOfSyncNetwork.prototype.Width = 600;
OutOfSyncNetwork.prototype.Height = 280;

OutOfSyncNetwork.prototype.Title = translate("Out of Sync");

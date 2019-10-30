class OutOfSyncReplay extends SessionMessageBox
{
	constructor()
	{
		super();
		Engine.GetGUIObjectByName("session").onReplayOutOfSync = this.onReplayOutOfSync.bind(this);
	}

	onReplayOutOfSync(turn, hash, expectedHash)
	{
		this.Caption = sprintf(this.Captions.join("\n"), {
			"turn": turn,
			"hash": hash,
			"expectedHash": expectedHash
		});
		this.display();
	}
}

OutOfSyncReplay.prototype.Width = 500;
OutOfSyncReplay.prototype.Height = 140;

OutOfSyncReplay.prototype.Title = translate("Out of Sync");

OutOfSyncReplay.prototype.Captions = [
	translate("Out-Of-Sync error on turn %(turn)s."),
	// Translation: This is shown if replay is out of sync
	translate("Out-Of-Sync", "The current game state is different from the original game state.")
];

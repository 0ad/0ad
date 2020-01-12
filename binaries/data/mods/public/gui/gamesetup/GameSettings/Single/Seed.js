GameSettingControls.Seed = class extends GameSettingControl
{
	onGameAttributesFinalize()
	{
		// The matchID is used for identifying rated game reports for the lobby and possibly when sharing replays.
		g_GameAttributes.matchID = Engine.GetMatchID();

		// Seed is used for map generation and simulation.
		g_GameAttributes.settings.Seed = randIntExclusive(0, Math.pow(2, 32));
		g_GameAttributes.settings.AISeed = randIntExclusive(0, Math.pow(2, 32));
	}
};

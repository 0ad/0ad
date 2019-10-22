/**
 * @file The lobby scripting code is kept separate from the rest of the session to
 * ease distribution of the game without any lobby code.
 */

/**
 * The host sends a gamelist update everytime a client joins or leaves the match.
 */
var g_LobbyGamelistReporter =
	LobbyGamelistReporter.Available() &&
	new LobbyGamelistReporter();

/**
 * The participants of a rated 1v1 match send a rating report when the winner was decided.
 */
var g_LobbyRatingReporter =
	LobbyRatingReporter.Available() &&
	new LobbyRatingReporter();

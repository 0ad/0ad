/**
 * The purpose of this class is to determine a color per playername and to apply that color,
 * escape reserved characters and add a moderator prefix when displaying playernames.
 */
class PlayerColor
{
}

/**
 * Generate a (mostly) unique color for this player based on their name.
 * @see https://stackoverflow.com/questions/3426404/create-a-hexadecimal-colour-based-on-a-string-with-jquery-javascript
 */
PlayerColor.GetPlayerColor = function(playername)
{
	// Generate a probably-unique hash for the player name and use that to create a color.
	let hash = 0;
	for (let i in playername)
		hash = playername.charCodeAt(i) + ((hash << 5) - hash);

	// First create the color in RGB then HSL, clamp the lightness so it's not too dark to read, and then convert back to RGB to display.
	// The reason for this roundabout method is this algorithm can generate values from 0 to 255 for RGB but only 0 to 100 for HSL; this gives
	// us much more variety if we generate in RGB. Unfortunately, enforcing that RGB values are a certain lightness is very difficult, so
	// we convert to HSL to do the computation. Since our GUI code only displays RGB colors, we have to convert back.
	let [h, s, l] = rgbToHsl(hash >> 24 & 0xFF, hash >> 16 & 0xFF, hash >> 8 & 0xFF);
	return hslToRgb(h, s, Math.max(0.7, l)).join(" ");
};

/**
 * Colorizes the given nickname with a color unique and deterministic for that player.
 */
PlayerColor.ColorPlayerName = function(playername, rating, role)
{
	let name = rating ?
		sprintf(translate("%(nick)s (%(rating)s)"), {
			"nick": playername,
			"rating": rating
		}) :
		playername;

	if (role == "moderator")
		name = PlayerColor.ModeratorPrefix + name;

	return coloredText(escapeText(name), PlayerColor.GetPlayerColor(playername));
};

/**
 * A symbol which is prepended to the nickname of moderators.
 */
PlayerColor.ModeratorPrefix = "@";


// TODO: Remove global required by formatPlayerInfo
var getPlayerColor = PlayerColor.GetPlayerColor;

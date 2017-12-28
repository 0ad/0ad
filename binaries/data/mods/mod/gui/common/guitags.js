/**
 * Add color to text string.
 */
function coloredText(text, color)
{
	return '[color="' + color + '"]' + text + '[/color]';
}

/**
 * Set GUI tags on text string.
 *
 * @param {string} string - String to apply tags to.
 * @param {object} tags - Object containing the tags, for instance { "color": "white" } or { "font": "sans-13" }.
 */
function setStringTags(text, tags)
{
	let result = text;

	for (let tag in tags)
		result = '[' + tag + '="' + tags[tag] + '"]' + result + '[/' + tag + ']';

	return result;
}

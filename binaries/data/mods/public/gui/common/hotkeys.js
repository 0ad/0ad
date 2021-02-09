/**
 * Holds a map of scancode name -> user keyboard name
 */
var g_ScancodesMap;

function hotkeySort(a, b)
{
	const specialKeys = ["Shift", "Alt", "Ctrl", "Super"];
	// Quick hack to put those first.
	if (specialKeys.indexOf(a) !== -1)
		a = ' ' + a;
	if (specialKeys.indexOf(b) !== -1)
		b = ' ' + b;
	return a.localeCompare(b, Engine.GetCurrentLocale().substr(0, 2), { "numeric": true });
}

function formatHotkeyCombination(comb, translateScancodes = true)
{
	if (!translateScancodes)
		return comb.sort(hotkeySort).join("+");

	if (!g_ScancodesMap)
		g_ScancodesMap = Engine.GetScancodeKeyNames();

	return comb.sort(hotkeySort).map(hk => g_ScancodesMap[hk]).join("+");
}

function formatHotkeyCombinations(combinations, translateScancodes = true)
{
	if (!combinations || !combinations.length)
		return "";

	let combs = combinations.map(x => formatHotkeyCombination(x, translateScancodes));
	combs.sort((a, b) => a.length - b.length || a - b);
	return translateScancodes ? combs.join(", ") : combs;
}

/**
 * returns a clone of a simple object or array
 * Only valid JSON objects are accepted
 * So no recursion, and only plain objects or arrays
 */
function clone(o)
{
	let r;
	if (o instanceof Array)
		r = [];
	else if (o instanceof Object)
		r = {};
	else // native data type
		return o;
	for (let key in o)
		r[key] = clone(o[key]);
	return r;
}

/**
 * "Inside-out" implementation of Fisher-Yates shuffle
 */
function shuffleArray(source)
{
	if (!source.length)
		return [];

	let result = [source[0]];
	for (let i = 1; i < source.length; ++i)
	{
		let j = Math.floor(Math.random() * (i+1));
		result[i] = result[j];
		result[j] = source[i];
	}
	return result;
}

/**
 * Removes prefixing path from a path or filename, leaving just the file's name (with extension)
 *
 * ie. a/b/c/file.ext -> file.ext
 */
function basename(path)
{
	return path.slice(path.lastIndexOf("/") + 1);
}

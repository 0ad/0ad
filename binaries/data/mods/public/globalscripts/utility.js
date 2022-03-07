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
		let j = randIntInclusive(0, i);
		result[i] = result[j];
		result[j] = source[i];
	}
	return result;
}

/**
 * Generates each permutation of the given array and runs the callback function on it.
 * Uses the given clone function on each item of the array.
 * Creating arrays with all permutations of the given array has a bad memory footprint.
 * Algorithm by B. R. Heap. Changes the input array.
 */
function heapsPermute(array, cloneFunc, callback)
{
	let c = new Array(array.length).fill(0);

	callback(array.map(cloneFunc));

	let i = 0;
	while (i < array.length)
	{
		if (c[i] < i)
		{
			let swapIndex = i % 2 ? c[i] : 0;
			let swapValue = cloneFunc(array[swapIndex]);
			array[swapIndex] = array[i];
			array[i] = swapValue;

			callback(array.map(cloneFunc));

			++c[i];
			i = 0;
		}
		else
		{
			c[i] = 0;
			++i;
		}
	}
}

/**
 * Compare two variables recursively. This compares better than a quick
 * JSON.stringify check since we also check undefineds, Sets and the like.
 *
 * @param first - Any javascript instance.
 * @param second - Any javascript instance.
 * @return {boolean} Whether first and second are equal.
 */
function deepCompare(first, second)
{
	// If the value of either variable is empty we can instantly compare
	// them and check for equality.
	if (first === null || first === undefined ||
	    second === null || second === undefined)
		return first === second;

	// Make sure both variables have the same type.
	if (first.constructor !== second.constructor)
		return false;

	// We know that the variables are of the same type so all we need to do is
	// check what type one of the objects is, and then compare them.

	// Check numbers seperately. Make sure this works with NaN, Infinity etc.
	if (typeof first == "number")
		return uneval(first) === uneval(second);

	// Functions and RegExps must have the same reference to be equal.
	if (first instanceof Function || first instanceof RegExp)
		return first === second;

	// If we are comparing simple objects, we can just compare them.
	if (first === second || first.valueOf() === second.valueOf())
		return true;

	// Dates would have equal valueOf if they are equal.
	if (first instanceof Date)
		return false;

	// From now we will assume we have some kind of objects so that
	// we can do a recursive check of the keys and values.
	if (!(first instanceof Object) || !(second instanceof Object))
		return false;

	// We cannot iterate over Sets, so collapse them to Arrays.
	if (first instanceof Set)
	{
		first = Array.from(first);
		second = Array.from(second);
	}

	// Objects need the same number of keys in order to be equal.
	const firstKeys = Object.keys(first);
	if (firstKeys.length != Object.keys(second).length)
		return false;

	// Make sure that all the object keys on this level of the object are the same.
	// Finally, we pass all the values of our of each object recursively to
	// make sure everything matches.
	return Object.keys(second).every(i => firstKeys.includes(i)) &&
	       firstKeys.every(i => deepCompare(first[i], second[i]));
}

/**
 * Removes prefixing path from a path or filename, leaving just the file's name (with extension)
 *
 * ie. a/b/c/file.ext -> file.ext
 */
function basename(path)
{
	return path.split("/").pop();
}

/**
 * Returns the directories of a given path.
 *
 * ie. a/b/c/file.ext -> a/b/c
 */
function dirname(path)
{
	return path.split("/").slice(0, -1).join("/");
}

/**
 * Returns names of files found in the given directory, stripping the directory path and file extension.
 */
function listFiles(path, extension, recurse)
{
	return Engine.ListDirectoryFiles(path, "*" + extension, recurse).map(filename => filename.slice(path.length, -extension.length));
}


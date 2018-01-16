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
 * Removes prefixing path from a path or filename, leaving just the file's name (with extension)
 *
 * ie. a/b/c/file.ext -> file.ext
 */
function basename(path)
{
	return path.slice(path.lastIndexOf("/") + 1);
}

/**
 * Returns names of files found in the given directory, stripping the directory path and file extension.
 */
function listFiles(path, extension, recurse)
{
	return Engine.ListDirectoryFiles(path, "*" + extension, recurse).map(filename => filename.slice(path.length, -extension.length));
}


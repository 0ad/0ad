const MatchSort = (function() {
const Highscore = -10E7;

return class
{
	/**
	 * Returns a new list filtered and sorted by the similarity with the input text
	 * Order of sorting:
	 * 1. Exact match
	 * 2. Exact lowercase match
	 * 3. Starting letters match and sorted alphabetically
	 * 4. By similarity score (lookahead match)
	 * 5. Entry is discarded if one of the previous don't apply
	 *
	 * @param {string} input text to seach for
	 * @param {string[] | object[]} list
	 * @param {string} [key] text to use if the list is made up of objects
	 */
	static get(input, list, key)
	{
		input = input.toLowerCase();

		let result = [];

		for (let obj of list)
		{
			let text = key == null ? obj : obj[key];
			let score = MatchSort.scoreText(input, text);
			if (score !== undefined)
				result.push([obj, score, text, text.startsWith(input)]);
		}

		return result.sort(MatchSort.sort).map(v => v[0]);
	}

	static sort([o1, s1, t1, a1], [o2, s2, t2, a2])
	{
		if (a1 && a2)
			return t1.localeCompare(t2);

		if (a1)
			return -1;

		if (a2)
			return 1;

		return s1 - s2;
	}

	/**
	 * The lower the score the better the match.
	 */
	static scoreText(input, text)
	{
		// Exact match.
		if (input == text)
			return Highscore;

		text = text.toLowerCase();

		// Exact match relaxed.
		if (input == text)
			return Highscore / 2;

		let score = 0;
		let offset = -1;

		for (let i = 0; i < input.length; ++i)
		{
			let offsetNext = text.indexOf(input[i], offset + 1);

			// No match.
			if (offsetNext == -1)
				return undefined;

			// Lower score increase if consecutive index.
			let isConsecutive = offsetNext == offset + 1 ? 0 : 1;
			score += offsetNext + isConsecutive * offsetNext;
			offset = offsetNext;
		}

		return score;
	}
};
})();

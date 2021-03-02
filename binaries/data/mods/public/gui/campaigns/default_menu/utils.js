/**
 * Various utilities.
 */
function markLevelComplete(run, levelID)
{
	if (!isCompleted(run, levelID))
	{
		if (!run.data.completedLevels)
			run.data.completedLevels = [];
		run.data.completedLevels.push(levelID);
		run.save();
	}
}

function isCompleted(run, levelID)
{
	return run.data.completedLevels && run.data.completedLevels.indexOf(levelID) !== -1;
}

function meetsRequirements(run, levelData)
{
	if (!levelData.Requires)
		return true;

	return MatchesClassList(run.data.completedLevels || [], levelData.Requires);
}

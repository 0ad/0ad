function hasRequirements(scenario)
{
	if (!scenario.Requires)
		return true;

	if (!g_CampaignData.completed)
		return false;

	if (scenario.Requires.some(req => g_CampaignData.completed.indexOf(req) === -1))
		return false;

	return true;
}

function hasCompleted(scenario)
{
	if (!g_CampaignData.completed)
		return false;

	if (g_CampaignData.completed.indexOf(scenario.ID) === -1)
		return false;

	return true;
}

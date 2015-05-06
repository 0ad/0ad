let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

for (let i = 1; i < cmpPlayerManager.GetNumPlayers(); ++i)
{
	let cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
	if (cmpPlayer)
		cmpPlayer.AddDisabledTemplate("structures/ptol_lighthouse");
}

/*
	DESCRIPTION	: Script called when the game is started, which should initialize the players and their units based on the game settings.
	NOTES		: 
*/

// Assign the players resources

for(var i=0; i<players.length; i++)
{
	var p = players[i];
	p.resources = new Object();
	
	switch( g_GameAttributes.resourceLevel.toLowerCase() )
	{
	case "high":
		p.resources.food = 1000;
		p.resources.wood = 1000;
		p.resources.metal = 500;
		p.resources.stone = 500;
		break;
	default:
		p.resources.food = 200;
		p.resources.wood = 200;
		p.resources.metal = 100;
		p.resources.stone = 100;
		break;
	}
	
	p.resources.population = 0;
	p.resources.housing = 0;
}

// Give the players their civ techs (which will set up their civ bonuses)
for(var i=0; i<players.length; i++)
{
	var tech = getTechnology( "civ_" + players[i].civilization.toLowerCase(), players[i] );
	if( tech != null )
	{
		tech.applyEffects( false, false );
	}
}
				
						
/*var gameMode = getGameMode();
//Create end game trigger based on game type

var endGameConquestFunction = 
function() 
{
	TODO: Needs player alliance information with team numbers for determining defeated teams.  
			Also needs local player ID to end game on 'this' computer.  Also, receive pplayer set
			size from somewhere, don't assume 6
	
	var livePlayers = new Array();
	for ( var i = 1; i < 7; ++i )
	{
		if ( trigPlayerSigEntities(i) <= 0 )
		{
			//if ( isPlayerAlive(i) == true ) { killPlayer(i); }
		}
		else
			livePlayers[i] = true;
	}
	
	var gameOver = true;
	var playerSet = getPlayerSet();
	
	//Go through and find team numbers (of alliance) - test against every other team number for enemies - if found, game is not done.
	for ( var i = 0; i < livePlayers.length; ++i )
	{
		for ( var j = 0; j < livePlayers.length; ++j )
		{
			if ( playerSet[i].getDiplomaticStance(j) == DIPLOMACY_ENEMY  )
				return;
		}
	}
	endGame()
}
	
				
if ( gameMode == "Conquest" )
{
	registerTrigger( 
		Trigger("END_GAME_TRIGGER", true, 0.0, -1, endGameConquestFunction, trigEndGame ) );
}
*/

console.write( "Game startup script done." );


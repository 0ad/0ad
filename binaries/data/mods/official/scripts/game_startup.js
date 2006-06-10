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
		p.resources.ore = 500;
		p.resources.stone = 500;
		break;
	default:
		p.resources.food = 200;
		p.resources.wood = 200;
		p.resources.ore = 100;
		p.resources.stone = 100;
		break;
	}
	
	p.resources.population = 0;
	p.resources.housing = 0;
}

// Give the players their civ techs (which will set up their civ bonuses)
for(var i=0; i<players.length; i++)
{
	var tech = getTechnology( "civ_" + players[i].civilization.toLowerCase() );
	if( tech != null )
	{
		tech.applyEffects( i, false, false );
	}
}

console.write( "Game startup script done." );


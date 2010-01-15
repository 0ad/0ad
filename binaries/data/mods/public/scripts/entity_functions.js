/*
	DESCRIPTION	: Functions that define the scripted behaviour of properties, particularly the effects of entity events and their properties when initialised.
	NOTES		: 
*/

// ====================================================================

// To add a new ContactAction order, do the following all within template_entity_script.js:

//    * Pick a number to be its ID (add this to the "const"s directly below). f

//    * Add code in the entityInit() function below that will call setActionParams to set the action's range, speed and animation if the entity supports this action. For example this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed, "gather" ) tells the entity that the action with ID of ACTION_GATHER has min range 0, max range a.range, speed a.speed, and should play the animation called "gather" while active. 

//    * Add code in entityEventTargetChanged() to tell the GUI whether the entity should use this action depending on what target the mouse is hovering over. This is also where you can set a cursor for the action. 

//    * Add code in entityEventContactAction() to deal with new ContactAction order events of your type. Note that if you want to have your action handler in a separate function (is preferable), you need to also add this function to the entity object in entityInit() (its initialize event), e.g. this.processGather = entityEventGather. 

const ACTION_NONE	= 0;
const ACTION_ATTACK	= 1;
const ACTION_GATHER	= 2;
const ACTION_HEAL	= 3;
const ACTION_ATTACK_RANGED = 4;
const ACTION_BUILD = 5;
const ACTION_REPAIR = 6;

const PRODUCTION_TRAIN = 1;
const PRODUCTION_RESEARCH = 2;


// ====================================================================

function entityInit( evt )
{
	// Initialise an entity when it is first spawned (generate starting hitpoints, etc).
	
	// This function is called for all "full" entities - those inheriting from template_entity_full; there is a simpler version below
	// called entityInitQuasi for quasi-entities (rocks, trees, etc) for which most of the things dealt with here don't apply.

    startXTimer(1);

	// If the entity is a foundation, we must deduct resource costs here
	if( this.building )
	{
		var template = getEntityTemplate( this.building, this.player );
		var result = checkEntityReqs( this.player, template );	

		if (result == true) // If the entry meets requirements to be added to the queue (eg sufficient resources) 
		{
			// Cycle through all costs of this entry.
			var pool = template.traits.creation.resource;
			for ( resource in pool )
			{
				switch ( resource.toString() )
				{
					case "population":
					case "housing":
					break;
					default:
						// Deduct the given quantity of resources.
						this.player.resources[resource.toString()] -= parseInt(pool[resource]);

						//console.write ("Spent " + pool[resource] + " " + resource + " to build " + 
						//	template.traits.id.generic);
					break;
				}
			}
		}
		else
		{
			// Might happen if the player clicks to place 2 buildings really fast
			evt.preventDefault();
			stopXTimer(1);
			return;
		}
	}
	
	stopXTimer(1);
	startXTimer(2);
	
	// Attach our functions to ourselves
	this.getAttackAction = getAttackAction;		// Note: required by CEntity
	this.performAttack = performAttack;
	this.performAttackRanged = performAttackRanged;
	this.performGather = performGather;
	this.performHeal = performHeal;
	this.performBuild = performBuild;
	this.performRepair = performRepair;
	this.damage = damage;
	this.entityComplete = entityComplete;
	this.GotoInRange = GotoInRange;
	this.attachAuras = attachAuras;
	this.setupRank = setupRank;
	this.chooseGatherTarget = chooseGatherTarget;
	
	// Some temp variables to speed up property access
	var id = this.traits.id;
	var health = this.traits.health;
	var stamina = this.traits.stamina;
	
	// If this is a foundation, initialize traits from the building we're converting into
	if( this.building && this.building != "" )
	{
		var building = getEntityTemplate( this.building, this.player );
		id.generic = building.traits.id.generic;
		id.specific = building.traits.id.specific;
		id.civ = building.traits.id.civ;
		id.icon_cell = building.traits.id.icon_cell;
		this.traits.health.max = building.traits.health.max;
		this.buildPoints = new Object();
		this.buildPoints.curr = 0.0;
		this.buildPoints.max = parseFloat( building.traits.creation.time );
		this.traits.creation.buildingLimitCategory = building.traits.creation.buildingLimitCategory;
		if( building.traits.supply )
		{
			this.traits.supply = new Object();
			this.traits.supply.max = building.traits.supply.max;
			this.traits.supply.curr = building.traits.supply.max;
			this.traits.supply.type = building.traits.supply.type;
			this.traits.supply.subType = building.traits.supply.subType;
		}
	}
	
	// Generate civ code (1st four characters of civ name, in lower case eg "Carthaginians" => "cart").
	if( !id.civ_code && id.civ )
	{
		var civ_code = id.civ.toString().substring (0, 4);
		civ_code = civ_code.toString().toLowerCase();

		// Exception to make the Romans into rome and Hellenes into hele.
		if (civ_code == "roma") civ_code = "rome";
		else if (civ_code == "hell") civ_code = "hele";
		
		id.civ_code = civ_code;
	}

	// If entity can contain garrisoned units, empty it.
	if ( this.traits.garrison && this.traits.garrison.max )
		this.traits.garrison.curr = 0;

	// If entity has health, set current to same, unless it's a foundation, in which case we set it to 0.
	if ( health && health.max  )
		health.curr = ( this.building!="" ? 0.0 : health.max );

	// If entity has stamina, set current to same.
	if ( stamina && stamina.max )
		stamina.curr = stamina.max

    stopXTimer(2);
    startXTimer(3);

	var supply = this.traits.supply;
	if( supply )
	{
		// If entity has supply, set current to same.
		supply.curr = supply.max;

		// If entity has type of supply and no subType, set subType to same
		// (so we don't have to say type="wood", subType="wood"
		if (supply.type && !supply.subType)
			supply.subType = supply.type;
			
		// The "dropsiteCount" array holds the number of units with gather aura in range of the object;
		// this is important so that if you have two mills near something and one of them is destroyed,
		// you can still gather from the thing. Initialize it to 0 (ungatherable) for every player unless
		// the entity is forageable (e.g. for huntable animals).
		var dropsiteCount = new Array();
		initialCount = supply.subType.meat ? 1 : 0;
		for( i=0; i<=8; i++ )
		{
			dropsiteCount[i] = initialCount;
		}
		supply.dropsiteCount = dropsiteCount;
	}

	if (!this.traits.promotion)
		this.traits.promotion = new Object();
		
	// If entity becomes another entity after it gains enough experience points, set up secondary attributes.
	if (this.traits.promotion.req)
	{
		this.setupRank();
			
		// Give the entity an initial value of 0 earned XP at startup if a default value is not specified.
		if (!this.traits.promotion.curr)
			this.traits.promotion.curr = 0;	
	}
	else
	{
		this.traits.promotion.rank = "0";
	}
	
	stopXTimer(3);
	startXTimer(4);
	
	// Register our actions with the ContactAction order system
	if( this.actions )
	{
		if ( this.actions.move && this.actions.move.speed )
			this.actions.move.speedCurr = this.actions.move.speed;
			
		if( this.actions.attack && this.actions.attack.melee )
		{
			a = this.actions.attack.melee;
			minRange = ( a.minRange ? a.minRange : 0.0 );
			this.setActionParams( ACTION_ATTACK, minRange, a.range, a.speed, "melee" );
		}
		if( this.actions.gather )
		{
			a = this.actions.gather;
			this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed, "gather" );
		}
		if( this.actions.heal )
		{
			a = this.actions.heal;
			this.setActionParams( ACTION_HEAL, 0.0, a.range, a.speed, "heal" );
		}
		if( this.actions.attack && this.actions.attack.ranged )
		{
			a = this.actions.attack.ranged;
			minRange = ( a.minRange ? a.minRange : 0.0 );
			// this animation should actually be "ranged" except the current actors still have it called "melee"
			this.setActionParams( ACTION_ATTACK_RANGED, minRange, a.range, a.speed, "melee" );
		}
		if( this.actions.build )
		{
			this.setActionParams( ACTION_BUILD, 0.0, 2.0, this.actions.build.speed, "build" );
		}
		if( this.actions.repair )
		{
			this.setActionParams( ACTION_REPAIR, 0.0, 2.0, this.actions.repair.speed, "build" );
		}
	}

    stopXTimer(4);
    startXTimer(5);

	this.attachAuras();
	
	// If the entity either costs population or adds to it,
	if (this.traits.population)
	{
		// If the entity increases the population limit (provides Housing),
		if (this.traits.population.add)
			this.player.resources.housing += parseInt(this.traits.population.add);
		// If the entity takes up Housing,
		if (this.traits.population.rem)
			this.player.resources.population += parseInt(this.traits.population.rem);
	}
	
	// Build Unit Ai Stance list, and set default stance.  ---> Can eventually be done in C++, since stances will likely be implemented in C++.
	if (this.actions && this.actions.move)
	{
		if ( !this.traits.ai.stance.list )
			this.traits.ai.stance.list = new Object();
		// Create standard stances that all units have.
		this.traits.ai.stance.list.avoid = new Object();
		this.traits.ai.stance.list.hold = new Object();	
		if ( this.actions && this.actions.attack )
		{
			// Create stances that units only have if they can attack.
			this.traits.ai.stance.list.aggress = new Object();	
			this.traits.ai.stance.list.defend = new Object();
			this.traits.ai.stance.list.stand = new Object();
			// Set default stance for combat units.
			this.traits.ai.stance.curr = "aggress";
		}
	}
	
	// Set up allure counter
	if (this.actions && this.actions.gather && this.actions.gather.affectedByAllure)
	{
		this.allureCount = 0;
	}
	
	stopXTimer(5);
/*	
	// Generate entity's personal name (if it needs one).
	if (this.traits.id.personal)
	{
		// todo

		// Look in the appropriate array for a random string.

		switch (this.traits.id.classes[])
		{
			case "Male":
				this.traits.id.personal.table1 = "Male"
			break;
			case "Female":
				this.traits.id.personal.table1 = "Female"
			break;
			default:
				this.traits.id.personal.table1 = "Gen"
			break;	
		}
		
		
		this.traits.id.personal.name = getRandom (this.traits.id.civ_code + this.traits.id.personal.table1 + "1")
		this.traits.id.personal.name = this.traits.id.personal.name + " " + getRandom (this.traits.id.civ_code + this.traits.id.personal.table1 + "2")
	}
	else
		this.traits.id.personal.name = "";
*/
	// Log creation of entity to console.
	//if (this.traits.id.personal && this.traits.id.personal.name)
	//	console.write ("A new " + this.traits.id.specific + " (" + this.traits.id.generic + ") called " + this.traits.id.personal.name + " has entered your dungeon.")
	//else	
	//	console.write ("A new " + this.traits.id.specific + " (" + this.traits.id.generic + ") has entered your dungeon.")
}

// ====================================================================

function entityInitQuasi()
{
	// Initialization function for quasi-entities like trees, rocks, etc; only sets up resources

    startXTimer(6);

	var supply = this.traits.supply;
	if( supply )
	{
		// If entity has supply, set current to same.
		supply.curr = supply.max;

		// If entity has type of supply and no subType, set subType to same
		// (so we don't have to say type="wood", subType="wood"
		if (supply.type && !supply.subType)
			supply.subType = supply.type;
			
		// The "dropsiteCount" array holds the number of units with gather aura in range of the object;
		// this is important so that if you have two mills near something and one of them is destroyed,
		// you can still gather from the thing. Initialize it to 0 (ungatherable) for every player unless
		// the entity is forageable (e.g. for huntable animals).
		var dropsiteCount = new Array();
		initialCount = supply.subType.meat ? 1 : 0;
		for( i=0; i<=8; i++ )
		{
			dropsiteCount[i] = initialCount;
		}
		supply.dropsiteCount = dropsiteCount;
	}
	
	stopXTimer(6);
	startXTimer(7);
	
	// Generate civ code (1st four characters of civ name, in lower case eg "Carthaginians" => "cart").
	var id = this.traits.id;
	if( !id.civ_code && id.civ )
	{
		var civ_code = id.civ.toString().substring (0, 4);
		civ_code = civ_code.toString().toLowerCase();

		// Exception to make the Romans into rome and Hellenes into hele.
		if (civ_code == "roma") civ_code = "rome";
		else if (civ_code == "hell") civ_code = "hele";
		
		id.civ_code = civ_code;
	}
	
	/*
	// Original unoptimized version of the above code (for reference)
	if( this.traits.id && this.traits.id.civ )
	{
		var id = this.traits.id; 
		id.civ_code = id.civ.toString().substring (0, 4);
		id.civ_code = id.civ_code.toString().toLowerCase();

		// Exception to make the Romans into rome.
		if (id.civ_code == "roma") id.civ_code = "rome";
		// Exception to make the Hellenes into hele.
		if (id.civ_code == "hell") id.civ_code = "hele";
	}
	*/
	
	this.damage = damage;
	
	stopXTimer(7);
}

// ====================================================================

// A special version of the entity init function that causes the unit to automatically start gathering
// nearby resources, for use in gameplay cinematics before full CPU player AI is implemented.
function entityInitGatherer( evt )
{
	// Call the original entityInit function
	this.entityInit = entityInit;
	this.entityInit( evt );
	
	// After initialization is complete, start gatherering the nearest thing we can;
	// this has to be done later using setTimeout to ensure that other entities are initialized
	setTimeout( 
		function() {
			this.chooseGatherTarget( null, this.getVisibleEntities() );
		}, 
		100, this
	);
}

// ====================================================================

// Setup entity's next rank
function setupRank()
{
	// Get the name of the entity. 
	entityName = this.tag.toString();
	
	// For accessing the this.traits.promotion object quicker
	var promotion = this.traits.promotion;

	// Determine whether current is basic, advanced or elite, and set rank to suit.
	switch (entityName.substring (entityName.length-2, entityName.length))
	{
		case "_b":
			// Basic. Upgrades to Advanced.
			promotion.rank = "1";
			// Set rank image to put over entity's head.
			this.traits.rank.name = "";
		break;
		case "_a":
			// Advanced. Upgrades to Elite.
			promotion.rank = "2";
			// Set rank image to put over entity's head.
			this.traits.rank.name = "advanced.dds";				
		break;
		case "_e":
			// Elite. Maximum rank.
			promotion.rank = "3";
			// Set rank image to put over entity's head.
			this.traits.rank.name = "elite.dds";				
		break;
		default:
			// Does not gain promotions.
			promotion.rank = "0"
		break;
	}

	// If entity is an additional rank and the correct actor has not been specified
	// (it's just inherited the Basic), point it to the correct suffix. (Saves us specifying it each time.)
	/*actorStr = this.actor.toString();
	if (promotion.rank > "1"
		&& actorStr.substring (actorStr.length-5, actorStr.length) != nextSuffix + ".xml")
		this.actor = actorStr.substring (1,actorStr.length-5) + nextSuffix + ".xml";*/
}

// ====================================================================

// Attach any auras the entity is entitled to. This was moved to a separate function so that buildings can have their auras 
// attached to them only when they finish construction.
function attachAuras() 
{
	// Add all auras defined in this.traits.auras
	var t = this.traits.auras;
	var a;
	if( t )
	{
		if( t.courage )
		{
			a = t.courage;
			this.addAura ( "courage", a.radius, 0, a.r, a.g, a.b, a.a, new DamageModifyAura( this, true, a.bonus ) );
		}
		if( t.fear )
		{
			a = t.fear;
			this.addAura ( "fear", a.radius, 0, a.r, a.g, a.b, a.a, new DamageModifyAura( this, false, -a.bonus ) );
		}
		if( t.infidelity )
		{
			a = t.infidelity;
			this.addAura ( "infidelity", a.radius, 0, a.r, a.g, a.b, a.a, new InfidelityAura( this, a.time ) );
		}
		if( t.dropsite )
		{
			a = t.dropsite;
			this.addAura ( "dropsite", a.radius, 0, a.r, a.g, a.b, a.a, new DropsiteAura( this, a.types ) );
		}
		if( t.heal )
		{
			a = t.heal;
			this.addAura ( "heal", a.radius, a.speed, a.r, a.g, a.b, a.a, new HealAura( this ) );
		}
		if( t.trample )
		{
			a = t.trample;
			this.addAura ( "trample", a.radius, a.speed, a.r, a.g, a.b, a.a, new TrampleAura( this ) );
		}
		if( t.allure )
		{
			a = t.allure;
			this.addAura ( "allure", a.radius, 0, a.r, a.g, a.b, a.a, new AllureAura( this ) );
		}
	}		
	
	// Settlements also get a special aura that will let them change player when a Civ Centre is built on top
	if( this.hasClass("Settlement") )
	{
		this.addAura ( "settlement", 1.0, 0, 0.0, 0.0, 0.0, 0.0, new SettlementAura( this ) );
	}
}

// ====================================================================

function entityDestroyed( evt )
{
	// If the entity either costs population or adds to it,
	if (this.traits.population)
	{
		// If the entity increases the population limit (provides Housing),
		if (this.traits.population.add)
			this.player.resources.housing -= parseInt(this.traits.population.add);
		// If the entity occupies population slots (occupies Housing),
		if (this.traits.population.rem)
			this.player.resources.population -= parseInt(this.traits.population.rem);
	}
}

// ====================================================================

function foundationDestroyed( evt )
{
	if( this.building != "" )	// Check that we're *really* a foundation since the event handler is kept when we change templates (probably a bug)
	{
		//console.write( "Hari Seldon made a small calculation error." );
		
		var bp = this.buildPoints;
		var fractionToReturn = (bp.max - bp.curr) / bp.max;
		
		var resources = getEntityTemplate( this.building, this.player ).traits.creation.resource;
		for( r in resources )
		{
			amount = parseInt( fractionToReturn * parseInt(resources[r]) );
			this.player.resources[r.toString()] += amount;
		}
	}
}

// ====================================================================

function performAttack( evt )
{
	this.lastCombatTime = getGameTime();
	
	if(getGUIGlobal().newRandomSound) {
		var curr_hit = getGUIGlobal().newRandomSound("voice", "hit", this.traits.audio.path);
		curr_hit.play();
	}

	// Attack logic.
	var dmg = new DamageType();
	if ( this.getRunState() )
	{
		console.write("" + this + " doing a charge attack!");
		var a = this.actions.attack.charge;
		dmg.crush = a.crush;
		dmg.hack = a.hack;
		dmg.pierce = a.pierce;
		this.setRun( false );
	}
	else
	{
		var a = this.actions.attack.melee;
		dmg.crush = a.crush;
		dmg.hack = a.hack;
		dmg.pierce = a.pierce;
	}
	
	// Add flank penalty
	var bonus = 1.0;
	if(evt.target.traits.flankPenalty)
		bonus += (evt.target.getAttackDirections()-1) * evt.target.traits.flankPenalty.value;
	bonus += getElevationBonus(this.getHeight() - evt.target.getHeight());
	dmg.crush *= bonus;
	dmg.hack *= bonus;
	dmg.pierce *= bonus;

	evt.target.damage( dmg, this );
	
}

// ====================================================================

function performAttackRanged( evt )
{
	this.lastCombatTime = getGameTime();

	// Create a projectile from us, to the target, that will do some damage when it hits them.
	dmg = new DamageType();
	var a = this.actions.attack.ranged;
	dmg.crush = a.crush;
	dmg.hack = a.hack;
	dmg.pierce = a.pierce;
	
	// Add flank penalty and elevation bonus
	var bonus = 1.0;
	if(evt.target.traits.flankPenalty)
		bonus += (evt.target.getAttackDirections()-1) * evt.target.traits.flankPenalty.value;
	bonus += getElevationBonus(this.getHeight() - evt.target.getHeight());
	dmg.crush *= bonus;
	dmg.hack *= bonus;
	dmg.pierce *= bonus;

	// The parameters for Projectile are:	
	// 1 - The actor to use as the projectile. There are two ways of specifying this:
	//     the first is by giving an entity. The projectile's actor is found by looking
	//     in the actor of that entity. This way is usual, and preferred - visual
	//     information, like the projectile model, should be stored in the actor files.
	//     The second way is to give a actor/file name string (e.g. "props/weapon/weap_
	//     arrow_front.xml"). This is only meant to be used for 'Acts of Gaia' where
	//     there is no originating entity. Right now, this entity is the one doing the
	//     firing, so pass this.
	// 2 - Where the projectile is coming from. This can be an entity or a Vector3D.
	//     For now, that's also us.
	// 3 - Where the projectile is going to. Again, either a vector (attack ground?) 
	//     or an entity. Let's shoot at our target, lest we get people terribly confused.
	// 4 - How fast the projectile should go. To keep things clear, we'll set it to 
	//     just a bit faster than the average cavalry.
	// 5 - Who fired it? Erm... yep, us again.
	// 6 - The function you'd like to call when it hits an entity.
	// There's also a seventh parameter, for a function to call when it misses (more
	//  accurately, when it hits the floor). At the moment, however, the default
	//  action (do nothing) is what we want.
	// Parameters 5, 6, and 7 are all optional.
	
	projectile = new Projectile( this, this, evt.target, 
		this.actions.attack.ranged.projectileSpeed, 
		this, projectileEventImpact )
	
	// We'll attach the damage information to the projectile, just to show you can
	// do that like you can with most other objects. Could also do this by making
	// the function we pass a closure.
	
	projectile.damage = dmg;
	
	// Finally, tell the engine not to send this event to anywhere else - 
	// in particular, this shouldn't get to the melee event handler, above.
	
	evt.stopPropagation();
	
	//console.write( "Fire!" );
}

// ====================================================================

function projectileEventImpact( evt )
{
	//console.write( "Hit!" );
	if(evt.impacted.damage)
		evt.impacted.damage( this.damage, evt.originator );
	
	// Just so you know - there's no guarantee that evt.impacted is the thing you were
	// aiming at. This function gets called when the projectile hits *anything*.
	// For example:
	
	//if( evt.impacted.player == evt.originator.player )
	//	console.write( "Friendly fire!" );
		
	// The three properties of the ProjectileImpact event are:
	// - impacted, the thing it hit
	// - originator, the thing that fired it (the fifth parameter of Projectile's
	//   constructor) - may be null
	// - position, the position the arrow was in the world when it hit.
	
	// The properties of the ProjectileMiss event (the one that gets sent to the
	// handler that was the seventh parameter of the constructor) are similar,
	// but it doesn't have 'impacted' - for obvious reasons.
}

// ====================================================================

function performGather( evt )
{
	g = this.actions.gather;
	s = evt.target.traits.supply;
	
	if( !s.dropsiteCount[this.player.id] )
	{
		// Entity has become ungatherable for us, probably meaning our mill near it was killed; cancel order
		evt.preventDefault();
		return;
	}

	var allureMod = 1;
	if( g.affectedByAllure && this.allureCount > 0 )
		allureMod = 1.2;
	
	if( g.resource[s.type][s.subType])
		gather_amt = parseFloat( g.resource[s.type][s.subType] * allureMod );
	else
		gather_amt = parseFloat( g.resource[s.type] * allureMod );

	if( s.max > 0 )
	{
		if( s.curr <= gather_amt )
		{
			gather_amt = s.curr.valueOf();
		}
		
		// Remove amount from target.
		s.curr -= gather_amt;
		
		// Add extracted resources to player's resource pool.
		this.player.resources[s.type.toString()] += gather_amt;
		
		// Kill the target if it's now out of resources
		if( s.curr == 0 )
		{
			evt.target.kill();
		}
	}
}

// ====================================================================

function performHeal( evt )
{
	if ( this.player.getDiplomaticStance(evt.target.player) == DIPOLMACY_ENEMY )
	{
		//console.write( "You have a traitor!" );
		return;
	}

	// Cycle through all resources.
	pool = this.player.resources;
	for( resource in pool )
	{
		switch( resource.toString() )
		{
			case "population" || "housing":
			break;
			default:
				// Make sure we have enough resources.
				if ( pool[resource] - evt.target.actions.heal.cost * evt.target.traits.creation.cost[resource] < 0 )
				{
					//console.write( "Not enough " + resource.toString() + " for healing." );
					showMessage("Not enough " + resource.toString() + " for healing.");
					return;
				}
			break;
		}
	}

	evt.target.traits.health.curr += this.actions.heal.speed;
	//console.write( this.traits.id.specific + " has performed a miracle!" );
	
	if (evt.target.traits.health.curr >= evt.target.traits.health.max)
	{		
		evt.target.traits.health.curr = evt.target.traits.health.max;
	}

	// Cycle through all resources.
	pool = this.player.resources;
	for( resource in pool )
	{
		switch( resource.toString() )
		{
			case "population":
			case "housing":
				break;
			default:
				// Deduct resources to pay for healing.
				this.player.resources[resource.toString()] -= parseInt(evt.target.actions.heal.cost * evt.target.traits.creation.cost[resource]);
				break;
		}
	}
}

// ====================================================================

function performBuild( evt )
{
	if( !canBuild( this, evt.target ) )
	{
		evt.preventDefault();
		return;
	}
	
	var t = evt.target;
	var b = this.actions.build;
	var bp = t.buildPoints;
	var hp = t.traits.health;
	
	var points = parseFloat( b.rate ) * parseFloat( b.speed ) / 1000.0;
	if ( bp.curr == 0 )
	{
		t.flattenTerrain(); //make the terrain stable for the building
		//t.placeBuildingFootprint(false);  //false means display regular footprint, not rubble
	}
	bp.curr += points;
	
	hp.curr += ( points / bp.max ) * hp.max;
	if( hp.curr >= hp.max )
		hp.curr = hp.max;
	
	if( bp.curr >= bp.max )
	{
		// We've finished building this object; convert the foundation to a building
		if( t.building != "" )	// Might be false if another unit finished building the thing during our last anim cycle
		{
			t.template = getEntityTemplate( t.building, t.player );
			t.building = "";
			
			t.attachAuras();
			
			if (t.traits.population && t.traits.population.add)
				this.player.resources.housing += parseInt(t.traits.population.add);
		}
		evt.preventDefault();	// Stop performing this action
	}
}

// ====================================================================

function performRepair( evt )
{
	if( !canRepair( this, evt.target ) )
	{
		evt.preventDefault();
		return;
	}
	
	var t = evt.target;
	var b = this.actions.build;
	var hp = t.traits.health;
	var resources = t.traits.creation.resource;
	
	// Find the fraction of max health to repair by; this should be one build tick (i.e. longer for buildings with
	// longer creation time) but also not so much that it causes the unit to have more than max HP
	var fraction = Math.min(
		parseFloat( b.rate ) / t.traits.creation.time,
		( hp.max - hp.curr ) / hp.max 
	);
	//console.write("Repair fraction is " + fraction);
	
	// Check if we can afford to repair
	for( r in resources )
	{
		var amount = parseInt( fraction * parseInt(resources[r]) );
		if( this.player.resources[r.toString()] < amount )
		{
			//console.write("Can't repair - not enough " + r.toString());
			showMessage("Can't repair - not enough " + r.toString());
			evt.preventDefault();
			return;
		}
	}

	// Heal the building
	hp.curr = Math.min( hp.max, hp.curr + fraction * hp.max );
	
	// Deduct the resources
	for( r in resources )
	{
		amount = parseInt( fraction * parseInt(resources[r]) );
		this.player.resources[r.toString()] -= amount;
	}
}

// ====================================================================

function damage( dmg, inflictor )
{	
	var arm = this.traits.armour;
	if( !arm ) return;		// corpses have no armour, everything else should

	// Unit has already been destroyed this frame, don't loot for everything hitting it
	if( this.isDestroyed() ) return;

	// Use traits.health.max = 0 to signify immortal things like settlements
	if( this.traits.health.max == 0 ) return;
	
	this.lastCombatTime = getGameTime();
	
	// Apply armour and work out how much damage we actually take
	crushDamage = dmg.crush - arm.crush;
	if ( crushDamage < 0 ) crushDamage = 0;
	pierceDamage = dmg.pierce - arm.pierce;
	if ( pierceDamage < 0 ) pierceDamage = 0;
	hackDamage = dmg.hack - arm.hack;
	if ( hackDamage < 0 ) hackDamage = 0;

	totalDamage = parseInt(dmg.typeless + crushDamage + pierceDamage + hackDamage);

	// Minimum of 1 damage
	if( totalDamage < 1 ) totalDamage = 1;

	this.traits.health.curr -= totalDamage;

	if( this.traits.health.curr <= 0 )
	{
		// If the fallen is worth any loot and the inflictor is capable of looting
		if (this.traits.loot && inflictor.actions && inflictor.actions.loot)
		{
			// Cycle through all loot on this entry.
			const LOOTS = ["food", "wood", "metal", "stone", "xp"]
			for( i in LOOTS )
			{
				var loot = LOOTS[i];
				if(this.traits.loot[loot]) {
					switch( loot )
					{
							case "xp":
							// If the inflictor gains promotions, and he's capable of earning more ranks,
							if (inflictor.traits.promotion && inflictor.traits.promotion.curr 
									&& inflictor.traits.promotion.req
									&& inflictor.traits.promotion.entity 
									&& inflictor.traits.promotion.entity != ""
									&& this.traits.loot && this.traits.loot.xp 
									&& inflictor.actions.loot.xp)
							{
								// Give him the fallen's upgrade points (if he has any).
								if (this.traits.loot.xp)
									inflictor.traits.promotion.curr = parseInt(inflictor.traits.promotion.curr) + parseInt(this.traits.loot.xp);

								// If he now has maximum upgrade points for his rank,
								if (inflictor.traits.promotion.curr >= inflictor.traits.promotion.req)
								{
									// Reset his upgrade points.
									inflictor.traits.promotion.curr = 0; 

									// Upgrade his portrait to the next level.
									inflictor.traits.id.icon_cell++; 

									// Transmogrify him into his next rank.
									inflictor.template = getEntityTemplate( inflictor.traits.promotion.entity, inflictor.player );
									inflictor.setupRank();
								}
							}
							break;
						default:
							if ( inflictor.actions.loot.resources )
							{
								// Give the inflictor his resources.
								//commenting because of #221
								//this.player.resources[loot.toString()] -= parseInt(this.traits.loot[loot.toString()]);
							}
							break;
					}
				}
			}
		}

		// Notify player.
		if ( inflictor ) {
			console.write( "P" + this.player.id + "'s " + this.traits.id.generic +
			               " got the point of " + inflictor.traits.id.generic + "'s weapon." );
		} else {
			console.write( "P" + this.player.id + "'s " + this.traits.id.generic + 
			               " died in mysterious circumstances." );
		}

		// Make him cry out in pain.
		if (this.traits.audio && this.traits.audio.path)
		{
			if(getGUIGlobal().newRandomSound) {
				var curr_pain = getGUIGlobal().newRandomSound(
					"voice", "pain", this.traits.audio.path);
				if (curr_pain) curr_pain.play();
			}
		}
		else
		{
			console.write ("Sorry, no death sound for this unit; you'll just have to imagine it ...");
		}

		// We've taken what we need. Kill the swine.
		//console.write("Kill!!");
		this.kill();
	}
	/*else if( inflictor && this.actions && this.actions.attack )
	{
		// If we're not already doing something else, take a measured response - hit 'em back.
		// You know, I think this is quite possibly the first Ai code the Ai divlead has written
		// for 0 A.D....
		//When the entity changes order, we can readjust flank penalty. We must destroy the notifiers ourselves later,however.
		this.requestNotification( inflictor, NOTIFY_ORDER_CHANGE, false, true );			
		this.registerDamage( inflictor );
		if( this.isIdle() )
			this.order( ORDER_CONTACT_ACTION, inflictor, this.getAttackAction( inflictor ), false );
	}*/
	
	this.onDamaged( inflictor );
}
// ====================================================================

function entityEventContactAction( evt )
{
	switch( evt.action )
	{
		case ACTION_ATTACK:
			this.performAttack( evt ); break;
		case ACTION_GATHER:
			evt.notifyType = NOTIFY_GATHER;
			this.performGather( evt );
			break;
		case ACTION_HEAL:
			this.performHeal( evt ); break;
		case ACTION_ATTACK_RANGED:
			this.performAttackRanged( evt ); break;
		case ACTION_BUILD:
			this.performBuild( evt ); break;
		case ACTION_REPAIR:
			this.performRepair( evt ); break;

		default:
			console.write( "Unknown contact action: " + evt.action );
	}
}

//======================================================================

function entityEventNotification( evt )
{
	//This is used to adjust the flank penalty (we're no longer being attacked).
	if ( this.getCurrentRequest() == NOTIFY_ORDER_CHANGE )
	{
		this.registerOrderChange( evt.target );
		destroyNotifier( evt.target );
		return;
	}
	//Add "true" to the end of order() to indicate that this is a notification order.
	switch( evt.notifyType )
	{
		
		case NOTIFY_GOTO:	
			this.GotoInRange( evt.location.x, evt.location.z, false);
			break;
		case NOTIFY_RUN:
			this.GotoInRange( evt.location.x, evt.location.z, true );
			break;
		case NOTIFY_ATTACK:
		case NOTIFY_DAMAGE:
			this.order( ORDER_CONTACT_ACTION, evt.target, ACTION_ATTACK, false );
			break;
		case NOTIFY_HEAL:
			this.order( ORDER_CONTACT_ACTION, evt.target, ACTION_HEAL, false );
			break;
		case NOTIFY_GATHER:
			this.order( ORDER_CONTACT_ACTION, evt.target, ACTION_GATHER, false );
			break;
		case NOTIFY_IDLE:
			//target is the unit that has become idle.  Eventually...do something here.
			break;
		default:
			console.write( "Unknown notification request " + evt.notifyType );
			return;
	}
	
}		

// ====================================================================

function getAttackAction( target )
{
	if ( !this.actions.attack || target.traits.health.max == 0 )
		return ACTION_NONE;
	var attack = this.actions.attack;
	if ( attack.melee )
		return ACTION_ATTACK;
	else if ( attack.ranged )
		return ACTION_ATTACK_RANGED;
	else
		return ACTION_NONE;
}

// ====================================================================

// TODO: Change this to an event so that it gets passed to our parent too, like other events
function entityComplete()
{
	console.write( this + " is finished building." );
}

//=====================================================================

function entityEventIdle( evt )
{
	//Use our own data for target; we aren't affecting anyone, so listeners want to know about us
	this.forceCheckListeners( NOTIFY_IDLE, this );
}

// ====================================================================

function entityEventTargetExhausted( evt )
{
	if( evt.action == ACTION_GATHER )
	{
		// Look for other resources of the same type to gather
		this.chooseGatherTarget( evt.target.traits.supply.subType.toString() );
	}
	else if( evt.action == ACTION_BUILD )
	{
		// If the target was gatherable, try to gather it.
		if( canGather(this, evt.target) )
		{
			this.order( ORDER_CONTACT_ACTION, evt.target, ACTION_GATHER, false );
			return;
		}
		
		// Look for other stuff to build
		var visible = this.getVisibleEntities();
		var bestDist = 1e20;
		var bestTarget = null;
		for( var i=0; i<visible.length; i++ )
		{
			var e = visible[i];
			if( canBuild( this, e ) )
			{
				var dist = this.getDistance( e );
				if( dist < bestDist )
				{
					bestDist = dist;
					bestTarget = e;
				}
			}
		}
		if( bestTarget != null )
		{
			this.order( ORDER_CONTACT_ACTION, bestTarget, ACTION_BUILD, false );
			return;
		}
		
		// Nothing to build, but try to gather any resource around us if this was a village object.
		if( evt.target.hasClass("Village") )
		{
			this.chooseGatherTarget( null, evt.target.getVisibleEntities() );
		}
	}
}

function chooseGatherTarget( resourceSubType, targetList )
{
	if( !targetList )
		targetList = this.getVisibleEntities();
	var bestDist = 1e20;
	var bestTarget = null;
	for( var i=0; i<targetList.length; i++ )
	{
		var e = targetList[i];
		if( canGather( this, e )
			&& ( !resourceSubType || e.traits.supply.subType.toString() == resourceSubType ) )
		{
			var dist = this.getDistance( e );
			if( dist < bestDist )
			{
				bestDist = dist;
				bestTarget = e;
			}
		}
	}
	if( bestTarget != null )
	{
		this.order( ORDER_CONTACT_ACTION, bestTarget, ACTION_GATHER, false );
		return true;
	}
	return false;
}

// ====================================================================

function entityEventTargetChanged( evt )
{
	// This event lets us know when the user moves his/her cursor to a different unit (provided this
	// unit is selected) - use it to tell the engine what context cursor should be displayed, given
	// the target.

	// If we can gather, and the target supplies, gather. If it's our enemy, and we're armed, attack. 
	// If all else fails, move (or run on a right-click).
	
	if ( getCursorName() == "cursor-rally" )
	{
		evt.defaultCursor = "cursor-rally";
		evt.defaultOrder = -1;	
		return;
	}	

	evt.defaultOrder = NMT_GOTO;
	evt.defaultCursor = "arrow-default";
	evt.defaultAction = ACTION_NONE;
	evt.secondaryAction = ACTION_NONE;
	evt.secondaryCursor = "arrow-default";
	if ( this.actions && this.actions.run && this.actions.run.speed > 0 )
	{
		evt.secondaryOrder = NMT_RUN;
	}

	if( evt.target && this.actions )
	{
		if( this.actions.attack && 
			this.player.getDiplomaticStance(evt.target.player) != DIPLOMACY_ALLIED &&
			evt.target.traits.health &&
			evt.target.traits.health.max != 0 )
		{
			evt.defaultOrder = NMT_CONTACT_ACTION;
			evt.defaultAction = this.getAttackAction( evt.target );
			evt.defaultCursor = "action-attack";

			evt.secondaryOrder = NMT_CONTACT_ACTION;
			evt.secondaryAction = this.getAttackAction( evt.target );
			evt.secondaryCursor = "action-attack";
		}
		
		if ( this.actions.escort &&
			this != evt.target &&
			evt.target.player == this.player &&
			evt.target.actions )
		{
			if (evt.target.actions.move)
			{ 
				//Send an empty order
				evt.defaultOrder = NMT_NOTIFY_REQUEST;
				evt.secondaryOrder = NMT_NOTIFY_REQUEST;

				evt.defaultAction = NOTIFY_ESCORT;
				evt.secondaryAction = NOTIFY_ESCORT;
			}
		}

		if ( canGather( this, evt.target ) )
		{
			evt.defaultOrder = NMT_CONTACT_ACTION;
			evt.defaultAction = ACTION_GATHER;
		    // Set cursor (eg "action-gather-fruit").
		    evt.defaultCursor = "action-gather-" + evt.target.traits.supply.subType;

			evt.secondaryOrder = NMT_CONTACT_ACTION;
			evt.secondaryAction = ACTION_GATHER;
		  	// Set cursor (eg "action-gather-fruit").
		    evt.secondaryCursor = "action-gather-" + evt.target.traits.supply.subType;
		}
		
		if ( canBuild( this, evt.target ) )
		{
			evt.defaultOrder = NMT_CONTACT_ACTION;
			evt.defaultAction = ACTION_BUILD;
		    evt.defaultCursor = "action-build";

			evt.secondaryOrder = NMT_CONTACT_ACTION;
			evt.secondaryAction = ACTION_BUILD;
		    evt.secondaryCursor = "action-build";
		}
		
		if ( canRepair( this, evt.target ) )
		{
			evt.defaultOrder = NMT_CONTACT_ACTION;
			evt.defaultAction = ACTION_REPAIR;
		    evt.defaultCursor = "action-build";

			evt.secondaryOrder = NMT_CONTACT_ACTION;
			evt.secondaryAction = ACTION_REPAIR;
		    evt.secondaryCursor = "action-build";
		}
	}
	//Rally point
	else if ( this.building )
	{
		evt.defaultOrder = -1;
		evt.defaultCursor = "cursor-rally";
	}

		
}

// ====================================================================

function entityEventPrepareOrder( evt )
{
	// This event gives us a chance to veto any order we're given before we execute it.
	// Not sure whether this really belongs here like this: the alternative is to override it in
	// subTypes - then you wouldn't need to check tags, you could hardcode results.

	if ( !this.actions )
	{
		evt.preventDefault();
		return;
	}
	
	//evt.notifySource is the entity order data will be obtained from, so if we're attacking and we 
	//want our listeners to copy us, then we will use our own order as the source.
	//registerOrderChange() is used to adjust the flank penalty

	switch( evt.orderType )
	{
		case ORDER_GOTO:
			if ( !this.actions.move )
			{
				evt.preventDefault();
				return;
			}
			evt.notifyType = NOTIFY_GOTO;
			evt.notifySource = this;
			this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
			break;
			
		case ORDER_RUN:
			if ( !this.actions.move || !this.actions.move.run )	
			{
				evt.preventDefault();
				return;
			}
			evt.notifyType = NOTIFY_RUN;
			evt.notifySource = this;
			this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
			break;
			
		case ORDER_PATROL:
			if ( !this.actions.patrol )
			{
				evt.preventDefault();
				return;
			}
			this.registerOrderChange();
			this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
			break;	
			
		case ORDER_CONTACT_ACTION:
			evt.notifySource = this;
			switch ( evt.action )
			{
				case ACTION_ATTACK:
				case ACTION_ATTACK_RANGED:
					evt.action = this.getAttackAction( evt.target );
					if ( evt.action == ACTION_NONE )
					{
						evt.preventDefault();
						return;
					}
					evt.notifyType = NOTIFY_ATTACK;
					this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
					break;

				case ACTION_GATHER:
					if ( !this.actions.gather )
					{
						evt.preventDefault();
						return;
					}
					evt.notifyType = NOTIFY_GATHER;
					this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
					
					// Change our gather animation based on the type of target
					var a = this.actions.gather;
					this.setActionParams( ACTION_GATHER, 0.0, a.range, a.speed,
						"gather_" + evt.target.traits.supply.subType );
						
					break;
					
				case ACTION_HEAL:
					if ( !this.actions.heal )
					{
						evt.preventDefault();
						return;
					}
					evt.notifyType = NOTIFY_HEAL;
					this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
					break;	
					
				case ACTION_BUILD:
					if ( !this.actions.build || !evt.target.building || evt.target.building=="" )
					{
						evt.preventDefault();
						return;
					}
					evt.notifyType = NOTIFY_NONE;
					this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
					break;	
					
				case ACTION_REPAIR:
					if ( !this.actions.repair )
					{
						evt.preventDefault();
						return;
					}
					evt.notifyType = NOTIFY_NONE;
					this.forceCheckListeners( NOTIFY_ORDER_CHANGE, this );
					break;	
			}
			break;
		
		case ORDER_PRODUCE:
		case ORDER_START_CONSTRUCTION:
			evt.notifyType = NOTIFY_NONE;
			break;

		case ORDER_SET_RALLY_POINT:
		case ORDER_SET_STANCE:
			break;
			
		default:
			console.write("Unknown order type " + evt.orderType + "; ignoring.");
			evt.preventDefault();
			break;
	}
}

// ====================================================================

function entityStartConstruction( evt )
{
	this.order( ORDER_CONTACT_ACTION, evt.target, ACTION_BUILD, false );
}

// ====================================================================

const TECH_RESOURCES = new Array("food", "wood", "stone", "metal");

function entityStartProduction( evt )
{
	//console.write("StartProduction: " + evt.productionType + " " + evt.name);
	
	if( evt.productionType == PRODUCTION_TRAIN ) 
	{
		var template = getEntityTemplate( evt.name, this.player );
		var result = checkEntityReqs( this.player, template );	

		if (result == true) // If the entry meets requirements to be added to the queue (eg sufficient resources) 
		{
			// Cycle through all costs of this entry.
			var pool = template.traits.creation.resource;
			for ( resource in pool )
			{
				switch ( resource.toString() )
				{
					case "population":
					case "housing":
					break;
					default:
						// Deduct the given quantity of resources.
						this.player.resources[resource.toString()] -= parseInt(pool[resource]);
					break;
				}
			}
			
			// Reserve population space for the unit
			if( template.traits.population && template.traits.population.rem )
			{
				this.player.resources.population += parseInt(template.traits.population.rem);
			}
	
			// Set the amount of time it will take to complete production of the production object.
			evt.time = getEntityTemplate( evt.name, this.player ).traits.creation.time;
		}
		else
		{	
			// If not, output the error message.
			//console.write(result);
			showMessage(result);
			evt.preventDefault();
			return;
		}
	}
	else if( evt.productionType == PRODUCTION_RESEARCH ) 
	{
		var tech = getTechnology( evt.name, this.player );
		
		if( !tech )
		{
			console.write( "No such tech: " + evt.name );
			evt.preventDefault();
			return;
		}
		
		// Check tech requirements other than resources
		if( !tech.isValid() ) 
		{
			console.write( "Tech " + evt.name + " is currently unavailable" );
			evt.preventDefault();
			return;
		}
		
		// Check for sufficient resources
		for( i in TECH_RESOURCES )
		{
			var res = TECH_RESOURCES[i];
			if( this.player.resources[res] < tech[res] )
			{
				//console.write( "Cannot afford " + evt.name + ": need " + tech[res] + " " + res );
				showMessage( "Cannot afford " + evt.name + ": need " + tech[res] + " " + res );
				evt.preventDefault();
				return;
			}
		}
		
		// Subtract resources
		for( i in TECH_RESOURCES )
		{
			var res = TECH_RESOURCES[i];
			this.player.resources[res] -= tech[res];
		}
		
		// Mark it as in progress
		tech.in_progress = true;
		
		// Set the amount of time it will take to complete production of the tech.
		evt.time = tech.time;
	}
	else
	{
		evt.preventDefault();
	}
}

function entityCancelProduction( evt )
{
	//console.write("CancelProduction: " + evt.productionType + " " + evt.name);
	
	if( evt.productionType == PRODUCTION_TRAIN )
	{
		// Give back all the resources spent on this entry.
		var template = getEntityTemplate( evt.name, this.player );
		var pool = template.traits.creation.resource;
		for ( resource in pool )
		{
			switch ( resource.toString() )
			{
				case "population":
				case "housing":
				break;
				default:
					// Refund the given quantity of resources.
					this.player.resources[resource.toString()] += parseInt(pool[resource]);

					console.write ("Got back " + pool[resource] + " " + resource + " from cancelling " + 
						template.traits.id.generic);
				break;
			}
		}
		
		// Give back the reserved population space
		if( template.traits.population && template.traits.population.rem )
		{
			this.player.resources.population -= parseInt(template.traits.population.rem);
		}
	}
	else if( evt.productionType == PRODUCTION_RESEARCH )
	{
		var tech = getTechnology( evt.name, this.player );
		
		// Give back the player's resources
		for( i in TECH_RESOURCES )
		{
			var res = TECH_RESOURCES[i];
			this.player.resources[res] += tech[res];
		}
		
		// Unmark tech as in progress
		tech.in_progress = false;
	}
}

function entityFinishProduction( evt )
{
	//console.write("FinishProduction: " + evt.productionType + " " + evt.name);
	
	if( evt.productionType == PRODUCTION_TRAIN ) 
	{
		var template =  getEntityTemplate( evt.name, this.player );
	
		// Give back reserved population space (the unit will take it up again in its initialize event, if we find space to place it)
		if( template.traits.population && template.traits.population.rem )
		{
			this.player.resources.population -= parseInt(template.traits.population.rem);
		}
	
		// Code to find a free space around an object is tedious and slow, so 
		// I wrote it in C. Takes the template object so it can determine how
		// much space it needs to leave.
		var position = this.getSpawnPoint( template );
		
		// The above function returns null if it couldn't find a large enough space.
		if( !position )
		{
			console.write( "Couldn't train unit - not enough space" );
			// Oh well. The player's just lost all the resources and time they put into
			// construction - serves them right for not paying attention to the land
			// around their barracks, doesn't it?
		}
		else
		{
			var created = new Entity( template, position, 0, this.player );
		
			// Above shouldn't ever fail, but just in case...
			if( created )
			{
				//console.write( "Created: ", template.tag );
				var rally = this.getRallyPoint();
				created.order( ORDER_GOTO, rally.x, rally.y );	
			}
		}		
	}
	else if( evt.productionType == PRODUCTION_RESEARCH ) 
	{
		// Apply the tech's effects
		var tech = getTechnology( evt.name, this.player );
		tech.applyEffects();
	}
}

// Old training queue system

// ====================================================================
/*
function entityAddCreateQueue( template, tab, list )
{
	// Make sure we have a queue to put things in...
	if( !this.actions.create.queue )
		this.actions.create.queue      = new Array();

	// Construct template object.
	comboTemplate = template;
	comboTemplate.list = list;
	comboTemplate.tab = tab;

	// Append to the end of this queue

	this.actions.create.queue.push( template );

	// If we're not already building something...
	if( !this.actions.create.progress )
	{
		console.write( "Starting work on (unqueued) ", template.tag );
		 // Start the progress timer.
		 // - First parameter is target value (in this case, base build time in seconds)
		 // - Second parameter is increment per millisecond (use build rate modifier and correct for milliseconds)
		 // - Third parameter is the function to call when the timer finishes.
		 // - Fourth parameter is the scope under which to run that function (what the 'this' parameter should be)
		 this.actions.create.progress = new ProgressTimer( template.traits.creation.time, 
			this.actions.create.speed / 1000, entityCreateComplete, this )
	}
}

// ====================================================================

// This is the syntax to add a function (or a property) to absolutely every entity.

Entity.prototype.add_create_queue = entityAddCreateQueue;

// ====================================================================

function entityCreateComplete()
{
	// Get the unit that was at the head of our queue, and remove it.
	// (Oh, for information about all these nifty properties and functions
	//  of the Array object that I use, see the ECMA-262 documentation
	//  at http://www.mozilla.org/js/language/E262-3.pdf. Bit technical but
	//  the sections on 'Native ECMAScript Objects' are quite useful)
	
	var template = this.actions.create.queue.shift();

	// Code to find a free space around an object is tedious and slow, so 
	// I wrote it in C. Takes the template object so it can determine how
	// much space it needs to leave.
	position = this.getSpawnPoint( template );
	
	// The above function returns null if it couldn't find a large enough space.
	if( !position )
	{
		console.write( "Couldn't train unit - not enough space" );
		// Oh well. The player's just lost all the resources and time they put into
		// construction - serves them right for not paying attention to the land
		// around their barracks, doesn't it?
	}
	else
	{
		created = new Entity( template, position );
	
		// Above shouldn't ever fail, but just in case...
		if( created )
		{
			console.write( "Created: ", template.tag );
	
			// Entities start under Gaia control - make the controller
			// the same as our controller
			created.player = this.player;
		}
	}		
	// If there's something else in the build queue...
	if( this.actions.create.queue.length > 0 )
	{
		// Start on the next item.
		template = this.actions.create.queue[0];
		console.write( "Starting work on (queued) ", template.tag );
		this.actions.create.progress = new ProgressTimer( template.traits.creation.time, 
			this.actions.create.speed / 1000, entityCreateComplete, this )
	}
	else
	{
		// Otherwise, delete the timer.
		this.actions.create.progress = null;
	}
}

// ====================================================================

function attemptAddToBuildQueue( entity, create_tag, tab, list )
{
	template = getEntityTemplate( create_tag, entity.player );
	result = checkEntityReqs( entity, template );	

	if (result == true) // If the entry meets requirements to be added to the queue (eg sufficient resources) 
	{
		// Cycle through all costs of this entry.
		pool = template.traits.creation.resource;
		for ( resource in pool )
		{
			switch ( resource.toString() )
			{
				case "population":
				case "housing":
				break;
				default:
					// Deduct the given quantity of resources.
					this.player.resources[resource.toString()] -= parseInt(pool[resource]);

					console.write ("Spent " + pool[resource] + " " + resource + " to purchase " + 
						template.traits.id.generic);
				break;
			}
		}

		// Add entity to queue.
		console.write( "Adding ", create_tag, " to build queue..." );
		entity.addCreateQueue( template, tab, list );
		
		return true;
	}
	else
	{	// If not, output the error message.
		//console.write(result);
		showMessage(result);
		return false;
	}
}
*/
// ====================================================================

function checkEntityReqs( player, template ) 
{ 
	// Determines if the given entity meets requirements for production by the player, and returns an appropriate 
	// error string.
	// A return value of 0 equals success -- entry meets requirements for production. 
	
	// Cycle through all resources that this item costs, and check the player can afford the cost.
	var resources = template.traits.creation.resource;
	for( resource in resources )
	{
		switch( resource.toString() )
		{
			case "population":
			case "housing": // Ignore housing. It's handled in combination with population.
			break
			default:
				// If the item costs more of this resource type than we have,
				var cur = parseInt(player.resources[resource]);
				var req = parseInt(resources[resource]);
				if (req > cur)
				{
					// Return an error.
					return ("Insufficient " + resource + "; " + (req-cur) + " more required."); 
				}
				//else
				//	console.write("Player has at least " + req + " " + resource + ".");
			break;
		}
	}
	
	// Check if we have enough population space for the entity
	
	if(template.traits.population && template.traits.population.rem) 
	{
		var req = parseInt(template.traits.population.rem);
		var space = player.resources.housing - player.resources.population;
	
		// If the item costs more of this resource type than we have,
		if (req > space)
		{
			// Return an error.
			return ("Insufficient Housing; " + (req - space) + " more required."); 
		}
	}

	// Check if another entity must first exist. 

	// Check if another tech must first be researched. 

	// Check if the limit for this type of entity has been reached. 

	// If we passed all checks, return success. Entity can be queued.
	return true; 
}

// ====================================================================

function canGather( source, target )
{
	// Checks whether we're allowed to gather from a target entity (this involves looking at both the type and subType).
	if( !source.actions || !target.traits )
		return false;
	g = source.actions.gather;
	s = target.traits.supply;
	return ( g && s && g.resource && g.resource[s.type] &&
		( s.subType==s.type || g.resource[s.type][s.subType] ) &&
		( s.curr > 0 || s.max == 0 ) && 
		s.dropsiteCount[source.player.id] );
}

// ====================================================================

function canBuild( source, target )
{
	// Checks whether we're allowed to build a target entity
	if( !source.actions )
		return false;
	b = source.actions.build;
	return (b && target.building != "" && target.player.id == source.player.id
		&& target.buildPoints.curr < target.buildPoints.max );
}

// ====================================================================

function canRepair( source, target )
{
	// Checks whether we're allowed to gather from a target entity (this involves looking at both the type and subType).
	if( !source.actions )
		return false;
	r = source.actions.repair;
	return( r && target.traits.health.repairable && target.player.id == source.player.id
			&& target.traits.health.curr < target.traits.health.max 
			&& target.building == "" );
}

// ====================================================================

// Get the elevation attack bonus if we are heightDif units higher than our target
function getElevationBonus( heightDif )
{
	if ( heightDif > 2.0 )
		return 0.3;	// we are significantly higher, get a positive bonus
	else 
		return 0.0;	// we are at roughly the same height, or below
}

// ====================================================================

// DamageType class
function DamageType()
{
	this.typeless = 0.0;
	this.crush = 0.0;
	this.pierce = 0.0;
	this.hack = 0.0;
}

// ====================================================================

function DamageModifyAura( source, ally, bonus )
{
	// Defines the effects of the DamageModify Aura. (Adjacent units have modified attack bonus.)
	// The Courage Aura uses this function to give attack bonus to allies.	
	// The Fear Aura uses this function to give attack penalties to enemies.

	this.source = source;
	this.bonus = bonus;
	this.ally = ally;
	
	this.affects = function( e ) 
	{
		if( this.ally )
		{
			return ( e.player.id == this.source.player.id && e.actions && e.actions.attack );
		}
		else
		{
			return ( e.player.id != this.source.player.id && e.actions && e.actions.attack );
		}
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "DamageModify aura: giving " + this.bonus + " damage to " + e );
			e.actions.attack.crush += this.bonus;
			e.actions.attack.pierce += this.bonus;
			e.actions.attack.hack += this.bonus;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "DamageModify aura: taking away " + this.bonus + " damage from " + e );
			e.actions.attack.crush -= this.bonus;
			e.actions.attack.pierce -= this.bonus;
			e.actions.attack.hack -= this.bonus;
		}
	};
}

// ====================================================================

function DropsiteAura( source, types )
{
	// Defines the effects of the Gather aura. Enables resource gathering on entities
	// near the source for it's owner.

	this.source = source;
	this.types = types;
	
	this.affects = function( e ) 
	{
		return( e.traits.supply && this.types[e.traits.supply.type] );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Dropsite aura: adding +1 for " + this.source.player.id + " on " + e );
			e.traits.supply.dropsiteCount[this.source.player.id]++;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Dropsite aura: adding -1 for " + this.source.player.id + " on " + e );
			e.traits.supply.dropsiteCount[this.source.player.id]--;
		}
	};
}

// ====================================================================

function InfidelityAura( source, time )
{
	// Defines the effects of the Infidelity Aura. Changes ownership of entity when only one player's units surrounds them.

	this.source = source;
	
	this.time = time;
	
	this.count = new Array( 9 );
	for( i = 0; i <= 8; i++ )
	{
		this.count[i] = 0;
	}
	
	this.convertTimer = 0;
		
	this.affects = function( e ) 
	{
		return ( e.player.id != 0 && ( !e.traits.auras || !e.traits.auras.infidelity ) );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Infidelity aura: adding +1 count to " + e.player.id );
			this.count[e.player.id]++;
			this.changePlayerIfNeeded();
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			//console.write( "Infidelity aura: adding -1 count to " + e.player.id );
			this.count[e.player.id]--;
			this.changePlayerIfNeeded();
		}
	};
	
	this.changePlayerIfNeeded = function()
	{
		if( this.count[this.source.player.id] == 0 )
		{
			// If our owner has nothing near us but someone else does, start a time to convert over if we haven't done so already
			if( !this.convertTimer )
			{
				for( i = 1; i <= 8; i++ )
				{
					if( this.count[i] > 0 )
					{
						this.convertTimer = setTimeout( this.convert, parseInt( this.time * 1000 ), this );
						return;
					}
				}
			}
		}
		
		// If we had started a convert timer before, cancel it (either we have units from our owner in range, or there are no units from anyone in range)
		if( this.convertTimer )
		{
			cancelTimer( this.convertTimer );
			this.convertTimer = 0;
		}
	}
	
	this.convert = function()
	{
		// Switch ownership to whichever non-gaia player has the most units near us
		bestPlayer = 0;
		bestCount = 0;
		for( i = 1; i <= 8; i++ )
		{
			if( this.count[i] > bestCount )
			{
				bestCount = this.count[i];
				bestPlayer = i;
			}
		}
		if( bestCount > 0 )
		{
			//console.write( "Infidelity aura: changing ownership to " + bestPlayer );
			this.source.player = players[bestPlayer];
		}
		
		this.convertTimer = 0;
	}
}

// ====================================================================

function HealAura( source )
{
	// Defines the effects of the Heal aura. Slowly heals nearby allies over time.

	this.source = source;
	
	this.affects = function( e ) 
	{
		return ( e.player.id == this.source.player.id && e.traits.health 
					&& e.traits.health.healable
					&& e.traits.health.curr < e.traits.health.max );
	}
	
	this.onTick = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			var hp = e.traits.health;
			var rate = this.source.traits.auras.heal.rate;
			hp.curr = Math.min( hp.max, hp.curr + rate );
		}
	};
}

// ====================================================================

function TrampleAura( source )
{
	// Defines the effects of the Trample  aura. Damages nearby enemies over time if the unit is charging or has recently charged.
	
	this.source = source;
	
	this.affects = function( e )
	{
		// Check if the target is an enemy foot unit with health and if we were running in the last 3 seconds
		var a = this.source.traits.auras.trample;
		return ( e.player.id != this.source.player.id && e.traits.health && e.hasClass("Foot")
			&& (getGameTime() - this.source.lastRunTime < a.duration) );
	}
	
	this.onTick = function( e )
	{
		if( this.affects( e ) )
		{
			// Set up the damage object
			var dmg = new DamageType();
			var a = this.source.traits.auras.trample;
			dmg.crush = parseInt(a.crush);
			dmg.hack = parseInt(a.hack);
			dmg.pierce = parseInt(a.pierce);

			// Add flank bonus
			if(e.traits.flankPenalty)
			{
				var flank = (e.getAttackDirections()-1)*e.traits.flankPenalty.value;
				dmg.crush += dmg.crush * flank;
				dmg.hack += dmg.hack * flank;
				dmg.pierce += dmg.pierce * flank;
			}

			// Perform the damage
			e.damage( dmg, this.source );
		}
	};
}

// ====================================================================

function SettlementAura( source )
{
	// Defines the effects of the Settlement Aura. Changes ownership of entity when a civil center is on it.

	this.source = source;
	
	this.affects = function( e ) 
	{
		return ( e.hasClass("CivilCentre") );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			// If a new Civ Centre has entered our radius, it must mean it's on us; switch player and become invisible
			source.player = e.player;
			source.visible = false;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			// If a Civ Centre has entered our radius, it must mean the one on us died; become visible again and assign it to Gaia.
			source.visible = true;
			source.player = players[0];	//player[0] is allways Gaia.
		}
	}
}

// ====================================================================

function AllureAura( source )
{
	// Defines the effects of the Allure Aura. (Adjacent male units owned by the same player gather faster.)

	this.source = source;
	
	this.affects = function( e ) 
	{
		return ( e.player.id == this.source.player.id && e.actions && e.actions.gather
			&& e.actions.gather.affectedByAllure );
	}
	
	this.onEnter = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			e.allureCount++;
		}
	};
	
	this.onExit = function( e ) 
	{
		if( this.affects( e ) ) 
		{
			e.allureCount--;
		}
	};
}

// ====================================================================

function GotoInRange( x, y, run )
{	
	if ( !this.actions || !this.actions.move )
		return;
	//Add "true" at the end to indicate that this is a notification order.
	if (run && this.actions.move.run)
		this.order( ORDER_RUN, x, y - this.actions.escort.distance, true);
	else
		this.order( ORDER_GOTO, x, y - this.actions.escort.distance, true);
}

//=====================================================================

function entityEventFormation( evt )
{
	if ( evt.formationEvent == FORMATION_ENTER )
	{
		if ( this.getFormationBonus() && this.hasClass( this.getFormationBonusType() ) )
		{
			eval( this + this.getFormationBonus() ) += eval( this + this.getFormationBonusBase() ) *		
				 this.getFormationBonusVal();
		}
		if ( this.getFormationPenalty() && this.hasInClass( this.getFormationPenaltyType() ) )
		{
			eval( this + this.getFormationPenalty() ) -= eval( this + this.getFormationPenaltyBase() ) *
				this.getFormationPenaltyVal();
		}
	}
	//Reverse the bonuses
	else if ( evt.formationEvent == FORMATION_LEAVE )
	{
		if ( this.getFormationPenalty() && this.hasInClass( this.getFormationPenaltyType() ) )
		{
			eval( this + this.getFormationPenalty() ) += eval( this + this.getFormationPenaltyBase() ) *
				this.getFormationPenaltyVal();
		}
		if ( this.getFormationBonus() && this.hasClass( this.getFormationBonusType() ) )
		{
			eval( this + this.getFormationBonus() ) -= eval( this + this.getFormationBonusBase() ) *						 this.getFormationBonusVal();
		}
		
	}
}

// ====================================================================

function getBuildingLimit( category/*, gameMode*/ )
{
    // Civil
    if(category=="CivilCentre") return 1;
    if(category=="House") return 15;
    if(category=="Farmstead") return 4;
    if(category=="Field") return 16;
    if(category=="Market") return 2;
    // Military
    if(category=="Dock") return 2;
    if(category=="Fortress") return 1;
    if(category=="Barracks") return 2;
    if(category=="ScoutTower") return 10;
    // Other
    if(category=="Special") return 2;
    
    return 0;
}


/*
 * Not exactly entity related funcions, but I wanted it to be accessible from gui/test/ and scripts/ scripts, as they have
 * to call this function. If anyone know of better place for these functions, please tell me.
 */

/* 
 * showMessage(string) - This function should be used when there is some info that should be showed to the user.
 * It is used for example for informing the user about the lack of resources for building or training a unit.
 * The message gets removed after WAIT milliseconds.
 */

const MAX_MESSAGES = 6; //max. messages at a time.
const WAIT = 15000;		//during what time a message will be shown.

var messagesList = new Array();

function showMessage(text)
{
	message = Engine.GetActiveGui().getGUIObjectByName("globalMessage");
	messageUnder = Engine.GetActiveGui().getGUIObjectByName("globalMessageUnder");
	// BUG: The active GUI might be e.g. a message box, not the session GUI,
	// so this won't be looking in the correct place for the GUI objects.
	// (But that's hard to fix cleanly with the new GUI system, and this
	// should probably get rewritten anyway.)

	if (messagesList.length == MAX_MESSAGES)
	{
		messagesList.pop();	
	}
	
	messagesList.unshift(text);
	updateMessageView();
	
	message.hidden = false;
	messageUnder.hidden = false;
	
	/*
	 * This is made so only the first message, calls hideMessage().
	 */
	if (messagesList.length == 1)
	{
		setTimeout(hideMessage, WAIT);
	}
}

function hideMessage()
{
	messagesList.pop();
	
	if (messagesList.length == 0) 
	{
		Engine.GetActiveGui().getGUIObjectByName("globalMessage").hidden = true;
		Engine.GetActiveGui().getGUIObjectByName("globalMessageUnder").hidden = true;
	}
	else
	{
		updateMessageView();
		setTimeout(hideMessage, WAIT);
	}
}

function updateMessageView()
{
	var result = "";
	for (var i=0; i < messagesList.length; i++)
	{
		result = result + messagesList[i] + "\n";
	}
	Engine.GetActiveGui().getGUIObjectByName("globalMessage").caption = result;
	Engine.GetActiveGui().getGUIObjectByName("globalMessageUnder").caption = result;
}


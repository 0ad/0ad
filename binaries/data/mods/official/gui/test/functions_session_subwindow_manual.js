function initManual()
{
	// ============================================ CONSTANTS ================================================

	MANUAL = new Object();
	MANUAL.span = 5;

	// ============================================= GLOBALS =================================================

	// Background of online manual.
	MANUAL_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 50; 
	Crd[Crd.last-1].height	= 50; 
	Crd[Crd.last-1].x	= 50; 
	Crd[Crd.last-1].y	= 50; 

	// Online manual portrait.
	MANUAL_PORTRAIT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= crd_portrait_lrg_width; 
	Crd[Crd.last-1].height	= crd_portrait_lrg_height; 
	Crd[Crd.last-1].x	= Crd[MANUAL_BKG].x+10; 
	Crd[Crd.last-1].y	= Crd[MANUAL_BKG].y+30; 

	// Online manual rollover.
	MANUAL_ROLLOVER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 60; 
	Crd[Crd.last-1].height	= Crd[MANUAL_PORTRAIT].height;
	Crd[Crd.last-1].x	= Crd[MANUAL_PORTRAIT].x+Crd[MANUAL_PORTRAIT].width+MANUAL.span;
	Crd[Crd.last-1].y	= Crd[MANUAL_PORTRAIT].y;

	// Online manual history.
	MANUAL_HISTORY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[MANUAL_ROLLOVER].width;
	Crd[Crd.last-1].height	= Crd[MANUAL_ROLLOVER].height;
	Crd[Crd.last-1].x	= Crd[MANUAL_PORTRAIT].x;
	Crd[Crd.last-1].y	= 60;

	// Online manual text.
	MANUAL_NAME = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[MANUAL_HISTORY].width;
	Crd[Crd.last-1].height	= Crd[MANUAL_HISTORY].y+Crd[MANUAL_HISTORY].height+MANUAL.span;
	Crd[Crd.last-1].x	= Crd[MANUAL_HISTORY].x;
	Crd[Crd.last-1].y	= Crd[MANUAL_ROLLOVER].y+Crd[MANUAL_ROLLOVER].height+MANUAL.span;

	// Online manual exit button.
	MANUAL_EXIT_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 16;
	Crd[Crd.last-1].height	= 16;
	Crd[Crd.last-1].x	= 26;
	Crd[Crd.last-1].y	= 25;
}

// ====================================================================

function manualDisplay()
{
	// Display heading.
	ManualBkg = getGUIObjectByName("MANUAL_BKG");
	ManualBkg.caption = "In-Game Help: ";
	if (selection[0].traits.id.civ)
		ManualBkg.caption += selection[0].traits.id.civ + ": ";
	if (selection[0].traits.id.generic)
		ManualBkg.caption += selection[0].traits.id.generic + ": ";
	if (selection[0].traits.id.specific)
		ManualBkg.caption += selection[0].traits.id.specific;

	// Display portrait.
	if (selection[0].traits.id.icon)
	{
		setPortrait("MANUAL_PORTRAIT", selection[0].traits.id.icon, selection[0].traits.id.civ_code, selection[0].traits.id.icon_cell);
	}

	// Display rollover text.
	if (selection[0].traits.id.rollover)
	{
		ManualRollover = getGUIObjectByName("MANUAL_ROLLOVER");
		ManualRollover.caption = selection[0].traits.id.rollover;
	}

	ManualText = getGUIObjectByName("MANUAL_NAME");
	ManualText.caption = "";

					// Display name(s).
					if (selection[0].traits.id.generic)
						ManualText.caption += selection[0].traits.id.generic;
					if (selection[0].traits.id.generic && selection[0].traits.id.specific)
						ManualText.caption += " - ";
					if (selection[0].traits.id.specific)
						ManualText.caption += selection[0].traits.id.specific;
					if (selection[0].traits.id.specific && selection[0].traits.id.ranked)
						ManualText.caption += " - ";
					if (selection[0].traits.id.ranked)
						ManualText.caption += selection[0].traits.id.ranked;
					// Personal name.
					if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
						ManualText.caption += " - " + selection[0].traits.id.personal;
					ManualText.caption += "\n";

					// Display civilisation.
					if (selection[0].traits.id.civ && selection[0].traits.id.civ_code)
						ManualText.caption += "Civilisation: " + selection[0].traits.id.civ + " (" + selection[0].traits.id.civ_code + ")" + "\n";
					if (!selection[0].traits.id.civ_code)
						ManualText.caption += "Civilisation: " + selection[0].traits.id.civ + "\n";

					// Display player info.
					if (selection[0].player){
						if (selection[0].player.name)
							ManualText.caption += "Player Name: " + selection[0].player.name + "\n";
						if (selection[0].player.id)
							ManualText.caption += "Player ID: " + selection[0].player.id + "\n";
						if (selection[0].player.colour)
							ManualText.caption += "Player Colour: " + selection[0].player.colour + "\n";
						if (selection[0].player.controlled)
							ManualText.caption += "Player Controlled: " + selection[0].player.controlled + "\n";
//						if (players[])
//							ManualText.caption += "Players[]: " + players[] + "\n";
					}
					
					// Display hitpoints.
					if (selection[0].traits.health.curr && selection[0].traits.health.max && selection[0].traits.health)
						ManualText.caption += "Hitpoints: " + selection[0].traits.health.curr + "/" + selection[0].traits.health.max + "\n";

					// Display rank.
					if (selection[0].traits.up.rank)
						ManualText.caption += "Rank: " + selection[0].traits.up.rank + "\n";

					// Display experience.
					if (selection[0].traits.up && selection[0].traits.up.curr && selection[0].traits.up.req)
						ManualText.caption += "XP: " + selection[0].traits.up.curr + "/" + selection[0].traits.up.req + "\n";

					// Display population.
					if (selection[0].traits.population)
					{
						if (selection[0].traits.population.sub)
							ManualText.caption += "Pop: -" + selection[0].traits.population.sub + "\n";
						if (selection[0].traits.population.add)
							ManualText.caption += "Housing: +" + selection[0].traits.population.add + "\n";
					}

					// Display garrison.
					if (selection[0].traits.garrison)
					{
						if (selection[0].traits.garrison.curr && selection[0].traits.garrison.max)
							ManualText.caption += "Garrison: " + selection[0].traits.garrison.curr + "/" + selection[0].traits.garrison.max + "\n";
					}

					// Display supply.
					if (selection[0].traits.supply)
					{
						if (selection[0].traits.supply.curr && selection[0].traits.supply.max && selection[0].traits.supply.type)
						{
							// If Supply is infinite,
							if (selection[0].traits.supply.curr == "0" && selection[0].traits.supply.max == "0")
								// Use infinity symbol.
								ManualText.caption += "Supply: " + selection[0].traits.supply.curr + "/" + selection[0].traits.supply.max + " " + selection[0].traits.supply.type + " (" + selection[0].traits.supply.subtype + ")\n";
							else
								// Use numbers.
								ManualText.caption += "Supply: 8 " + selection[0].traits.supply.type + " (" + selection[0].traits.supply.subtype + ")\n";
						}
					}

					if (selection[0].traits.loot)
					{
						// Display UP on death.
						if (selection[0].traits.loot.up)
							ManualText.caption += "UP: " + selection[0].traits.loot.up + "\n";
	
						// Display loot.
						if (selection[0].traits.loot.food || selection[0].traits.loot.wood || selection[0].traits.loot.stone || selection[0].traits.loot.ore)
						{
							ManualText.caption += "Loot: ";
							if (selection[0].traits.loot.food)
								ManualText.caption += selection[0].traits.loot.food + " Food ";
							if (selection[0].traits.loot.wood)
								ManualText.caption += selection[0].traits.loot.wood + " Wood ";
							if (selection[0].traits.loot.stone)
								ManualText.caption += selection[0].traits.loot.stone + " Stone ";
							if (selection[0].traits.loot.ore)
								ManualText.caption += selection[0].traits.loot.ore + " Ore ";
							ManualText.caption += "\n";
						}
					}

					// Display minimap.
					if (selection[0].traits.minimap)
					{

						if (selection[0].traits.minimap && selection[0].traits.minimap.type)
							ManualText.caption += "Map Type: " + selection[0].traits.minimap.type + "\n";

						if (selection[0].traits.minimap && selection[0].traits.minimap.red && selection[0].traits.minimap.green && selection[0].traits.minimap.blue)
							ManualText.caption += "Map Colour: " + selection[0].traits.minimap.red + "-" + selection[0].traits.minimap.green + "-" + selection[0].traits.minimap.blue + "\n";
					}

					// Armour.
					if (selection[0].traits.armour)
					{
						ManualText.caption += "Armour: ";
						
						if (selection[0].traits.armour.value)
						{
							ManualText.caption += selection[0].traits.armour.value + " [";

							if (selection[0].traits.armour.crush)
								ManualText.caption += "Crush: " + Math.round(selection[0].traits.armour.crush*100) + "%, ";

							if (selection[0].traits.armour.hack)
								ManualText.caption += "Hack: " + Math.round(selection[0].traits.armour.hack*100) + "%, ";

							if (selection[0].traits.armour.pierce)
								ManualText.caption += "Pierce: " + Math.round(selection[0].traits.armour.pierce*100) + "%";

							ManualText.caption += "]\n";
						}
					}

					// Attack.
					if (selection[0].actions.attack)
					{
						ManualText.caption += "Attack: ";
						
						if (selection[0].actions.attack.damage)
						{
							ManualText.caption += selection[0].actions.attack.damage + " [";

							if (selection[0].actions.attack.crush)
								ManualText.caption += "Crush: " + Math.round(selection[0].actions.attack.crush*100) + "%, ";

							if (selection[0].actions.attack.hack)
								ManualText.caption += "Hack: " + Math.round(selection[0].actions.attack.hack*100) + "%, ";

							if (selection[0].actions.attack.pierce)
								ManualText.caption += "Pierce: " + Math.round(selection[0].actions.attack.pierce*100) + "%";

							ManualText.caption += "]\n";
						}

						if (selection[0].actions.attack.range)						
							ManualText.caption += "Attack Range: " + selection[0].actions.attack.range + "\n";

						if (selection[0].actions.attack.accuracy)						
							ManualText.caption += "Attack Accuracy: " + selection[0].actions.attack.accuracy*100 + "%\n";
					}

					// Speed.
					if (selection[0].actions.move)
					{
						if (selection[0].actions.move.speed)
							ManualText.caption += "Speed: " + selection[0].actions.move.speed + "\n";

						// Turn Radius.
						if (selection[0].actions.move.turningradius)
							ManualText.caption += "TurnRadius: " + selection[0].actions.move.turningradius + "\n";
					}

					// Vision.
					if (selection[0].traits.vision)
					{
						if (selection[0].traits.vision.los)
							ManualText.caption += "LOS: " + selection[0].traits.vision.los + "\n";
					}

					// Classes.
					if (selection[0].traits.id.class1)
						ManualText.caption += "Class1: " + selection[0].traits.id.class1 + "\n";
					if (selection[0].traits.id.class2)
						ManualText.caption += "Class2: " + selection[0].traits.id.class2 + "\n";
					if (selection[0].traits.id.class3)
						ManualText.caption += "Class3: " + selection[0].traits.id.class3 + "\n";

					// Name directory.
					if (selection[0].traits.id.personal1 && selection[0].traits.id.personal2)
					ManualText.caption += "Name File: " + selection[0].traits.id.personal1 + " & " + selection[0].traits.id.personal2 + "\n";

					// Internal flag.
					if (selection[0].traits.id.internal_only)
					ManualText.caption += "Internal: " + selection[0].traits.id.internal_only + "\n";

					// Icon.
					if (selection[0].traits.id.icon)
					ManualText.caption += "Icon: " + selection[0].traits.id.icon + "\n";
					if (selection[0].traits.id.icon_cell)
					ManualText.caption += "Icon_Cell: " + selection[0].traits.id.icon_cell + "\n";

					// Version.
					if (selection[0].traits.id.version)
					ManualText.caption += "Version: " + selection[0].traits.id.version + "\n";

					// Lists.
					if (selection[0].actions.create && selection[0].actions.create.list)
					{
						if (selection[0].actions.create.list.unit)
							ManualText.caption += "Trains: " + selection[0].actions.create.list.unit + "\n";
						if (selection[0].actions.create.list.structciv || selection[0].actions.create.list.structmil)
						{
							ManualText.caption += "Builds: ";
							if (selection[0].actions.create.list.structciv)
								ManualText.caption += selection[0].actions.create.list.structciv + " ";
							if (selection[0].actions.create.list.structmil)
								ManualText.caption += selection[0].actions.create.list.structmil + " ";
							ManualText.caption += "\n";
						}
						if (selection[0].actions.create.list.tech)
							ManualText.caption += "Research: " + selection[0].actions.create.list.research + "\n";
					}


					// Display types.
					if (selection[0].traits.id.type)
					{
						ManualTypeString = "";
						if (selection[0].traits.id.type.gaia)
						{
								ManualTypeString += "gaia ";
							if (selection[0].traits.id.type.gaia.group.aqua)
								ManualTypeString += "gaia.group.aqua ";
							if (selection[0].traits.id.type.gaia.group.fauna)
								ManualTypeString += "gaia.group.fauna ";
							if (selection[0].traits.id.type.gaia.group.flora)
								ManualTypeString += "gaia.group.flora ";
							if (selection[0].traits.id.type.gaia.group.geo)
								ManualTypeString += "gaia.group.geo ";
							if (selection[0].traits.id.type.gaia.group.resource)
								ManualTypeString += "gaia.group.resource ";
						}
						if (selection[0].traits.id.type.unit)
						{
							ManualTypeString += "unit ";
							if (selection[0].traits.id.type.unit.group.citizensoldier)
								ManualTypeString += "unit.group.citizensoldier ";
							if (selection[0].traits.id.type.unit.group.hero)
								ManualTypeString += "unit.group.hero ";
							if (selection[0].traits.id.type.unit.group.military)
								ManualTypeString += "unit.group.military ";
							if (selection[0].traits.id.type.unit.group.ship)
								ManualTypeString += "unit.group.ship ";
							if (selection[0].traits.id.type.unit.group.siege)
								ManualTypeString += "unit.group.siege ";
							if (selection[0].traits.id.type.unit.group.superunit)
								ManualTypeString += "unit.group.superunit ";
							if (selection[0].traits.id.type.unit.group.support)
								ManualTypeString += "unit.group.support ";
							if (selection[0].traits.id.type.unit.group.trade)
								ManualTypeString += "unit.group.trade ";
							if (selection[0].traits.id.type.unit.group.warship)
								ManualTypeString += "unit.group.warship ";
							if (selection[0].traits.id.type.unit.group.worker)
								ManualTypeString += "unit.group.worker ";
							if (selection[0].traits.id.type.unit.material.mechanical)
								ManualTypeString += "unit.material.mechanical ";
							if (selection[0].traits.id.type.unit.material.organic)
								ManualTypeString += "unit.material.organic ";
							if (selection[0].traits.id.type.unit.attack.melee)
								ManualTypeString += "unit.attack.melee ";
							if (selection[0].traits.id.type.unit.attack.ranged)
								ManualTypeString += "unit.attack.ranged ";
							if (selection[0].traits.id.type.unit.foot)
							{
								ManualTypeString += "unit.foot ";
								if (selection[0].traits.id.type.unit.foot.bow)
									ManualTypeString += "unit.foot.bow ";
								if (selection[0].traits.id.type.unit.foot.javelin)
									ManualTypeString += "unit.foot.javelin ";
								if (selection[0].traits.id.type.unit.foot.sling)
									ManualTypeString += "unit.foot.sling ";
								if (selection[0].traits.id.type.unit.foot.spear)
									ManualTypeString += "unit.foot.spear ";
								if (selection[0].traits.id.type.unit.foot.sword)
									ManualTypeString += "unit.foot.sword ";
							}
							if (selection[0].traits.id.type.unit.mounted)
							{
								ManualTypeString += "unit.mounted ";
								if (selection[0].traits.id.type.unit.mounted.bow)
									ManualTypeString += "unit.mounted.bow ";
								if (selection[0].traits.id.type.unit.mounted.javelin)
									ManualTypeString += "unit.mounted.javelin ";
								if (selection[0].traits.id.type.unit.mounted.spear)
									ManualTypeString += "unit.mounted.spear ";
								if (selection[0].traits.id.type.unit.mounted.sword)
									ManualTypeString += "unit.mounted.sword ";
								if (selection[0].traits.id.type.unit.weapon.bow)
									ManualTypeString += "unit.weapon.bow ";
								if (selection[0].traits.id.type.unit.weapon.javelin)
									ManualTypeString += "unit.weapon.javelin ";
								if (selection[0].traits.id.type.unit.weapon.sling)
									ManualTypeString += "unit.weapon.sling ";
								if (selection[0].traits.id.type.unit.weapon.spear)
									ManualTypeString += "unit.weapon.spear ";
								if (selection[0].traits.id.type.unit.weapon.sword)
									ManualTypeString += "unit.weapon.sword ";
							}
						}
						if (selection[0].traits.id.type.structure)
						{
							ManualTypeString += "structure ";
							if (selection[0].traits.id.type.structure.group.defensive)
								ManualTypeString += "structure.group.defensive ";
							if (selection[0].traits.id.type.structure.group.housing)
								ManualTypeString += "structure.group.housing ";
							if (selection[0].traits.id.type.structure.group.offensive)
								ManualTypeString += "structure.group.offensive ";
							if (selection[0].traits.id.type.structure.group.supply)
								ManualTypeString += "structure.group.supply ";
							if (selection[0].traits.id.type.structure.group.support)
								ManualTypeString += "structure.group.support ";
							if (selection[0].traits.id.type.structure.group.tower)
								ManualTypeString += "structure.group.tower ";
							if (selection[0].traits.id.type.structure.group.train)
								ManualTypeString += "structure.group.train ";
							if (selection[0].traits.id.type.structure.group.wall)
								ManualTypeString += "structure.group.wall ";
							if (selection[0].traits.id.type.structure.phase.cp)
								ManualTypeString += "structure.phase.cp ";
							if (selection[0].traits.id.type.structure.phase.tp)
								ManualTypeString += "structure.phase.tp ";
							if (selection[0].traits.id.type.structure.phase.vp)
								ManualTypeString += "structure.phase.vp ";
							if (selection[0].traits.id.type.structure.material.stone)
								ManualTypeString += "structure.material.stone ";
							if (selection[0].traits.id.type.structure.material.wood)
								ManualTypeString += "structure.material.wood ";
						}

						if (ManualTypeString != "")
							ManualText.caption = ManualText.caption + "Type(s): " + ManualTypeString + "\n";
					}

					// Display history text.
					if (selection[0].traits.id.history)
					{
						ManualHistory = getGUIObjectByName("MANUAL_HISTORY");
						ManualHistory.caption = "History: " + selection[0].traits.id.history;
					}
}


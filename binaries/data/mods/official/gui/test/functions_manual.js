function manualDisplay()
{
					// Display heading.
					ManualBkg = getGUIObjectByName("manual_bkg");
					ManualBkg.caption = "In-Game Help: ";
					if (selection[0].traits.id.civ)
						ManualBkg.caption += selection[0].traits.id.civ + ": ";
					if (selection[0].traits.id.specific)
						ManualBkg.caption += selection[0].traits.id.specific;

					// Display portrait.
					if (selection[0].traits.id.icon)
					{
						if (selection[0].traits.id.icon_cell && selection[0].traits.id.icon_cell != "")
							setPortrait("manual_portrait", selection[0].traits.id.icon + "_" + selection[0].traits.id.icon_cell);
						else
							setPortrait("manual_portrait", selection[0].traits.id.icon);
					}

					// Display rollover text.
					if (selection[0].traits.id.rollover)
					{
						ManualRollover = getGUIObjectByName("manual_rollover");
						ManualRollover.caption = selection[0].traits.id.rollover;
					}

					// Display name(s).
					ManualRollover = getGUIObjectByName("manual_name");
					ManualRollover.caption = "";
					if (selection[0].traits.id.generic)
						ManualRollover.caption += selection[0].traits.id.generic;
					if (selection[0].traits.id.generic && selection[0].traits.id.specific)
						ManualRollover.caption += " - ";
					if (selection[0].traits.id.specific)
						ManualRollover.caption += selection[0].traits.id.specific;
					if (selection[0].traits.id.specific && selection[0].traits.id.ranked)
						ManualRollover.caption += " - ";
					if (selection[0].traits.id.ranked)
						ManualRollover.caption += selection[0].traits.id.ranked;
					// Personal name.
					if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
						ManualRollover.caption += " - " + selection[0].traits.id.personal;
					ManualRollover.caption += "\n";

					// Display civilisation.
					if (selection[0].traits.id.civ && selection[0].traits.id.civ_code)
						ManualRollover.caption += "Civilisation: " + selection[0].traits.id.civ + " (" + selection[0].traits.id.civ_code + ")" + "\n";
					if (!selection[0].traits.id.civ_code)
						ManualRollover.caption += "Civilisation: " + selection[0].traits.id.civ + "\n";
					
					// Display hitpoints.
					if (selection[0].traits.health.curr && selection[0].traits.health.max && selection[0].traits.health)
						ManualRollover.caption += "Hitpoints: " + selection[0].traits.health.curr + "/" + selection[0].traits.health.max + "\n";

					// Display rank.
					if (selection[0].traits.up.rank)
						ManualRollover.caption += "Rank: " + selection[0].traits.up.rank + "\n";

					// Display experience.
					if (selection[0].traits.up && selection[0].traits.up.curr && selection[0].traits.up.req)
						ManualRollover.caption += "XP: " + selection[0].traits.up.curr + "/" + selection[0].traits.up.req + "\n";

					// Display population.
					if (selection[0].traits.population)
					{
						if (selection[0].traits.population.sub)
							ManualRollover.caption += "Pop: -" + selection[0].traits.population.sub + "\n";
						if (selection[0].traits.population.add)
							ManualRollover.caption += "Housing: +" + selection[0].traits.population.add + "\n";
					}

					// Display garrison.
					if (selection[0].traits.garrison)
					{
						if (selection[0].traits.garrison.curr && selection[0].traits.garrison.max)
							ManualRollover.caption += "Garrison: " + selection[0].traits.garrison.curr + "/" + selection[0].traits.garrison.max + "\n";
					}

					// Display supply.
					if (selection[0].traits.supply)
					{
						if (selection[0].traits.supply.curr && selection[0].traits.supply.max && selection[0].traits.supply.type)
						{
							// If Supply is infinite,
							if (selection[0].traits.supply.curr == "0" && selection[0].traits.supply.max == "0")
								// Use infinity symbol.
								ManualRollover.caption += "Supply: " + selection[0].traits.supply.curr + "/" + selection[0].traits.supply.max + " " + selection[0].traits.supply.type + " (" + selection[0].traits.supply.subtype + ")\n";
							else
								// Use numbers.
								ManualRollover.caption += "Supply: 8 " + selection[0].traits.supply.type + " (" + selection[0].traits.supply.subtype + ")\n";
						}
					}

					if (selection[0].traits.loot)
					{
						// Display UP on death.
						if (selection[0].traits.loot.up)
							ManualRollover.caption += "UP: " + selection[0].traits.loot.up + "\n";
	
						// Display loot.
						if (selection[0].traits.loot.food || selection[0].traits.loot.wood || selection[0].traits.loot.stone || selection[0].traits.loot.ore)
						{
							ManualRollover.caption += "Loot: ";
							if (selection[0].traits.loot.food)
								ManualRollover.caption += selection[0].traits.loot.food + " Food ";
							if (selection[0].traits.loot.wood)
								ManualRollover.caption += selection[0].traits.loot.wood + " Wood ";
							if (selection[0].traits.loot.stone)
								ManualRollover.caption += selection[0].traits.loot.stone + " Stone ";
							if (selection[0].traits.loot.ore)
								ManualRollover.caption += selection[0].traits.loot.ore + " Ore ";
							ManualRollover.caption += "\n";
						}
					}

					// Display minimap.
					if (selection[0].traits.minimap)
					{

						if (selection[0].traits.minimap && selection[0].traits.minimap.type)
							ManualRollover.caption += "Map Type: " + selection[0].traits.minimap.type + "\n";

						if (selection[0].traits.minimap && selection[0].traits.minimap.red && selection[0].traits.minimap.green && selection[0].traits.minimap.blue)
							ManualRollover.caption += "Map Colour: " + selection[0].traits.minimap.red + "-" + selection[0].traits.minimap.green + "-" + selection[0].traits.minimap.blue + "\n";
					}

					// Armour.
					if (selection[0].traits.armour)
					{
						ManualRollover.caption += "Armour: ";
						
						if (selection[0].traits.armour.value)
						{
							ManualRollover.caption += selection[0].traits.armour.value + " [";

							if (selection[0].traits.armour.crush)
								ManualRollover.caption += "Crush: " + Math.round(selection[0].traits.armour.crush*100) + "%, ";

							if (selection[0].traits.armour.hack)
								ManualRollover.caption += "Hack: " + Math.round(selection[0].traits.armour.hack*100) + "%, ";

							if (selection[0].traits.armour.pierce)
								ManualRollover.caption += "Pierce: " + Math.round(selection[0].traits.armour.pierce*100) + "%";

							ManualRollover.caption += "]\n";
						}
					}

					// Attack.
					if (selection[0].actions.attack)
					{
						ManualRollover.caption += "Attack: ";
						
						if (selection[0].actions.attack.damage)
						{
							ManualRollover.caption += selection[0].actions.attack.damage + " [";

							if (selection[0].actions.attack.crush)
								ManualRollover.caption += "Crush: " + Math.round(selection[0].actions.attack.crush*100) + "%, ";

							if (selection[0].actions.attack.hack)
								ManualRollover.caption += "Hack: " + Math.round(selection[0].actions.attack.hack*100) + "%, ";

							if (selection[0].actions.attack.pierce)
								ManualRollover.caption += "Pierce: " + Math.round(selection[0].actions.attack.pierce*100) + "%";

							ManualRollover.caption += "]\n";
						}

						if (selection[0].actions.attack.range)						
							ManualRollover.caption += "Attack Range: " + selection[0].actions.attack.range + "\n";

						if (selection[0].actions.attack.accuracy)						
							ManualRollover.caption += "Attack Accuracy: " + selection[0].actions.attack.accuracy*100 + "%\n";
					}

					// Speed.
					if (selection[0].actions.move)
					{
						if (selection[0].actions.move.speed)
							ManualRollover.caption += "Speed: " + selection[0].actions.move.speed + "\n";

						// Turn Radius.
						if (selection[0].actions.move.turningradius)
							ManualRollover.caption += "TurnRadius: " + selection[0].actions.move.turningradius + "\n";
					}

					// Vision.
					if (selection[0].traits.vision)
					{
						if (selection[0].traits.vision.los)
							ManualRollover.caption += "LOS: " + selection[0].traits.vision.los + "\n";
					}

					// Classes.
					if (selection[0].traits.id.class1)
						ManualRollover.caption += "Class1: " + selection[0].traits.id.class1 + "\n";
					if (selection[0].traits.id.class2)
						ManualRollover.caption += "Class2: " + selection[0].traits.id.class2 + "\n";
					if (selection[0].traits.id.class3)
						ManualRollover.caption += "Class3: " + selection[0].traits.id.class3 + "\n";

					// Name directory.
					if (selection[0].traits.id.personal1 && selection[0].traits.id.personal2)
					ManualRollover.caption += "Name File: " + selection[0].traits.id.personal1 + " & " + selection[0].traits.id.personal2 + "\n";

					// Internal flag.
					if (selection[0].traits.id.internal_only)
					ManualRollover.caption += "Internal: " + selection[0].traits.id.internal_only + "\n";

					// Icon.
					if (selection[0].traits.id.icon)
					ManualRollover.caption += "Icon: " + selection[0].traits.id.icon + "\n";
					if (selection[0].traits.id.icon_cell)
					ManualRollover.caption += "Icon_Cell: " + selection[0].traits.id.icon_cell + "\n";

					// Version.
					if (selection[0].traits.id.version)
					ManualRollover.caption += "Version: " + selection[0].traits.id.version + "\n";

					// Lists.
					if (selection[0].actions.create)
					{
						if (selection[0].actions.create.unitlist)
							ManualRollover.caption += "Trains: " + selection[0].actions.create.unitlist + "\n";
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
							ManualRollover.caption = ManualRollover.caption + "Type(s): " + ManualTypeString + "\n";
					}

					// Display history text.
					if (selection[0].traits.id.history)
					{
						ManualHistory = getGUIObjectByName("manual_history");
						ManualHistory.caption = "History: " + selection[0].traits.id.history;
					}
}


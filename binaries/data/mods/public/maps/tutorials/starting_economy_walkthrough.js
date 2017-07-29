let tutorialGoals = [
	{
		"instructions": markForTranslation("This tutorial will teach the basics of developing your economy. Typically, you will start with a Civic Center and a couple units in 'Village Phase' and ultimately, your goal will be to develop and expand your empire, often by evolving to 'Town Phase' and 'City Phase' afterward.\n\nBefore starting, you can toggle between fullscreen and windowed mode using Alt+Enter. You can also change the level of zoom using the mouse wheel and the camera view using any of your keyboard's arrow keys.\nAdjust the game window to your preferences.\n"),
	},
	{
		"instructions": markForTranslation("To start off, select your building, the Civic Center, by clicking on it. A selection ring in the color of your civilization will be displayed after clicking."),
	},
	{
		"instructions": markForTranslation("Now that the Civic Center is selected, you will notice that a production panel will appear on the lower right of your screen detailing the actions that the buildings support. For the production panel, available actions are not masked in any color, while an icon masked in either grey or red indicates that the action has not been unlocked or you do not have sufficient resources to perform that action, respectively. Additionally, you can hover your mouse over any icon to show a tooltip with more details.\nThe top row of buttons contains portraits of units that may be trained at the building while the bottom one or two rows will have researchable technologies. Hover your mouse over the 'II' icon. The tooltip will tell us that advancing to 'Town Phase' requires both more constructed structures as well as more Food and Wood resources."),
	},
	{
		"instructions": markForTranslation("You have two main types of starting units: Female Citizens and Citizen Soldiers. Female Citizens are purely economic units; they have low HP, no armour, and little to no attack. Citizen Soldiers are workers by default, but in times of need, can utilizing a weapon to fight. You have two categories of Citizen Soldiers: Infantry and Cavalry. Female Citizens and Infantry Citizen Soldiers can gather any land resources while Cavalry Citizen Soldiers can only gather meat from hunted animals.\n"),
	},
	{
		"instructions": markForTranslation("As a general rule of thumb, left-clicking represents selection while right-clicking with an entity selected represents an order (gather, build, fight, etc.).\n"),
	},
	{
		"instructions": markForTranslation("At this point, Food and Wood are the most important resources for developing your economy, so let's start with gathering food. Females gather non-meat food faster than their male counterparts.\nThere are primarily three ways to select units:\n1) Hold the left mouse button and drag a selection rectangle that encloses the units you want to select.\n2) Click on one of them and then add additional units to your selection by shift-clicking each additional unit (or also via the above selection rectangle).\n3) Double-click on a unit. This will select every unit of the same type as the specified unit in your visible window. Triple-click will select all units of the same type on the entire map.\n You can click on an empty space on the map to reset the selection. Try each of these methods before tasking all of your Female Citizens to gather the grapes to the southeast of your Civic Center by right-clicking on the grapes when you have all the Female Citizens selected."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "fruit")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Now, let's gather some Wood with your Infantry Citizen Soldiers. Select your Infantry Citizen Soldiers and order them to gather Wood by right-clicking on the nearest tree."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "tree")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Cavalry Citizen Soldiers are good for hunting. Select your cavalry and order him to hunt the chicken around your Civic Center in similar fashion."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "meat")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("All your units are now gathering resources. We should train more units!\nFirst, let's set a rally-point. Setting a rally point on a building that can train units will automatically designate a task to the new unit upon completion of training. We want to send the newly trained units to gather Wood on the group of trees to the south of the Civic Center. To do so, select the Civic Center by clicking on it and then right-click on one of the trees.\nRally-Points are indicated by a small flag at the end of the blue line."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data ||
			   !msg.cmd.data.command || msg.cmd.data.command != "gather" ||
			   !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "tree")
			{
				this.WarningMessage(markForTranslation("Select the Civic Center, then hover your mouse over a tree and right-click when you see the cursor change into a Wood icon."));
				return;
			}
			this.NextGoal();
		}

	},
	{
		"instructions": markForTranslation("Now that the rally-point is set, we can produce additional units and they will do their assigned task automatically.\nAs Citizen Soldiers are better than Female Citizens for gathering Wood, select the Civic Center and shift-click on the second unit icon, the hoplites (shift-clicking produces a batch of five units). You can also train units individually by simply clicking, but training 5 units together takes less time than training 5 units individually."),
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_infantry_spearman_b" || +msg.count == 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to shift-click to produce several units.") :
					markForTranslation("Shift-click on the HOPLITE icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Let's wait for the units to be trained.\nWhile waiting, direct your attention to the panel at the top of your screen. On the upper left, you will see your current resource supply (Food, Wood, Stone, and Metal). As each worker brings resources back to the Civic Center (or another dropsite), you will see the amount of the corresponding resource increase. This is a very important concept to keep in mind: gathered resources have to be brought back to a dropsite to be accounted, and you should always try to minimize the distance between resource and nearest dropsite to improve your gathering efficiency."),
		"OnTrainingFinished": function(msg)
		{
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The newly trained units automatically go to the trees and start gathering Wood.\nBut as they have to bring it back to the Civic Center to deposit it, their gathering efficiency suffers from the distance. To fix that, we can build a storehouse, a dropsite for Wood, Stone, and Metal, close to the trees. To do so, select your five newly trained Citizen Solders and look for the construction panel on the bottom right, click on the storehouse icon, move the mouse as close as possible to the trees you want to collect and click on a valid place to build the dropsite.\nInvalid (obstructed) positions will show the building preview overlay in red."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_storehouse")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The selected workers will automatically start constructing the building once you place the foundation."),
		"OnStructureBuilt": function(msg)
		{
			let cmpResourceDropsite = Engine.QueryInterface(msg.building, IID_ResourceDropsite);
			if (cmpResourceDropsite && cmpResourceDropsite.AcceptsType("wood"))
				this.NextGoal();
		},
	},
	{
		"instructions": markForTranslation("When construction finishes, the builders default to gathering Wood automatically.\nLet's train some female workers to gather more food. Select the Civic Center and shift-click on the female citizen icon to train 5."),
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_support_female_citizen" || +msg.count == 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to shift-click to produce several units.") :
					markForTranslation("Shift-click on the FEMALE CITIZEN icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Let's wait for the units to be trained.\nIn the meantime, we seem to have enough workers gathering Wood. We should remove the current rally-point of the Civic Center away from gathering Wood. For that purpose, right-click on the Civic Center when it is selected (and the flag icon indicating the rally-point is crossed out)."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "unset-rallypoint")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The units should be ready soon.\nIn the meantime, direct your attention to your population count on the top panel. It is the fifth item from the left, after the resources. It would be prudent to keep an eye on it. It indicates your current population (including those being trained) and the current population limit, which is determined by your built structures."),
		"OnTrainingFinished": function(msg)
		{
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("As you have nearly reached the population limit, you must increase it by building some new structures if you want to train more units. The most cost effective structure to increase your population limit is the house.\nNow that the units are ready, let's build two houses."),
	},
	{
		"instructions": markForTranslation("Select two of your newly trained Female Citizens and ask them to build these houses in the empty space to the east of the Civic Center. To do so, after selecting the Female Citizens, click on the house icon in the bottom right panel and shift-click on the position in the map where you want to build the first house followed by a shift-click on the position of the second house (when giving orders, shift-click put the order in the queue and the units will automatically switch to the next order in their queue when they finish their work). Press Escape to get rid of the house cursor so you don't spam houses all over the map.\nReminder: to select only two Female Citizens, click on the first one and then shift-click on the second one to add her to the selection."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_house" &&
				++this.count == 2)
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("You may notice that berries are a finite supply of food. We will need a more lasting food source. Fields produce an unlimited food resource, but are slower to gather than forageable fruits.\nBut to minimize the distance between a farm and its corresponding food dropsite, we will first build a farmstead."),
	},
	{
		"instructions": markForTranslation("Select the three remaining (idle) Female Citizens and order them to build a farmstead in the center of the large open area to the west of the Civic Center.\nWe will need a decent chunk of space around the farmstead to build fields. In addition, we can see goats on the west side to further improve our food gathering efficiency should we ever decide to hunt them.\nIf you try to select the three idle Female Citizens by selection rectangle them, there is a high chance that one additional gatherers are included in your selection rectangle. To prevent that, hold the 'i' key while grabing: only idle units are then selected. If during your selection you select the cavalry which may now be idle, you can remove it by pressing the control key while clicking on its corresponding portrait in the selection panel on the bottom."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_farmstead")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("When the farmstead construction is finished, its builders will automatically look for food, and in this case, they will go after the nearby goats.\nBut your house builders will only look for something else to build and, if nothing found, become idle. Let's wait for them to build the houses."),
		"OnStructureBuilt": function(msg)
		{warn("h1: " + this.count);
			if (TriggerHelper.EntityHasClass(msg.building, "House") && ++this.count == 2)
				this.NextGoal();
			warn("h2: " + this.count);
		},
	},
	{
		"name": "buildField",
		"instructions": markForTranslation("When both houses are built, select your two Female Citizens and order them to build a field as close as possible to the farmstead, which is a dropsite for all types of food."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_field")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("When the field is constructed, the builders will automatically start gathering it.\nThe cavalry unit should have slaughtered all chicken by now. Select it and right-click on one of the goats in the west to start hunting them for food."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "meat")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("A field can have up to five farmers working on it. To add additional gathereres, select the Civic Center and setup a rally-point on a field by right-clicking on it. As long as the field is not yet build, new workers sent by a rally-point will help building it, while they will gather it when built."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data || !msg.cmd.data.command ||
			   (msg.cmd.data.command != "build" || !msg.cmd.data.target || !TriggerHelper.EntityHasClass(msg.cmd.data.target, "Field")) &&
			   (msg.cmd.data.command != "gather" || !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "grain"))
			{
				this.WarningMessage(markForTranslation("Select the Civic Center and right-click on the field."));
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Now click three times on the female icon in the bottom right panel to train three additional farmers."),
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_support_female_citizen" || +msg.count != 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do a simple left-click to produce a single unit.") :
					markForTranslation("Click on the FEMALE CITIZEN icon.");
				this.WarningMessage(txt);
				return;
			}
			if (++this.count < 3)
				return;
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("You can increase the gather rates of your workers by researching new technologies available in some buildings.\nThe farming rate, for example, can be improved with a researchable technology in the farmstead. Select the farmstead and look at its production panel on the bottom right. You will see several researchable technologies. Hover the mouse over them to see their costs and effects and click on the one you want to research."),
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityHasClass(msg.researcherEntity, "Farmstead"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("We should start preparing to phase up into 'Town Phase', which will unlock many more units and buildings. Select the Civic Center and hover the mouse over the 'Town Phase' icon to see what is still needed.\nWe now have enough resources, but one structure is missing. Although this is an economic tutorial, it is nonetheless useful to be prepared for defense in case of attack, so let's build barracks.\nSelect four of your soldiers and ask them to build a barracks: as before, start selecting the soldiers, click on the barracks icon in the production panel and then lay down a foundation not far from your Civic Center where you want to build."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_barracks")
				this.NextGoal();
		}

	},
	{
		"instructions": markForTranslation("Let's wait for the barracks to be built. As this construction is lengthy, you can add two soldiers to build it faster. To do so, select your Civic Center and set up a rally-point on the barracks foundation by right-clicking on it (you should see a hammer icon), and then produce two more builders by clicking on the hoplite icon twice."),
		"OnStructureBuilt": function(msg)
		{
			if (TriggerHelper.EntityHasClass(msg.building, "Barracks"))
				this.NextGoal();
		},
	},
	{
		"instructions": markForTranslation("You should now be able research 'Town Phase'. Select the Civic Center and click on the technology icon.\nIf you still miss some resources (icon with red overlay), wait for them to be gathered by your workers."),
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityHasClass(msg.researcherEntity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("In later phases, you need usually Stone and Metal to build bigger structures and train better soldiers. Hence, while waiting for the research to be done, you will send half of your idle Citizen Soldiers (who have finished building the Barracks) to gather Stone and the other half to gather Metal.\nTo do so, we could select three Citizen Soldiers and right-click on the Stone mine on the west of the Civic Center (a Stone cursor is shown when you hover the mouse over it while your soldiers are selected). However, these soldiers were gathering Wood, so they may still carry some Wood which would be lost when starting to gather another resource."),
	},
	{
		"instructions": markForTranslation("Thus, we should order them to deposit their Wood in the Civic Center along the way. To do so, we will queue orders with shift-click: select your soldiers, shift-right-click on the Civic Center to deposit their Wood and then shift-right-click on the Stone mine to gather it.\nPerform a similar order queue with the remaining soldiers and the Metal mine in the west."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).generic == "stone" &&
				++this.count == 3)
				this.NextGoal();
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).generic == "metal" &&
				++this.count == 3)
				this.NextGoal();
		},
		"OnResearchFinished": function(msg)
		{
			if (msg.tech == "phase_town_athen" && ++this.count == 3)
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("This is the end of the walkthrough. This should give you a good idea of the basics of setting up your economy.")
	}
];

Trigger.prototype.tutorialGoals = tutorialGoals;
var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
cmpTrigger.RegisterTrigger("OnInitGame", "InitTutorial", { "enabled": true });

Trigger.prototype.tutorialGoals = [
	{
		"instructions": [
			markForTranslation("This tutorial will teach the basics of developing your economy. Typically, you will start with a Civic Center and a couple units in Village Phase and ultimately, your goal will be to develop and expand your empire, often by evolving to Town Phase and City Phase afterward.\n"),
			{
				"text": markForTranslation("\nBefore starting, you can toggle between fullscreen and windowed mode using %(hotkey)s."),
				"hotkey": ["togglefullscreen"]
			},
			markForTranslation("You can change the level of zoom using the mouse wheel and the camera view using any of your keyboard's arrow keys.\n"),
			markForTranslation("Adjust the game window to your preferences.\n"),
			{
				"text": markForTranslation("\nYou may also toggle between showing and hiding this tutorial panel at any moment using %(hotkey)s.\n"),
				"hotkey": ["session.gui.tutorial.toggle"]
			}
		]
	},
	{
		"instructions": [
			markForTranslation("To start off, select your building, the Civic Center, by clicking on it. A selection ring in the color of your civilization will be displayed after clicking.")
		]
	},
	{
		"instructions": [
			markForTranslation("Now that the Civic Center is selected, you will notice that a production panel will appear on the lower right of your screen detailing the actions that the buildings supports. For the production panel, available actions are not masked in any color, while an icon masked in either grey or red indicates that the action has not been unlocked or you do not have sufficient resources to perform that action, respectively. Additionally, you can hover the cursor over any icon to show a tooltip with more details.\n"),
			markForTranslation("The top row of buttons contains portraits of units that may be trained at the building while the bottom one or two rows will have researchable technologies. Hover the cursor over the II icon. The tooltip will tell us that advancing to Town Phase requires both more constructed structures as well as more Food and Wood resources.")
		]
	},
	{
		"instructions": [
			markForTranslation("You have two main types of starting units: female citizens and citizen soldiers. Female citizens are purely economic units; they have low HP, no armor, and little to no attack. Citizen soldiers are workers by default, but in times of need, can utilize a weapon to fight. You have two categories of citizen soldiers: infantry and cavalry. Female citizens and infantry citizen soldiers can gather any land resources while cavalry citizen soldiers can only gather meat from hunted animals.\n")
		]
	},
	{
		"instructions": [
			markForTranslation("As a general rule of thumb, left-clicking represents selection while right-clicking with an entity selected represents an order (gather, build, fight, etc.).\n")
		]
	},
	{
		"instructions": [
			markForTranslation("At this point, food and wood are the most important resources for developing your economy, so let's start with gathering food. Female citizens gather vegetables faster than other units.\n"),
			markForTranslation("There are primarily three ways to select units:\n"),
			markForTranslation("1) Hold the left mouse button and drag a selection rectangle that encloses the units you want to select.\n"),
			markForTranslation("2) Click on one of them and then add additional units to your selection by holding Shift and clicking each additional unit (or also via the above selection rectangle).\n"),
			markForTranslation("3) Double-click on a unit. This will select every unit of the same type as the specified unit in your visible window. Triple-click will select all units of the same type on the entire map.\n"),
			markForTranslation("You can click on an empty space on the map to reset the selection. Try each of these methods before tasking all of your female citizens to gather the grapes to the southeast of your Civic Center by right-clicking on the grapes when you have all the female citizens selected.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "fruit")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("Now, let's gather some Wood with your Infantry Citizen Soldiers. Select your Infantry Citizen Soldiers and order them to gather Wood by right-clicking on the nearest tree.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "tree")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("Cavalry Citizen Soldiers are good for hunting. Select your cavalry and order him to hunt the chickens around your Civic Center in similar fashion.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
				TriggerHelper.GetResourceType(msg.cmd.target).specific == "meat")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("All your units are now gathering resources. We should train more units!\n"),
			markForTranslation("First, let's set a rally point. Setting a rally point on a building that can train units will automatically designate a task to the new unit upon completion of training. We want to send the newly trained units to gather Wood on the group of trees to the south of the Civic Center. To do so, select the Civic Center by clicking on it and then right-click on one of the trees.\n"),
			markForTranslation("Rally points are indicated by a small flag at the end of the blue line.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data ||
			   !msg.cmd.data.command || msg.cmd.data.command != "gather" ||
			   !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "tree")
			{
				this.WarningMessage(markForTranslation("Select the Civic Center, then hover the cursor over a tree and right-click when you see the cursor change into a Wood icon."));
				return;
			}
			this.NextGoal();
		}

	},
	{
		"instructions": [
			markForTranslation("Now that the rally point is set, we can produce additional units and they will do their assigned task automatically.\n"),
			markForTranslation("Citizen soldiers gather wood faster than female citizens. Select the Civic Center and, while holding Shift, click on the second unit icon, the hoplites (holding Shift trains a batch of five units). You can also train units individually by simply clicking, but training 5 units together takes less time than training 5 units individually.")
		],
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_infantry_spearman_b" || +msg.count == 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to hold Shift while clicking to train several units.") :
					markForTranslation("Hold Shift and click on the Hoplite icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("Let's wait for the units to be trained.\n"),
			markForTranslation("While waiting, direct your attention to the panel at the top of your screen. On the upper left, you will see your current resource supply (Food, Wood, Stone, and Metal). As each worker brings resources back to the Civic Center (or another dropsite), you will see the amount of the corresponding resource increase.\n"),
			markForTranslation("This is a very important concept to keep in mind: gathered resources have to be brought back to a dropsite to be accounted, and you should always try to minimize the distance between resource and nearest dropsite to improve your gathering efficiency.")
		],
		"OnTrainingFinished": function(msg)
		{
			this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("The newly trained units automatically go to the trees and start gathering Wood.\n"),
			markForTranslation("But as they have to bring it back to the Civic Center to deposit it, their gathering efficiency suffers from the distance. To fix that, we can build a storehouse, a dropsite for Wood, Stone, and Metal, close to the trees. To do so, select your five newly trained Citizen Soldiers and look for the construction panel on the bottom right, click on the storehouse icon, move the mouse as close as possible to the trees you want to gather and click on a valid place to build the dropsite.\n"),
			markForTranslation("Invalid (obstructed) positions will show the building preview overlay in red.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_storehouse")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("The selected citizens will automatically start constructing the building once you place the foundation.")
		],
		"OnStructureBuilt": function(msg)
		{
			let cmpResourceDropsite = Engine.QueryInterface(msg.building, IID_ResourceDropsite);
			if (cmpResourceDropsite && cmpResourceDropsite.AcceptsType("wood"))
				this.NextGoal();
		},
	},
	{
		"instructions": [
			markForTranslation("When construction finishes, the builders default to gathering Wood automatically.\n"),
			markForTranslation("Let's train some female citizens to gather more food. Select the Civic Center, hold Shift and click on the female citizen icon to train 5 female citizens.")
		],
		"Init": function()
		{
			this.trainingDone = false;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_support_female_citizen" || +msg.count == 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to hold Shift and click to train several units.") :
					markForTranslation("Hold shift and click on the Female Citizen icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("Let's wait for the units to be trained.\n"),
			markForTranslation("In the meantime, we seem to have enough workers gathering Wood. We should remove the current rally point of the Civic Center away from gathering Wood. For that purpose, right-click on the Civic Center when it is selected (and the flag icon indicating the rally point is crossed out).")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "unset-rallypoint")
				this.NextGoal();
		},
		"OnTrainingFinished": function(msg)
		{
			this.trainingDone = true;
		}
	},
	{
		"instructions": [
			markForTranslation("The units should be ready soon.\n"),
			markForTranslation("In the meantime, direct your attention to your population count on the top panel. It is the fifth item from the left, after the resources. It would be prudent to keep an eye on it. It indicates your current population (including those being trained) and the current population limit, which is determined by your built structures.")
		],
		"IsDone": function(msg)
		{
			return this.trainingDone;
		},
		"OnTrainingFinished": function(msg)
		{
			this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("As you have nearly reached the population limit, you must increase it by building some new structures if you want to train more units. The most cost effective structure to increase your population limit is the house.\n"),
			markForTranslation("Now that the units are ready, let's see how to build several houses in a row.")
		]
	},
	{
		"instructions": [
			markForTranslation("Select two of your newly-trained female citizens and ask them to build these houses in the empty space to the east of the Civic Center. To do so, after selecting the female citizens, click on the house icon in the bottom right panel and, while holding Shift, click first on the position in the map where you want to build the first house, and then click on the position where you want to build the second house (when you give a command while holding Shift, you put the command in a queue; units automatically switch to the next command in their queue when they finish their current command). Press Escape to get rid of the house cursor so you don't spam houses all over the map.\n"),
			markForTranslation("Reminder: to select only two female citizens, click on the first one and then hold Shift and click on the second one.")
		],
		"Init": function()
		{
			this.houseGoal = new Set();
			this.houseCount = 0;
		},
		"IsDone": function()
		{
			return this.houseCount > 1;
		},
		"OnOwnershipChanged": function(msg)
		{
			if (msg.from != INVALID_PLAYER && this.houseGoal.has(+msg.entity))
			{
				this.houseGoal.delete(+msg.entity);
				let cmpFoundation = Engine.QueryInterface(+msg.entity, IID_Foundation);
				if (cmpFoundation && cmpFoundation.GetBuildProgress() < 1)	// Destroyed before built
					--this.houseCount;
			}
			else if (msg.from == INVALID_PLAYER && msg.to == this.playerID &&
			         Engine.QueryInterface(+msg.entity, IID_Foundation) &&
			         TriggerHelper.EntityMatchesClassList(+msg.entity, "House"))
			{
				this.houseGoal.add(+msg.entity);
				++this.houseCount;
				if (this.IsDone())
					this.NextGoal();
			}
		}
	},
	{
		"instructions": [
			markForTranslation("You may notice that berries are a finite supply of food. We will need a more lasting food source. Fields produce an unlimited food resource, but are slower to gather than forageable fruits.\n"),
			markForTranslation("But to minimize the distance between a farm and its corresponding food dropsite, we will first build a farmstead.")
		],
		"delay": -1,
		"OnOwnershipChanged": function(msg)
		{
			if (this.houseGoal.has(+msg.entity))
				this.houseGoal.delete(+msg.entity);
		}
	},
	{
		"instructions": [
			markForTranslation("Select the three remaining (idle) female citizens and order them to build a farmstead in the center of the large open area to the west of the Civic Center.\n"),
			markForTranslation("We will need a decent chunk of space around the farmstead to build fields. In addition, we can see goats on the west side to further improve our food gathering efficiency should we ever decide to hunt them.\n"),
			markForTranslation("If you try to select the three idle female citizens by clicking and dragging a selection rectangle over them, you might accidentally select additional units. To avoid that, hold the I key while selecting so that only idle units are selected. If you accidentally select a cavalry unit, hold Ctrl and click on the cavalry unit icon of the selection panel at the bottom of the screen to remove the cavalry unit from the current selection.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_farmstead")
				this.NextGoal();
		},
		"OnOwnershipChanged": function(msg)
		{
			if (this.houseGoal.has(+msg.entity))
				this.houseGoal.delete(+msg.entity);
		}
	},
	{
		"instructions": [
			markForTranslation("When the farmstead construction is finished, its builders will automatically look for food, and in this case, they will go after the nearby goats.\n"),
			markForTranslation("But your house builders will only look for something else to build and, if nothing found, become idle. Let's wait for them to build the houses.")
		],
		"IsDone": function()
		{
			return !this.houseGoal.size;
		},
		"OnOwnershipChanged": function(msg)
		{
			if (this.houseGoal.has(+msg.entity))
				this.houseGoal.delete(+msg.entity);
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("When both houses are built, select your two female citizens and order them to build a field as close as possible to the farmstead, which is a dropsite for all types of food.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_field")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("When the field is ready, the builders will automatically start gathering it.\n"),
			markForTranslation("The cavalry unit should have slaughtered all chickens by now. Select it and explore the south-west area: there is a lake with some camels around. Move your cavalry by right-clicking on the point you want to go, and when you see a herd of camels, right-click on one of them to start hunting for food.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
			    TriggerHelper.GetResourceType(msg.cmd.target).specific == "meat")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("A field can have up to five farmers working on it. To add additional gatherers, select the Civic Center and setup a rally point on a field by right-clicking on it. As long as the field is not yet build, new workers sent by a rally point will help building it, while they will gather it when built.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data || !msg.cmd.data.command ||
			   (msg.cmd.data.command != "build" || !msg.cmd.data.target || !TriggerHelper.EntityMatchesClassList(msg.cmd.data.target, "Field")) &&
			   (msg.cmd.data.command != "gather" || !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "grain"))
			{
				this.WarningMessage(markForTranslation("Select the Civic Center and right-click on the field."));
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("Now click three times on the female citizen icon in the bottom right panel to train three additional farmers.")
		],
		"Init": function(msg)
		{
			this.femaleCount = 0;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/athen_support_female_citizen" || +msg.count != 1)
			{
				let entity = msg.trainerEntity;
				let cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count != 1 ?
					markForTranslation("Click without holding Shift to train a single unit.") :
					markForTranslation("Click on the Female Citizen icon.");
				this.WarningMessage(txt);
				return;
			}
			if (++this.femaleCount == 3)
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("You can increase the gather rates of your workers by researching new technologies available in some buildings.\n"),
			markForTranslation("The farming rate, for example, can be improved with a researchable technology in the farmstead. Select the farmstead and look at its production panel on the bottom right. You will see several researchable technologies. Hover the cursor over them to see their costs and effects and click on the one you want to research.")
		],
		"IsDone": function()
		{
			return TriggerHelper.HasDealtWithTech(this.playerID, "gather_wicker_baskets") ||
			       TriggerHelper.HasDealtWithTech(this.playerID, "gather_farming_plows");
		},
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "Farmstead"))
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("We should start preparing to phase up into Town Phase, which will unlock many more units and buildings. Select the Civic Center and hover the cursor over the Town Phase icon to see what is still needed.\n"),
			markForTranslation("We now have enough resources, but one structure is missing. Although this is an economic tutorial, it is nonetheless useful to be prepared for defense in case of attack, so let's build Barracks.\n"),
			markForTranslation("Select four of your soldiers and ask them to build a Barracks: as before, start selecting the soldiers, click on the Barracks icon in the production panel and then lay down a foundation not far from your Civic Center where you want to build.")
		],
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "construct" && msg.cmd.template == "structures/athen_barracks")
				this.NextGoal();
		}

	},
	{
		"instructions": [
			markForTranslation("Let's wait for the Barracks to be built. As this construction is lengthy, you can add two soldiers to build it faster. To do so, select your Civic Center and set up a rally point on the Barracks foundation by right-clicking on it (you should see a hammer icon). Then produce two more builders by clicking on the hoplite icon twice.")
		],
		"OnStructureBuilt": function(msg)
		{
			if (TriggerHelper.EntityMatchesClassList(msg.building, "Barracks"))
				this.NextGoal();
		},
	},
	{
		"instructions": [
			markForTranslation("You should now be able to research Town Phase. Select the Civic Center and click on the technology icon.\n"),
			markForTranslation("If you still miss some resources (icon with red overlay), wait for them to be gathered by your workers.")
		],
		"IsDone": function()
		{
			return TriggerHelper.HasDealtWithTech(this.playerID, "phase_town_athen");
		},
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("In later phases, you need usually Stone and Metal to build bigger structures and train better soldiers. Hence, while waiting for the research to be done, you will send half of your idle Citizen Soldiers (who have finished building the Barracks) to gather Stone and the other half to gather Metal.\n"),
			markForTranslation("To do so, we could select three Citizen Soldiers and right-click on the Stone mine on the west of the Civic Center (the cursor changes when hovering the Stone mine while your soldiers are selected). However, these soldiers were gathering Wood, so they may still carry some Wood which would be lost when starting to gather another resource.")
		],
	},
	{
		"instructions": [
			markForTranslation("Thus, we should order them to deposit their Wood in the Civic Center along the way. To do so, we will hold Shift while clicking to queue orders: select your soldiers, hold Shift and right-click on the Civic Center to deposit their Wood and then hold Shift and right-click on the Stone mine to gather it.\n"),
			markForTranslation("Perform a similar order queue with the remaining soldiers and the Metal mine in the west.")
		],
		"Init": function()
		{
			this.stone = false;
			this.metal = false;
		},
		"IsDone": function()
		{
			if (!this.stone || !this.metal)
				return false;
			return TriggerHelper.HasDealtWithTech(this.playerID, "phase_town_athen");

		},
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target)
			{
				if (TriggerHelper.GetResourceType(msg.cmd.target).generic == "stone")
					this.stone = true;
				else if (TriggerHelper.GetResourceType(msg.cmd.target).generic == "metal")
					this.metal = true;
			}
			if (this.IsDone())
				this.NextGoal();
		},
		"OnResearchFinished": function(msg)
		{
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": [
			markForTranslation("This is the end of the walkthrough. This should give you a good idea of the basics of setting up your economy.")
		]
	}
];

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.playerID = 1;
	cmpTrigger.RegisterTrigger("OnInitGame", "InitTutorial", { "enabled": true });
}

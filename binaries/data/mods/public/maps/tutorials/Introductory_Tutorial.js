Trigger.prototype.tutorialGoals = [
	{
		"instructions": markForTranslation("Welcome to the 0 A.D. tutorial."),
	},
	{
		"instructions": markForTranslation("Left-click on a female citizen and then right-click on a berry bush to make that female citizen gather food. Female citizens gather vegetables faster than other units."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
			    TriggerHelper.GetResourceType(msg.cmd.target).specific == "fruit")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the citizen-soldier, right-click on a tree near the Civil Center to begin collecting wood. Citizen-soldiers gather wood faster than female citizens."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
			    TriggerHelper.GetResourceType(msg.cmd.target).specific == "tree")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the Civil Center building, and shift-click on the Hoplite icon (2nd in the row) once to begin training 5 Hoplites."),
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart_infantry_spearman_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
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
		"instructions": markForTranslation("Select the two idle female citizens and build a house nearby by selecting the house icon. Place the house by left-clicking on a piece of land."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "House"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("When they are ready, select the newly trained Hoplites and assign them to build a storehouse beside some nearby trees. They will begin to gather wood when it's constructed."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Storehouse"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a set of 5 skirmishers by shift-clicking on the skirmisher icon (3rd in the row) in the Civil Center."),
		"Init": function()
		{
			this.trainingDone = false;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart_infantry_javelinist_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to shift-click to produce several units.") :
					markForTranslation("Shift-click on the SKIRMISHER icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a farmstead in an open space beside the Civil Center using any idle builders."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Farmstead"))
				this.NextGoal();
		},
		"OnTrainingFinished": function(msg)
		{
			this.trainingDone = true;
		}
	},
	{
		"instructions": markForTranslation("Let's wait for the farmstead to be built."),
		"OnTrainingFinished": function(msg)
		{
			this.trainingDone = true;
		},
		"OnStructureBuilt": function(msg)
		{
			if (TriggerHelper.EntityMatchesClassList(msg.building, "Farmstead"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Once the farmstead is constructed, its builders will automatically begin gathering food if there is any nearby. Select the builders and instead make them construct a field beside the farmstead."),
		"Init": function()
		{
			this.farmStarted = false;
		},
		"IsDone": function()
		{
			return this.farmStarted && this.trainingDone;
		},
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Field"))
				this.farmStarted = true;
			if (this.IsDone())
				this.NextGoal();
		},
		"OnTrainingFinished": function(msg)
		{
			this.trainingDone = true;
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The field's builders will now automatically begin collecting food from the field. Using the newly created group of skirmishers, get them to build another house nearby."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "House"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Train 5 Hoplites from the Civil Center. Select the Civil Center and with it selected right click on a tree nearby. Units from the Civil Center will now automatically gather wood."),
		"Init": function()
		{
			this.rallyPointSet = false;
			this.trainingStarted = false;
		},
		"IsDone": function()
		{
			return this.rallyPointSet && this.trainingStarted;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart_infantry_spearman_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to shift-click to produce several units.") :
					markForTranslation("Shift-click on the HOPLITE icon.");
				this.WarningMessage(txt);
				return;
			}
			this.trainingStarted = true;
			if (this.IsDone())
				this.NextGoal();
		},
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data ||
			   !msg.cmd.data.command || msg.cmd.data.command != "gather" ||
			   !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "tree")
			{
				this.WarningMessage(markForTranslation("Select the Civic Center, then hover your mouse over the tree and right-click when you see your cursor change into a Wood icon."));
				return;
			}
			this.rallyPointSet = true;
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Order the idle Skirmishers to build an outpost to the north east at the edge of your territory.  This will be the fifth Village Phase structure that you have built, allowing you to advance to the Town Phase."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Outpost"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the Civil Center again and advance to Town Phase by clicking on the 'II' icon (you have to wait for the outpost to be built first). This will allow Town Phase buildings to be constructed."),
		"IsDone": function()
		{
			return TriggerHelper.HasDealtWithTech(this.playerID, "phase_town_generic");
		},
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("While waiting for the phasing up, you may reaffect your idle workers to gathering the resources you are short of."),
		"IsDone": function()
		{
			let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
			let playerEnt = cmpPlayerManager.GetPlayerByID(this.playerID);
			let cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
			return cmpTechnologyManager && cmpTechnologyManager.IsTechnologyResearched("phase_town_generic");
		},
		"OnResearchFinished": function(msg)
		{
			if (msg.tech == "phase_town_generic")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Start building 5 female citizens in the Civil Center and set its rally point to the farm (right click on it)."),
		"Init": function()
		{
			this.rallyPointSet = false;
			this.trainingStarted = false;
		},
		"IsDone": function()
		{
			return this.rallyPointSet && this.trainingStarted;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart_support_female_citizen" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to shift-click to produce several units.") :
					markForTranslation("Hold shift and click on the female citizen icon.");
				this.WarningMessage(txt);
				return;
			}
			this.trainingStarted = true;
			if (this.IsDone())
				this.NextGoal();
		},
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "set-rallypoint" || !msg.cmd.data ||
			   !msg.cmd.data.command || msg.cmd.data.command != "gather" ||
			   !msg.cmd.data.resourceType || msg.cmd.data.resourceType.specific != "grain")
				return;
			this.rallyPointSet = true;
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a Barracks nearby. Whenever your population limit is reached, build an extra house using any available builder units."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Barracks"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Prepare for an attack by an enemy player. Build more soldiers using the Barracks, and get idle soldiers to build a Defense Tower near your Outpost."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "DefenseTower"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a Blacksmith and research the Infantry Training technology (sword icon) to improve infantry hack attack."),
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "Blacksmith"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The enemy is coming. Build more soldiers to fight off the enemies."),
		"OnResearchFinished": function(msg)
		{
			this.LaunchAttack();
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Try to repel the attack."),
		"OnOwnershipChanged": function(msg)
		{
			if (msg.to != -1)
				return;
			if (this.IsAttackRepelled())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The enemy's attack has been defeated. Now build a market and a temple while assigning new units to gather any required resources."),
		"Init": function()
		{
			this.marketStarted = false;
			this.templeStarted = false;
		},
		"IsDone": function()
		{
			return this.marketStarted && this.templeStarted;
		},
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type != "repair")
				return;
			this.marketStarted = this.marketStarted || TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Market");
			this.templeStarted = this.templeStarted || TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Temple");
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("When that City Phase requirements have been reached, select your Civil Center and advance to City Phase."),
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("While waiting for the phase change, you may build more soldiers at the barracks."),
		"OnResearchFinished": function(msg)
		{
			if (msg.tech == "phase_city_generic")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Now that you are in City Phase, build a fortress nearby (gather some stone first if needed) and then use it to build 2 Battering Rams."),
		"Init": function()
		{
			this.ramCount = 0;
		},
		"IsDone": function()
		{
			return this.ramCount > 1;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate == "units/spart_mechanical_siege_ram")
				++this.ramCount;
			if (this.IsDone())
			{
				this.RemoveChampions();
				this.NextGoal();
			}
		}
	},
	{
		"instructions": [
			markForTranslation("Stop all your soldiers gathering resources and instead task small groups to find the enemy Civil Center on the map. Once The enemy's base has been spotted, send your siege weapons and all remaining soldiers to destroy it.\n"),
			markForTranslation("Female citizens should continue to gather resources.")
		],
		"OnOwnershipChanged": function(msg)
		{
			if (msg.from != this.enemyID)
				return;
			if (TriggerHelper.EntityMatchesClassList(msg.entity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The enemy has been defeated. These tutorial tasks are now completed."),
	}
];

Trigger.prototype.LaunchAttack = function()
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let entities = cmpRangeManager.GetEntitiesByPlayer(this.playerID);
	let target = entities.find(e => Engine.QueryInterface(e, IID_Identity) && Engine.QueryInterface(e, IID_Identity).HasClass("DefenseTower")) ||
	             entities.find(e => Engine.QueryInterface(e, IID_Identity) && Engine.QueryInterface(e, IID_Identity).HasClass("CivilCentre"));
	let position = Engine.QueryInterface(target, IID_Position).GetPosition2D();

	this.attackers = cmpRangeManager.GetEntitiesByPlayer(this.enemyID).filter(e =>
			Engine.QueryInterface(e, IID_Identity) && Engine.QueryInterface(e, IID_UnitAI) &&
			Engine.QueryInterface(e, IID_Identity).HasClass("CitizenSoldier")
		);
	this.attackers.forEach(e => { Engine.QueryInterface(e, IID_UnitAI).WalkAndFight(position.x, position.y, { "attack": ["Unit"] }, false); });
};

Trigger.prototype.IsAttackRepelled = function()
{
	return !this.attackers.some(e => Engine.QueryInterface(e, IID_Health) && Engine.QueryInterface(e, IID_Health).GetHitpoints() > 0);
};

Trigger.prototype.RemoveChampions = function()
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let champions = cmpRangeManager.GetEntitiesByPlayer(this.enemyID).filter(e => Engine.QueryInterface(e, IID_Identity).HasClass("Champion"));
	let keep = 6;
	for (let ent of champions)
	{
		let cmpHealth = Engine.QueryInterface(ent, IID_Health);
		if (!cmpHealth)
			Engine.DestroyEntity(ent);
		else if (--keep < 0)
			cmpHealth.Kill();
	}
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.playerID = 1;
	cmpTrigger.enemyID = 2;
	cmpTrigger.RegisterTrigger("OnInitGame", "InitTutorial", { "enabled": true });
}

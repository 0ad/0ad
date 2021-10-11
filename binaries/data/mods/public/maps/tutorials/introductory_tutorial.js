Trigger.prototype.tutorialGoals = [
	{
		"instructions": markForTranslation("Welcome to the 0 A.D. tutorial."),
	},
	{
		"instructions": markForTranslation("Left-click on a Female Citizen and then right-click on a berry bush to make that Female Citizen gather food. Female Citizens gather vegetables faster than other units."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
			    TriggerHelper.GetResourceType(msg.cmd.target).specific == "fruit")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the Citizen Soldier, right-click on a tree near the Civic Center to begin gathering wood. Citizen Soldiers gather wood faster than Female Citizens."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "gather" && msg.cmd.target &&
			    TriggerHelper.GetResourceType(msg.cmd.target).specific == "tree")
				this.NextGoal();
		}
	},
	{
		"instructions": [
			{
				"text": markForTranslation("Select the Civic Center building and hold %(hotkey)s while clicking on the Hoplite icon once to begin training a batch of Hoplites."),
				"hotkey": "session.batchtrain"
			}
		],
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart/infantry_spearman_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to press the batch training hotkey while clicking to produce multiple units.") :
					markForTranslation("Click on the Hoplite icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the two idle Female Citizens and build a House nearby by selecting the House icon. Place the House by left-clicking on a piece of land."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "House"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("When they are ready, select the newly trained Hoplites and assign them to build a Storehouse beside some nearby trees. They will begin to gather wood when it's constructed."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Storehouse"))
				this.NextGoal();
		}
	},
	{
		"instructions": [
			{
				"text": markForTranslation("Train a batch of Skirmishers by holding %(hotkey)s and clicking on the Skirmisher icon in the Civic Center."),
				"hotkey": "session.batchtrain"
			}
		],
		"Init": function()
		{
			this.trainingDone = false;
		},
		"OnTrainingQueued": function(msg)
		{
			if (msg.unitTemplate != "units/spart/infantry_javelineer_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to press the batch training hotkey while clicking to produce multiple units.") :
					markForTranslation("Click on the Skirmisher icon.");
				this.WarningMessage(txt);
				return;
			}
			this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a Farmstead in an open space beside the Civic Center using any idle builders."),
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
		"instructions": markForTranslation("Let's wait for the Farmstead to be built."),
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
		"instructions": markForTranslation("Once the Farmstead is constructed, its builders will automatically begin gathering food if there is any nearby. Select the builders and instead make them construct a Field beside the Farmstead."),
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
		"instructions": markForTranslation("The Field's builders will now automatically begin gathering food from the Field. Using the newly created group of skirmishers, get them to build another House nearby."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "House"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Train a batch of Hoplites at the Civic Center. Select the Civic Center and with it selected right-click on a tree nearby. Units from the Civic Center will now automatically gather wood."),
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
			if (msg.unitTemplate != "units/spart/infantry_spearman_b" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to press the batch training hotkey while clicking to produce multiple units.") :
					markForTranslation("Click on the Hoplite icon.");
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
				this.WarningMessage(markForTranslation("Select the Civic Center, then hover the cursor over the tree and right-click when you see your cursor change into a wood icon."));
				return;
			}
			this.rallyPointSet = true;
			if (this.IsDone())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a Barracks nearby. Whenever your population limit is reached, build an extra House using any available builder units. This will be the fifth Village Phase structure that you have built, allowing you to advance to the Town Phase."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Barracks"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Select the Civic Center again and advance to Town Phase by clicking on the II icon (you have to wait for the barracks to be built first). This will allow Town Phase buildings to be constructed."),
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
		"instructions": markForTranslation("While waiting for the phasing up, you may reassign your idle workers to gathering the resources you are short of."),
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
		"instructions": markForTranslation("Order the idle Skirmishers to build an outpost to the north east at the edge of your territory."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Outpost"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Start training a batch of Female Citizens in the Civic Center and set its rally point to the Field (right click on it)."),
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
			if (msg.unitTemplate != "units/spart/support_female_citizen" || +msg.count == 1)
			{
				let cmpProductionQueue = Engine.QueryInterface(msg.trainerEntity, IID_ProductionQueue);
				cmpProductionQueue.ResetQueue();
				let txt = +msg.count == 1 ?
					markForTranslation("Do not forget to press the batch training hotkey while clicking to produce multiple units.") :
					markForTranslation("Click on the Female Citizen icon.");
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
		"instructions": markForTranslation("Prepare for an attack by an enemy player. Train more soldiers using the Barracks, and get idle soldiers to build a Tower near your Outpost."),
		"OnPlayerCommand": function(msg)
		{
			if (msg.cmd.type == "repair" && TriggerHelper.EntityMatchesClassList(msg.cmd.target, "Tower"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Build a Forge and research the Infantry Training technology (sword icon) to improve infantry hack attack."),
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "Forge"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The enemy is coming. Train more soldiers to fight off the enemies."),
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
			if (msg.to != INVALID_PLAYER)
				return;
			if (this.IsAttackRepelled())
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("The enemy attack has been thwarted. Now build a market and a temple while you assign new units to gather required resources."),
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
		"instructions": markForTranslation("Once you meet the City Phase requirements, select your Civic Center and advance to City Phase."),
		"IsDone": function()
		{
			return TriggerHelper.HasDealtWithTech(this.playerID, "phase_city_generic");
		},
		"OnResearchQueued": function(msg)
		{
			if (msg.technologyTemplate && TriggerHelper.EntityMatchesClassList(msg.researcherEntity, "CivilCentre"))
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("While waiting for the phase change, you may train more soldiers at the Barracks."),
		"IsDone": function()
		{
			return TriggerHelper.HasDealtWithTech(this.playerID, "phase_city_generic");
		},
		"OnResearchFinished": function(msg)
		{
			if (msg.tech == "phase_city_generic")
				this.NextGoal();
		}
	},
	{
		"instructions": markForTranslation("Now that you are in City Phase, build the Arsenal nearby and then use it to construct 2 Battering Rams."),
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
			if (msg.unitTemplate == "units/spart/siege_ram")
				this.ramCount += msg.count;
			if (this.IsDone())
			{
				this.RemoveChampions();
				this.NextGoal();
			}
		}
	},
	{
		"instructions": [
			markForTranslation("Stop all your soldiers gathering resources and instead task small groups to find the enemy Civic Center on the map. Once the enemy's base has been spotted, send your Siege Engines and all remaining soldiers to destroy it.\n"),
			markForTranslation("Female Citizens should continue to gather resources.")
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
		"Init": function()
		{
			let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
			cmpEndGameManager.MarkPlayerAndAlliesAsWon(1, () => "Tutorial completed", () => "");
		}
	}
];

Trigger.prototype.LaunchAttack = function()
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let entities = cmpRangeManager.GetEntitiesByPlayer(this.playerID);
	let target =
		entities.find(e => {
			let cmpIdentity = Engine.QueryInterface(e, IID_Identity);
			return cmpIdentity && cmpIdentity.HasClass("Tower") && Engine.QueryInterface(e, IID_Position);
		}) ||
		entities.find(e => {
			let cmpIdentity = Engine.QueryInterface(e, IID_Identity);
			return cmpIdentity && cmpIdentity.HasClass("CivilCentre") && Engine.QueryInterface(e, IID_Position);
		});

	let position = Engine.QueryInterface(target, IID_Position).GetPosition2D();

	this.attackers = cmpRangeManager.GetEntitiesByPlayer(this.enemyID).filter(e => {
		let cmpIdentity = Engine.QueryInterface(e, IID_Identity);
		return Engine.QueryInterface(e, IID_UnitAI) && cmpIdentity && cmpIdentity.HasClass("CitizenSoldier");
	});

	ProcessCommand(this.enemyID, {
		"type": "attack-walk",
		"entities": this.attackers,
		"x": position.x,
		"z": position.y,
		"targetClasses": { "attack": ["Unit"] },
		"allowCapture": false,
		"queued": false
	});
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

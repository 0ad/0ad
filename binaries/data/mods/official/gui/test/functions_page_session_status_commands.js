/*
	DESCRIPTION	: Functions for the Command Buttons orbitting the Status Pane.
	NOTES		: 
*/

// ====================================================================

function defineCommandButtons(command)
{
	snStatusPaneCommand = new Array();
	snStatusPaneCommand.tab = new Object();
	snStatusPaneCommand.list = new Object();
	snStatusPaneCommand.button = new Object();

	// Maximum number of buttons (either single or lists).
	snStatusPaneCommand.tab.max = command.substring (command.lastIndexOf ("d")+1, command.lastIndexOf ("_")); // 10
	// Maximum number of entries in a list.
	snStatusPaneCommand.list.max = command.substring (command.lastIndexOf ("_")+1, command.length); // 12

	// Number of tabs that are single buttons (no list).
	snStatusPaneCommand.button.max = 5;		
	// When we reach this button, split the rows (remainder are vertical, not horizontal).
	snStatusPaneCommand.split = 7;			
	// Spacing between lists.
	snStatusPaneCommand.span = 2;	

	// Get the coordinates of the Status Pane background (referenced to determine command button locations).
	currCrd = getCrd ("snStatusPaneBkg");	

	// Update each tab.
	for (var tabLoop = 1; tabLoop <= snStatusPaneCommand.tab.max; tabLoop++)
	{
		snStatusPaneCommand[tabLoop]	    = new Array();

		tempGroupObject = getGUIObjectByName("snStatusPaneCommand" +
			"Group" + tabLoop);

		// Update each list under each tab.
		for (var listLoop = 1; listLoop <= snStatusPaneCommand.list.max; listLoop++)
		{
			tempListObject = getGUIObjectByName("snStatusPaneCommand" +
				tabLoop + "_" + listLoop);

			// Define properties of list buttons.
			tempListObject.style = "snAction";

			// Set portrait to default.
			setPortrait (tempListObject.name, "default_square");

			// Create parameters where command button information will later be stored.
			snStatusPaneCommand[tabLoop][listLoop]	    = new Array();
			snStatusPaneCommand[tabLoop][listLoop].type = new Object();
			snStatusPaneCommand[tabLoop][listLoop].name = new Object();
			snStatusPaneCommand[tabLoop][listLoop].last = new Object();

			// Determine x and y position for current button.
			if (tabLoop >= snStatusPaneCommand.split)
			{
				if (listLoop == 1)
				{
					var x = currCrd.coord[rb].x+currCrd.coord[rb].width;
	
					if (tabLoop == snStatusPaneCommand.split && listLoop == 1)
						var y = currCrd.coord[rb].y; 
					else
						var y = Crd[Crd.last].coord[rb].y+Crd[Crd.last].coord[rb].height
							+snStatusPaneCommand.span;
				}
				else
				{
					var x = Crd[Crd.last].coord[rb].x+Crd[Crd.last].coord[rb].width
							+snStatusPaneCommand.span;
					var y = Crd[Crd.last].coord[rb].y;
				}
			}
			else
			{
				if (listLoop == 1)
				{
					if (tabLoop == 1 && listLoop == 1)
						var x = currCrd.coord[rb].x; 
					else
						var x = Crd[Crd.last].coord[rb].x+Crd[Crd.last].coord[rb].width
							+snStatusPaneCommand.span;;

					var y = currCrd.coord[rb].y+currCrd.coord[rb].height; 
				}
				else
				{
					var x = Crd[Crd.last].coord[rb].x;
					var y = Crd[Crd.last].coord[rb].y+Crd[Crd.last].coord[rb].height
						+snStatusPaneCommand.span;
				}
			}

			// Define dimensions of list buttons.
			addCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, rb, 0, 100, x, y,
				snConst.Portrait.Sml.Width, snConst.Portrait.Sml.Height);
			addCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, lb, 100, 100);	
			addCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, lt, 100, 0);
			addCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, rt, 0, 0);

			// Press button.
			tempListObject.onPress =
				function (m, n, o)
				{
					return function()
					{
						pressCommandButton(m, n, o); 
					}
				} (tempListObject, tabLoop, listLoop);
		}
	}
}

// ====================================================================

function UpdateList(listIcon, listCol)
{
	// Populates a given column of command icons with appropriate build portraits for the selected object.
	// Returns an array of this selection.

	// Build unit list.
	if ( selection[0].traits.id.civ_code
	&& selection[0].actions.create
	&& selection[0].actions.create.list )
	{
		listName = "";

		switch (listIcon)
		{
			case action_tab_train:
				if ( selection[0].actions.create.list.unit && shouldUpdateStat( "actions.create.list.unit" ) )
					listName = selection[0].actions.create.list.unit.toString();
			break;
			case action_tab_buildciv:
				if ( selection[0].actions.create.list.structciv && shouldUpdateStat( "actions.create.list.structciv" ) )
					listName = selection[0].actions.create.list.structciv.toString();
			break;
			case action_tab_buildmil:
				if ( selection[0].actions.create.list.structmil && shouldUpdateStat( "actions.create.list.structmil" ) )
					listName = selection[0].actions.create.list.structmil.toString();
			break;
			case action_tab_research:
				if ( selection[0].actions.create.list.tech && shouldUpdateStat( "actions.create.list.tech" ) )
					listName = selection[0].actions.create.list.tech.toString();
			break;
			default:
				return 0;
			break;
		}

		if (listName != "")
		{
			// Enable tab portrait.
			setPortrait("snStatusPaneCommand" + listCol + "_1", "sheet_action", "", listIcon);
			guiUnHide("snStatusPaneCommand" + listCol + "_1");

			// Store content info in tab button for future reference.
			snStatusPaneCommand[listCol][1].type = "list";

			// Extract entity list into an array.
			listArray = parseDelimiterString(listName, ";");

			// Reset list length.
			snStatusPaneCommand[listCol][1].last = 0;

			// Populate appropriate command buttons.
			for (createLoop = 1; createLoop < snStatusPaneCommand.list.max; createLoop++)
			{
				if (createLoop < listArray.length)
				{
					// Get name of entity to display in list.
					UpdateListEntityName = selection[0].traits.id.civ_code + "_" + listArray[createLoop];

					setPortrait("snStatusPaneCommand" + listCol + "_" + parseInt(createLoop+1), getEntityTemplate(UpdateListEntityName).traits.id.icon, selection[0].traits.id.civ_code, getEntityTemplate(UpdateListEntityName).traits.id.icon_cell);
					getGUIObjectByName("snStatusPaneCommand" + listCol + "_" + parseInt(createLoop+1)).caption = "";
					guiUnHide("snStatusPaneCommand" + listCol + "_" + parseInt(createLoop+1));
					
					// Store content info in tab button for future reference.
					snStatusPaneCommand[parseInt(createLoop+1)][listCol].name = listArray[createLoop];
					snStatusPaneCommand[parseInt(createLoop+1)][listCol].last++;		
				}
				else
					guiHide("snStatusPaneCommand" + listCol + "_" + parseInt(createLoop+1));
			}
			return listArray;
		}
	}

	return 0;
}

// ====================================================================

function UpdateCommand(listIcon, listCol)
{
	// Similar to UpdateList, but without the list.
	// Updates a particular command button with a particular action.

	if (
            (listIcon == action_attack && selection[0].actions.attack)
         || (listIcon == action_patrol && selection[0].actions.patrol)
         || (listIcon == action_repair && selection[0].actions.repair)
         || (listIcon == action_gather_food && selection[0].actions.gather && selection[0].actions.gather.food)
         || (listIcon == action_gather_wood && selection[0].actions.gather && selection[0].actions.gather.wood)
         || (listIcon == action_gather_stone && selection[0].actions.gather && selection[0].actions.gather.stone)
         || (listIcon == action_gather_ore && selection[0].actions.gather && selection[0].actions.gather.ore)
           )
	{	
		// Set appearance of tab.
		setPortrait("snStatusPaneCommand" + listCol + "_1", "sheet_action", "", listIcon);
		guiUnHide("snStatusPaneCommand" + listCol + "_1");

		// Hide its list.
		guiHide("snStatusPaneCommand" + "Group" + listCol);

		// Store content info in tab button for future reference.
		snStatusPaneCommand[listCol][1].type = "command";
		snStatusPaneCommand[listCol][1].last = 0;
		snStatusPaneCommand[listCol][1].name = listIcon;

		return (listCol-1);
	}
	else
		return (listCol);
}

// ====================================================================

function pressCommandButton(commandButton, tab, list)
{
console.write ("Button pressed. " + commandButton + " " + tab + " " + list);
console.write (snStatusPaneCommand[tab][list].type);
	switch (list)
	{
		case 1:
			commandButton.caption = "";
			if (snStatusPaneCommand[tab][list].type == "list")
			{
				// Click the tab button to toggle visibility of its list (if it's of a list type).
				guiToggle ("snStatusPaneCommand" + "Group" + tab);
console.write ("Toggled " + "snStatusPaneCommand" + "Group" + tab);
			}
			else
			{
console.write ("Some weird action.");
				// Perform appropriate actions for different command buttons.

				switch (snStatusPaneCommand[tab][list].name)
				{
					case action_patrol:
						setCursor ("action-patrol");
						selectLocation(
							function (x, y) {
								issueCommand (selection, NMT_Patrol, x, y);
							}
						);
					break;
					case action_attack:
						setCursor ("action-attack");
						selectEntity(
							function (target) {
								issueCommand (selection, NMT_AttackMelee, target);
							}
						);
					break;
				}
			}
		break;
		default:
			// Left-clicked list button.
			console.write("Clicked [" + tab + "," + list + "]: list of type " + snStatusPaneCommand[tab][list].type + "; " + snStatusPaneCommand[tab][list].name);

			switch (snStatusPaneCommand[tab][list].name)
			{
				case action_tab_buildciv:
				case action_tab_buildmil:
					// Create building placement cursor.
					startPlacing(selection[0].traits.id.civ_code + "_" + snStatusPaneCommand[tab][list].name);
				break;
				default:
					// Attempt to add the entry to the queue.
					attempt_add_to_build_queue( selection[0], selection[0].traits.id.civ_code + "_" + snStatusPaneCommand[tab][list].name, tab, list);
				break;
			}
		break;
	}
}

// ====================================================================

function refreshCommandButtons()
{
	if( shouldUpdateStat( "actions.create.list" ) )
	{
		// Everything in this block is tied to properties in
		// actions.create.list, the above check should limit the
		// number of times this update is needlessly made.
	
		// Update train/research/build lists.
		listCounter	= 1; 
		unitArray 	= UpdateList(action_tab_train, listCounter); 		if (unitArray != 0)	 listCounter++;
		structcivArray 	= UpdateList(action_tab_buildciv, listCounter);		if (structcivArray != 0) listCounter++;
		structmilArray 	= UpdateList(action_tab_buildmil, listCounter);		if (structmilArray != 0) listCounter++;
		techArray 	= UpdateList(action_tab_research, listCounter);		if (techArray != 0)	 listCounter++;
		formationArray 	= UpdateList(action_tab_formation, listCounter);	if (formationArray != 0) listCounter++;
		stanceArray 	= UpdateList(action_tab_stance, listCounter);		if (stanceArray != 0)	 listCounter++;
	}
	
	if( shouldUpdateStat( "actions" ) )
	{
		// Update commands.
		commandCounter = snStatusPaneCommand.tab.max;
		commandCounter = UpdateCommand(action_attack, commandCounter);
		commandCounter = UpdateCommand(action_patrol, commandCounter);
		commandCounter = UpdateCommand(action_repair, commandCounter);
		commandCounter = UpdateCommand(action_gather_food, commandCounter);
		commandCounter = UpdateCommand(action_gather_wood, commandCounter);
		commandCounter = UpdateCommand(action_gather_stone, commandCounter);
		commandCounter = UpdateCommand(action_gather_ore, commandCounter);
	}

	if (listCounter > 0 && commandCounter > 0)
	{
		// Clear remaining buttons between them.
		for (commandClearLoop = listCounter; commandClearLoop <= commandCounter; commandClearLoop++)
		{
			guiHide("snStatusPaneCommand" + commandClearLoop + "_1");
			// If this slot could possibly contain a list, hide that too.
			guiHide("snStatusPaneCommand" + "Group" + commandClearLoop);
		}
	}

/*
	// Update production queue.
	GUIObject = getGUIObjectByName("snStatusPaneCommandProgress");
	// If the entity has a production item underway,
	if ( shouldUpdateStat( "actions.create" ) && shouldUpdateStat( "actions.create.progress" )
	  && shouldUpdateStat( "actions.create.progress.valueOf()" )
          && shouldUpdateStat( "actions.create.progress.valueOf().current" )
          && shouldUpdateStat( "actions.create.queue.valueOf()" )
          && shouldUpdateStat( "actions.create.queue.valueOf()[0].traits.creation.time" ) )
	{
		// Set the value of the production progress bar.
		GUIObject.caption = ((Math.round(Math.round(selection[0].actions.create.progress.valueOf().current)) * 100 ) / Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time));
		// Set position of progress bar.
		GUIObject.size = getGUIObjectByName("snStatusPaneCommand" + selection[0].actions.create.queue.valueOf()[0].tab + "_" + selection[0].actions.create.queue.valueOf()[0].list).size;
		// Set progress bar tooltip.
		GUIObject.tooltip = "Training " + selection[0].actions.create.queue.valueOf()[0].traits.id.generic + " ... " + (Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time-Math.round(selection[0].actions.create.progress.valueOf().current)) + " seconds remaining.";
		// Reveal progressbar.
		GUIObject.hidden  = false;
		
		// Seek through queue.
		for( queueEntry = 0; queueEntry < selection[0].actions.create.queue.valueOf().length; queueEntry++)
		{
			// Update list buttons so that they match the number of entries of that type in the queue.
			getGUIObjectByName("snStatusPaneCommand" + selection[0].actions.create.queue.valueOf()[queueEntry].tab + "_" + selection[0].actions.create.queue.valueOf()[queueEntry].list).caption++;
		}
	}
	else
	{
		// Hide the progress bar.
		GUIObject.hidden  = true;
		GUIObject.tooltip = "";
	}
*/
}

// ====================================================================

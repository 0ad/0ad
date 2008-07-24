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
	snStatusPaneCommand.tab.max = command.substring (command.lastIndexOf ("d")+1, command.lastIndexOf ("_")); // 8
	// Maximum number of entries in a list.
	snStatusPaneCommand.list.max = command.substring (command.lastIndexOf ("_")+1, command.length); // 12

	// When we reach this button, split the rows (remainder are vertical, not horizontal).
	snStatusPaneCommand.split = 5;			
	// Spacing between lists.
	snStatusPaneCommand.span = 1;	

	// Get the coordinates of the Status Pane background (referenced to determine command button locations).
	var currCrd = getCrd ("snStatusPaneBkg");	

	// Update each tab.
	for (var tabLoop = 1; tabLoop <= snStatusPaneCommand.tab.max; tabLoop++)
	{
		var tempGroupObject = getGUIObjectByName("snStatusPaneCommand" +
			"Group" + tabLoop);
		
		// Update each list under each tab.
		for (var listLoop = 1; listLoop <= snStatusPaneCommand.list.max; listLoop++)
		{
			var tempListObject = getGUIObjectByName("snStatusPaneCommand" +
				tabLoop + "_" + listLoop);
				
			// Set default portrait.
			setPortrait (tempListObject.name, "IconPortrait");
			
			// Width and height of buttons is always the same.
			var buttonWidth = snConst.Portrait.Sml.Width;
			var buttonHeight = snConst.Portrait.Sml.Height;

			// If we're doing the arc of commands.
			if (tabLoop >= snStatusPaneCommand.split)
			{
				if (listLoop == 1)
				{
					if (tabLoop > snStatusPaneCommand.split)
					{
						// Get the first tab.
						var firstTab = getCrd ("snStatusPaneCommand" + (snStatusPaneCommand.split) + "_" + listLoop);
						// Get the previous tab.
						var lastTab = getCrd ("snStatusPaneCommand" + (tabLoop-1) + "_" + listLoop);				
					}
				
					// Set position of tab (it curves, so we need to specifically set each button position).					
					switch (tabLoop)
					{
						case (snStatusPaneCommand.split):
							var buttonX = currCrd.coord[rb].x + currCrd.coord[rb].width - buttonWidth + 2;
							var buttonY = currCrd.coord[rb].y + currCrd.coord[rb].height - buttonHeight - 3.3;
						break;
						case (snStatusPaneCommand.split+1):
							var buttonX = lastTab.coord[rb].x + (lastTab.coord[rb].width/1.7);
							var buttonY = lastTab.coord[rb].y - (lastTab.coord[rb].height/1.3);
						break;						
						case (snStatusPaneCommand.split+2):
							var buttonX = firstTab.coord[rb].x - (buttonWidth / 1.5);
							var buttonY = firstTab.coord[rb].y + (buttonHeight / 1.5);
						break;						
						default:
							var buttonX = getCrd ("snStatusPaneCommand" + (snStatusPaneCommand.split+1) + "_" + listLoop).coord[rb].x;
							var buttonY = currCrd.coord[rb].y + 3;						
						break;
					}
				
					// Set default portrait.
					setPortrait (tempListObject.name, "IconCommand");										
				}
				else
				{
					var parentTab = getCrd ("snStatusPaneCommand" + (tabLoop) + "_" + (listLoop - 1));
					// Set position of buttons under tab (parallel row to the right of it).
					var buttonX = parentTab.coord[rb].x+parentTab.coord[rb].width
							+snStatusPaneCommand.span;
					var buttonY = parentTab.coord[rb].y;
				}
				var barX = buttonX;
				var barY = 0;				
			}
			else	// If we're doing the row of tabs,
			{
				// Set position of tab.
				if (listLoop == 1)
				{
					if (tabLoop == 1 && listLoop == 1)
						var buttonX = currCrd.coord[rb].x; 
					else
						var buttonX = Crd[Crd.last].coord[rb].x+Crd[Crd.last].coord[rb].width
							+snStatusPaneCommand.span;

					var buttonY = currCrd.coord[rb].y+currCrd.coord[rb].height-7; 
				}
				else	// Set position of buttons under tab.
				{
					var buttonX = Crd[Crd.last].coord[rb].x;
					var buttonY = Crd[Crd.last].coord[rb].y+Crd[Crd.last].coord[rb].height
						+snStatusPaneCommand.span;
				}
			}

			// Define dimensions of list buttons.
			addCrds ("snStatusPaneCommand" + tabLoop + "_" + listLoop, 0, 100, buttonX, buttonY,
				buttonWidth, buttonHeight);
			
			// If we're defining the last button in the list, and it's not a tab,
			if (tabLoop == snStatusPaneCommand.tab.max && listLoop != 1)
			{
				// It has health bars (for the current selection). Set them up too.
				addCrds ("snStatusPaneCommand" + tabLoop + "_" + listLoop + "Bar", 0, 100, barX, barY, 
					buttonWidth, 4);
				getGUIObjectByName ("snStatusPaneCommand" +	tabLoop + "_" + listLoop + "Bar").hidden = false;
				getGUIObjectByName ("snStatusPaneCommand" +	tabLoop + "_" + listLoop + "Bar").caption = 100;
			}
/*				
			// Store indexes to the button for easy future reference.
			Crd[getCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, true)].tab = tabLoop;
			Crd[getCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, true)].list = listLoop;
			// Create quantity and entity for create lists for future use.
			Crd[getCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, true)].entity = "";
			Crd[getCrd ("snStatusPaneCommand" +	tabLoop + "_" + listLoop, true)].quantity = 0;			
*/			
		}
	}
}

// ====================================================================

function updateTab (tab, type, cellSheet, attribute, attribute2, arrayCells)
{
	// Refresh the next tab (and any buttons in its list, if appropriate), according to tabCounter.
	// tab: 		The name of the tab (eg research).
	// type:		The type of button:
	//			* production (Tab button, persistent list of buttons, click them to do things.)
	//			* pick (Tab button holds current selection, click a button from the list to select a new option and close the tab.)
	//			* command (Click "tab" button to do something; no list.)
	// cellSheet:	* The cell sheet (icon group) containing this item's tab icon.
	//				* production: Tends to be "Tab" (since they need a special tab icon), so defaults to this.
	//				* pick: Usually a group that's the same as the tab name, so use this.
	//				* command: Tends to be "Command" (though not always), so defaults to this.
	//			* If no value is specified, it will use the above default for the type.
	// attribute: 	* For production & pick: the attribute containing the list of items to display (eg selection[0].actions.create.list.research), which this entity must
	//			   have in order to update the tab with this list.
	//			* For command: the attribute that the entity must have in order to have this command.
	//			* If no value is specified, all entities have this attribute.
	// attribute2:	* For pick: The variable used to store the current item in the pick that has been selected -- placed in the tab (eg selection[0].actions.formation.curr)
	// arrayCells:	* Optional attribute. If true, assume that items cannot all be found in the same cellSheet; check in attribute.sheet for cell sheet name for each item.

	// Store string form of attribute for future reference.
	attributeString = attribute;
	attribute2String = attribute2;
	
	// If either attribute is not a valid property, return false.
	if ( (validProperty (attributeString)) && (validProperty (attribute2String)) )
	{
		// Use the value of the attribute from this point forwards.
		attribute = eval (attribute);
		attribute2 = eval (attribute2);
		// String properties taken from entities can be a little confused about their variable type, so make sure they're considered strings.
		attribute2 = String (attribute2);
		
//console.write ("1st: " + tabCounter + " " + tab + " " + type + " " + cellSheet + " " + attribute + " " + attribute2);
		
		// Set default values.
		if (cellSheet == "")
		{
			switch (type)
			{
				case "command":
					cellSheet = "Command";
				break;
				case "pick":
					cellSheet = toTitleCase(tab);
					tab		  = attribute2.toLowerCase();
					// avoid, pick, Stance, .traits.ai.stance.list
				break;
				default:
					cellSheet = "Tab";
				break;
			}			
		}
		
//console.write ("2nd: " + tabCounter + " " + tab + " " + type + " " + cellSheet + " " + attribute + " " + attribute2);

		// Get tab.
		var tabObject 	= getGUIObjectByName ("snStatusPaneCommand" + tabCounter + "_1");		
		// Enable tab.
		guiUnHide (tabObject.name);
		
		// Set tab portrait.
		switch (tab)
		{
			case "selection":
			case "garrison":
				// Temporarily (until we have the tab textures) force a particular cell ID for selection and garrison so we can see the icon.
				cellGroup[cellSheet][tab].id = 2;
			
				// Use the horizontal tab. (Extends right.)
				setPortrait ("snStatusPaneCommand" + tabCounter + "_1", "IconSheet", cellSheet + "Button_H", cellGroup[cellSheet][tab].id);		
			break;
			default:
				if (type == "pick" && cellSheet != "Tab")	// Force the tab background for pick lists,
					setPortrait ("snStatusPaneCommand" + tabCounter + "_1", "IconSheet", cellSheet + "TabButton", cellGroup[cellSheet][tab].id);			
				else
					setPortrait ("snStatusPaneCommand" + tabCounter + "_1", "IconSheet", cellSheet + "Button", cellGroup[cellSheet][tab].id);
			break;
		}
		
//console.write ("3rd: " + "snStatusPaneCommand" + tabCounter + "_1" + "|" + "IconSheet" + "|" + cellSheet + "Button" + "|" + cellGroup[cellSheet][tab].id);
			
		switch (type)
		{
			case "command":
				// Set tab tooltip.
				var tooltip = cellGroup[cellSheet][tab].name;
				tooltip += " " + cellSheet;
			break;
			case "pick":
				// Set tab tooltip.
				var tooltip = cellSheet;				
				tooltip += " List\nCurrent " + cellSheet + ": " + cellGroup[cellSheet][tab].name;
			break;
			default:
				// Set tab tooltip.
				var tooltip = cellGroup[cellSheet][tab].name;				
				tooltip += " " + cellSheet;
			break;
		}
		tabObject.tooltip = tooltip;		
		
		switch (type)
		{
			case "command":
				// An array of actions for the command "tabs" can be triggered by clicking the individual commands. We'll set them here.
				switch(tab)
				{
				case "rally":
					tabObject.onPress = function (event)
					{
						setCursor("cursor-rally");
					}
					break;
				default:
					tabObject.onPress =  function (event)
					{
					}	
					break;
				}		
			break;
		
			default:
				// Set tab function when user moves mouse over tab.
				tabObject.onMouseEnter = function (event)
				{
					var currTab = this.name.substring (this.name.lastIndexOf ("d")+1, this.name.lastIndexOf ("_"));
					// Seek through all list tabs. (Only the one being hovered should stay open.)
					for (var i = 1; i < snStatusPaneCommand.split; i++)
					{
						// If we've found the current tab,
						if (i == currTab) 
						{
							// Click the tab button to toggle visibility of its list.
							guiToggle ( "snStatusPaneCommandGroup" + currTab );
						}
						else // If it's another list tab,
						{
							// Ensure this tab is hidden.
							guiHide ("snStatusPaneCommandGroup" + i);
						}
					}
				}			
				// Set tab function when user clicks tab.
				tabObject.onPress = function (event)
				{
					// Click the tab button to toggle visibility of its list.
					guiToggle ( "snStatusPaneCommandGroup" + this.name.substring (this.name.lastIndexOf ("d")+1, this.name.lastIndexOf ("_")) );
				}				
			break;
		}
		
		// Get group.
		var groupObject = getGUIObjectByName ("snStatusPaneCommand" + "Group" + tabCounter);		

		// If the list hasn't been hidden (tab is open), and it should have list items (it's not merely a command),
		if ( type != "command" && attribute != undefined )
		{
			// Reset array.
			var listArray = [];
			// Insert blank array element at location 0 (to buffer array so items are in 1..n range).
			listArray[0] = "";

			// Extract item list into an array.		
			if (!attribute.length)			
			{	// If it's a list where each element is a value, (entity list)
				for ( var i in attribute )
				{
					listArray[listArray.length] = i;
					// Store any current quantity in the queue for this object.
//					if (attribute[i].quantity)
//						listArray[listArray.length].quantity = attribute[i].quantity;					
				}
			}
			else
			{	// If it's a list where each element is part of a numbered array, (array list)
				for ( var i = 0; i < attribute.length; i++ )
				{
					listArray[listArray.length] = attribute[i];
					// If cell sheet for each item is stored in attribute.sheet, transfer that across too.
					if (arrayCells == true)
					{
						//console.write (attribute[i].sheet);					
						listArray[listArray.length].sheet = new Object(attribute[i].sheet);
						//console.write (listArray[listArray.length].sheet);
					}
					// Store any current quantity in the queue for this object.
//					if (attribute[i].quantity)
//						listArray[listArray.length].quantity = attribute[i].quantity;					
				}
			}

			// Populate the buttons in this tab's list.
			for (var createLoop = 1; createLoop < snStatusPaneCommand.list.max; createLoop++)
			{
				// (Skip research list for the moment, since we don't have any portraits for techs. But we should remove the research condition later.)
				if (createLoop < listArray.length && type != "command")
				{
					// Get name of current button.
					var listObject = getGUIObjectByName ("snStatusPaneCommand" + tabCounter + "_" + (createLoop+1));

					switch (type)
					{
						case "pick":
							// Set tooltip.
							if (arrayCells == true) {	// Check a .sheet property for each item in the list to know its cellsheet.
								listObject.tooltip = cellGroup[listArray[createLoop].sheet][listArray[createLoop]].name;
							}
							else {	// Assume each item uses the same cell sheet as the tab.
								if(cellGroup[cellSheet][listArray[createLoop]]) {
									listObject.tooltip = cellGroup[cellSheet][listArray[createLoop]].name;
								}
								else {
									listObject.tooltip = "(no name)";
								}
							}
							
							// Set portrait.
							if(cellGroup[cellSheet][listArray[createLoop]]) {
								setPortrait (listObject.name, 
									"IconSheet", cellSheet + "Button", cellGroup[cellSheet][listArray[createLoop]].id);	
							}
							
							// Store properties which we will need when doing the press event as a comma-delimited string.
							// * 0 Name of variable that contains the ".curr" value.
							// * 1 String name of current button's content.
							// * 2 Array list of tab items.
							// * 3 Name of tab (eg stance).
							// * 4 Tab number.
							// (Storing the properties in the hotkey is an evil hack, and we'll eventually need the hotkey, but since new properties cannot be
							// attached to controls, there doesn't seem to be much choice ... Maybe add a .container property for situations such as this?)
							listObject.hotkey = attribute2String + "," + listArray[createLoop] + "," + attributeString + "," + cellSheet.toLowerCase() + "," + tabCounter + ",";
//							console.write (listObject.caption);
								
							// Set item function.
							listObject.onPress = function (event)
							{
								// Extract the properties from the comma-delimited string.
								var tempArray = parseDelimiterString (this.hotkey, ",");

								// Refresh tab so it displays the new .curr sprite.
								tabCounter = tempArray[4];
								updateTab (tempArray[3], "pick", "", tempArray[2], '"' + tempArray[1] + '"');
								
								// Do case-specific stuff depending on name of tab.
								switch (tempArray[3])
								{
									case "stance":
										issueCommand(selection, true, NMT_SET_STANCE, tempArray[1]);
										break;
									case "formation":
										// Remove the selected units from any existing formation.
										removeFromFormation (selection);
										// Set the selected entities to the chosen formation name.
										createEntityFormation (selection, tempArray[0]);
										// If the player is choosing to "disband" the formation (disable "batallion mode"),
										if ( tempArray[0] == "Loose" && isFormationLocked (selection) )
										{
											// Unlock the formation so each entity can be individually selected.
											lockEntityFormation (false);
										}
										else
										{
											// Lock the formation so the selection can be controlled as one unit.
											lockEntityFormation (true);
										}
									break;
									default:
									break;
								}
							}
						break;
						default:
							var itemName = "?";
							//console.write(listArray[createLoop]);
							switch (tab)
							{
								case "selection":
								case "garrison":
								case "research":
									// Get name of item to display in list.
									// (These already know the full name of the entity tag, so we don't prefix it.)
									itemName = listArray[createLoop];
								break;
								default:
									// Get name of item to display in list.
									itemName = selection[0].traits.id.civ_code + "_" + listArray[createLoop];
								break;
							}

//console.write ("4th: " + "snStatusPaneCommand" + tabCounter + "_1" + "|" + "IconSheet" + "|" + cellSheet + "Button" + "|" + createLoop + "|" + listArray[createLoop]);				

							switch (tab)
							{
								case "research":
									// Store name of tech to display in list in this button's coordinate.
									var tech = getTechnology(itemName, selection[0].player);
									Crd[getCrd (listObject.name, true)].entity = tech;
									
									if(!tech) {
										listObject.tooltip = "Bad tech: " + itemName;
									}
									else {
										// Set tooltip.
										listObject.tooltip = Crd[getCrd (listObject.name, true)].entity.generic + " (" + Crd[getCrd (listObject.name, true)].entity.specific + ")";
										
										// Set portrait.
										setPortrait (listObject.name, 
											Crd[getCrd (listObject.name, true)].entity.icon, 
											"Button", 
											Crd[getCrd (listObject.name, true)].entity.icon_cell);
									}
								break;
								default:
									// Store name of entity to display in list in this button's coordinate.
									Crd[getCrd (listObject.name, true)].entity = new Object(itemName);

									// Set tooltip.
									var template = getEntityTemplate(itemName);
									if (!template) break;
									listObject.tooltip = "[font=trebuchet14b]" + template.traits.id.specific + " (" +
									                     template.traits.id.generic + ")[/font]";
									
									//Show required resources list
									if (template.traits.creation) {
									    var first = true;
						    			if (template.traits.creation.resource.food > 0)
						    			{
											listObject.tooltip += (first ? "\n" : ", ");
											first = false;
						    				listObject.tooltip += "[font=tahoma10b]Food:[/font] " +
												    template.traits.creation.resource.food
						    			}
						    			if (template.traits.creation.resource.wood > 0)
						    			{
											listObject.tooltip += (first ? "\n" : ", ");
											first = false;
						    				listObject.tooltip += "[font=tahoma10b]Wood:[/font] " + 
												    template.traits.creation.resource.wood;
						    			}
									    if (template.traits.creation.resource.metal > 0)
								    	{
											listObject.tooltip += (first ? "\n" : ", ");
											first = false;
							    			listObject.tooltip += "[font=tahoma10b]Metal:[/font] " +
												    template.traits.creation.resource.metal
							    		}
							    		if (template.traits.creation.resource.stone > 0)
							    		{
											listObject.tooltip += (first ? "\n" : ", ");
											first = false;
								    		listObject.tooltip += "[font=tahoma10b]Stone:[/font] " + 
												    template.traits.creation.resource.stone
						    			}
									}

									if (template.traits.id.tooltip)
									{
										listObject.tooltip += "\n[font=tahoma10]" + 
										                      template.traits.id.tooltip + "[/font]";
									}
									
									// Set portrait.
									setPortrait (listObject.name, 
										template.traits.id.icon, 
										"Button" + toTitleCase(selection[0].traits.id.civ_code), 
										template.traits.id.icon_cell);
								break;
							}
								
							// Set item function.
							listObject.onPress = function (event)
							{
								switch (tab)
								{
									case "train":
										// TODO: Remove this item from the production queue if right-clicked.
										issueCommand(selection, true, NMT_PRODUCE, PRODUCTION_TRAIN, ""+(Crd[getCrd (this.name, true)].entity));
										break;
									case "research":
										// TODO: Remove this item from the production queue if right-clicked.
										issueCommand(selection, true, NMT_PRODUCE, PRODUCTION_RESEARCH, ""+(Crd[getCrd (this.name, true)].entity.name));
										break;
									case "barter":
										// Buy a quantity of this resource if left-clicked.
										// Sell a quantity of this resource if right-clicked.
										break;
									case "structCiv":
									case "structMil":
										// Select building placement cursor.
										tryConstruction( Crd[getCrd (this.name, true)].entity );
										break;
									case "garrison":
										// Remove this item from the entity's garrison inventory.
										break;
									case "selection":
										// Change the selection to this unit.
										break;
									default:
										break;
								}
							}									
						break;
					}
					
					// Create quantity container in entity's create list if necessary.
//					if (!attribute[listArray[createLoop]].quantity)
//						attribute[listArray[createLoop]].quantity = new Object(0);
					// Set caption to counter.
//					if (attribute[listArray[createLoop]].quantity > 1)
//						listObject.caption = attribute[listArray[createLoop]].quantity-1;
					// Store pointer to quantity in coordinate.
//					Crd[getCrd (listObject.name, true)].quantity = new Object(attribute[listArray[createLoop]].quantity);
					
					// Reveal portrait.
					guiUnHide (listObject.name);					
			
//				
//				// Set function that occurs when the button is pressed (left-clicked).
//				// (Somehow, we also need to do one for right-clicking -- decrement counter and remove item from queue.)
//				listObject.onPress = function (event)
//				{
//					switch (tab)
//					{
//						case "StructCiv":
//						case "StructMil":
//							// Create building placement cursor.
//							startPlacing (Crd[getCrd (this.name, true)].entity);
//						break;
//						default:
//							// Attempt to add the entry to the queue.
//							if (attemptAddToBuildQueue (selection[0], Crd[getCrd (this.name, true)].entity, Crd[getCrd (this.name, true)].tab, Crd[getCrd (this.name, true)].list))
//										if (attemptAddToBuildQueue (selection[0], itemName, tab, list))
//							{
////								// Create quantity container in entity's create list if necessary.
//	//							if (!attribute[Crd[getCrd (this.name, true)].list].quantity)
//		//							attribute[Crd[getCrd (this.name, true)].list].quantity = new Object(0);
//								// Increment counter.
//								attribute[Crd[getCrd (this.name, true)].list].quantity++;
//								// Set caption to counter.
//								if (attribute[Crd[getCrd (this.name, true)].list].quantity > 1)
//									this.caption = attribute[Crd[getCrd (this.name, true)].list].quantity-1;
//								
//								console.write (this.caption);
//							}
//						break;
//					}
//				}
//				
				}
				else
				{
					// Conceal this button.
					guiHide ("snStatusPaneCommand" + tabCounter + "_" + parseInt(createLoop+1));
					// Ensure it doesn't have a stored entity to display in list.
//					Crd[getCrd ("snStatusPaneCommand" + tabCounter + "_" + parseInt(createLoop+1), true)].entity = "";
					// Ensure it doesn't have a stored quantity of queued items.
//					Crd[getCrd ("snStatusPaneCommand" + tabCounter + "_" + parseInt(createLoop+1), true)].quantity = 0;
				}
			}
		}
		
		// Default the list to closed.
		groupObject.hidden = true;			
		
		tabCounter++;
		
		// Completed tab update successfully.
		return true;
	}
	// Invalid property ... Cancel tab update.
	return false;
}

// ====================================================================

function tryConstruction( name )
{
	// Start the building placement cursor for the given template, if we have enough resources

	var result = checkEntityReqs( localPlayer, getEntityTemplate( name ) );

	if (result == true) // If the entry meets requirements to be built (i.e. resources)
	{
		// Show the building placement cursor
		startPlacing( name );
	}
	else
	{	
		// If not, output the error message.
		//console.write( result );
		showMessage(result);
	}
}

// ====================================================================

function refreshCommandButtons()
{
	// Reset button counter.
	tabCounter = 1;

	if ( selectionChanged )
	{
		// Update production lists (both types of Construction, Train). (Tab button, persistent buttons, click them to do things.)
		if (validProperty ("selection[0].actions.create.list"))
		{
			listRoot = selection[0].actions.create.list;
			for (listTab in listRoot)
			{
				if (listTab != "research")	// Do research later.
					updateTab (listTab, "production", "Tab", "listRoot[listTab]", "");
			}	
		}
	
		// Update Barter. (Tab button, persistent buttons, click them to do things.)
		updateTab ("barter", "production", "Tab", "selection[0].actions.barter.list");
		
		// Update pick lists (formation, stance, trade). (Tab button holds current selection, click a button to select a new option and close the tab.)
		//updateTab ("formation", "pick", "", "selection[0].traits.formation.list", "selection[0].traits.formation.curr");
		updateTab ("stance", "pick", "", "selection[0].traits.ai.stance.list", "selection[0].traits.ai.stance.curr");
		//updateTab ("trade", "pick", "", "selection[0].actions.trade.list", "selection[0].actions.trade.curr");
		//updateTab ("gate", "pick", "", "selection[0].actions.gate.list", "selection[0].actions.gate.curr");
		//updateTab ("weapon", "pick", "", "selection[0].actions.attack", "selection[0].actions.attack.curr");
/*		
		// Update Allegiance. (Tab button, persistent buttons (though unit will be lost as soon as option selected), click them to do things.)
		tempArray = new Array ("kill", "food");
		tempArray.sheet = new Object();
		tempArray.sheet = new Array ("Command", "Resource");
		console.write(tempArray[1].sheet);
		updateTab ("allegiance", "pick", "Tab", "", "", true);
*/		
		// Update research. (Tab button, persistent buttons, click them to do things.)
		updateTab ("research", "production", "", "selection[0].actions.create.list.research", "");
		
		// End of production and pick lists. Store end position.
		var listCounter = tabCounter;
		
		// Commands begin from this point.
		tabCounter = snStatusPaneCommand.split;
		// Update commands. (Click "tab" button to do something; no list.)		
		updateTab ("patrol", "command", "", "selection[0].actions.patrol", "");
		updateTab ("townbell", "command", "", "selection[0].actions.townBell", "");				
		updateTab ("rally", "command", "", "selection[0].actions.create.rally", "");		
		updateTab ("explore", "command", "", "selection[0].actions.explore", "");		
		updateTab ("retreat", "command", "", "selection[0].actions.retreat", "");			
		updateTab ("stop", "command", "", "selection[0].actions.stop", "");	

		// End of commands. Store end position.
		var commandCounter = tabCounter;
	}
	else
	{
		// Ensure tabs should be cleanable.
		listCounter		= 1;
		commandCounter 	= snStatusPaneCommand.split-1;
	}

	if ( selectionChanged )
	{
		// Clear remaining buttons between the lists and commands.
		for (var commandClearLoop = listCounter; commandClearLoop <= snStatusPaneCommand.split-1; commandClearLoop++)
		{
			guiHide ("snStatusPaneCommand" + commandClearLoop + "_1");
			// If this slot could possibly contain a list, hide that too.
			guiHide ("snStatusPaneCommand" + "Group" + commandClearLoop);
		}
		for (var commandClearLoop = commandCounter; commandClearLoop <= snStatusPaneCommand.tab.max; commandClearLoop++)
		{
			guiHide ("snStatusPaneCommand" + commandClearLoop + "_1");
			// If this slot could possibly contain a list, hide that too.
			guiHide ("snStatusPaneCommand" + "Group" + commandClearLoop);
		}		
	}
	
	if ( selectionChanged )
	{
		// Do the selection/garrison list.
		tabCounter = snStatusPaneCommand.tab.max;
		
		// If there are entities garrisoned in the current entity,
		if (selection[0].traits.garrison && selection[0].traits.garrison.curr > 0)
		{
			updateTab ("garrison", "production", "Tab", "selection", "");	
		}
		else
		{
			// If more than one entity is selected, list them.
			if (selection.length > 1)
			{
				tempArray = [];
				for ( var i = 0; i < selection.length; i++ )
					tempArray[i] = selection[i].tag;
				tempArray.length = i;
				updateTab ("selection", "production", "Tab", "tempArray", "");	
			}
		}
	}
}


// ====================================================================

// ====================================================================
/*
function updateList (listTab, listGroup)
{
	// If any items in the list (construction, train, research, etc, need to be updated, update that list.)
	if ( shouldUpdateStat (listGroup) )
	{
		// Get tab.
		tabObject 	= getGUIObjectByName ("snStatusPaneCommand" + listCounter + "_1");		
		groupObject = getGUIObjectByName ("snStatusPaneCommand" + "Group" + listCounter);		
		// Enable tab.
		guiUnHide (tabObject.name);
		// Set tab portrait.
		setPortrait ("snStatusPaneCommand" + listCounter + "_1", "IconSheet", "TabButton", cellGroup["Tab"][listTab].id);
		tooltip = cellGroup["Tab"][listTab].name;
		tooltip += " Tab";
		tabObject.tooltip = tooltip;
		// Set tab function.
		tabObject.onPress = function (event)
		{
			// Click the tab button to toggle visibility of its list.
			guiToggle ( "snStatusPaneCommandGroup" + this.name.substring (this.name.lastIndexOf ("d")+1, this.name.lastIndexOf ("_")) );
		}

		// If the list hasn't been hidden (tab is open),
		if ( groupObject.hidden == false )
		{
			
			// Extract entity list into an array.
			listArray = [];
			for ( i in listGroup )
			{
				listArray[listArray.length] = i;
				// Store any current quantity in the queue for this object.
//				if (listGroup[i].quantity)
//					listArray[listArray.length].quantity = listGroup[i].quantity;
			}

			// Populate the buttons in this tab's list.
			for (createLoop = 1; createLoop < snStatusPaneCommand.list.max; createLoop++)
			{
				if (createLoop < listArray.length)
				{
					// Get name of current button.
					listObject = getGUIObjectByName ("snStatusPaneCommand" + listCounter + "_" + (createLoop+1));

					// Get name of entity to display in list.
					updateListEntityName = selection[0].traits.id.civ_code + "_" + listArray[createLoop];

					// Store name of entity to display in list in this button's coordinate.
//					Crd[getCrd (listObject.name, true)].entity = new Object(updateListEntityName);

					// Set tooltip.
					listObject.tooltip = getEntityTemplate(updateListEntityName).traits.id.civ + " " + getEntityTemplate(updateListEntityName).traits.id.generic;
					
					// Create quantity container in entity's create list if necessary.
//					if (!listGroup[listArray[createLoop]].quantity)
//						listGroup[listArray[createLoop]].quantity = new Object(0);
					// Set caption to counter.
//					if (listGroup[listArray[createLoop]].quantity > 1)
//						listObject.caption = listGroup[listArray[createLoop]].quantity-1;
					// Store pointer to quantity in coordinate.
//					Crd[getCrd (listObject.name, true)].quantity = new Object(listGroup[listArray[createLoop]].quantity);
					
					// Set portrait.
					switch (listTab)
					{
						case "research":
							// Skip research list for the moment, since we don't have any portraits for techs.
						break;
						default:
							setPortrait (listObject.name, 
								getEntityTemplate(updateListEntityName).traits.id.icon, 
								toTitleCase(selection[0].traits.id.civ_code), 
								getEntityTemplate(updateListEntityName).traits.id.icon_cell);
						break;
					}
					
					// Reveal portrait.
					guiUnHide (listObject.name);					
*/					
/*				
					// Set function that occurs when the button is pressed (left-clicked).
					// (Somehow, we also need to do one for right-clicking -- decrement counter and remove item from queue.)
					listObject.onPress = function (event)
					{
						switch (listTab)
						{
							case "StructCiv":
							case "StructMil":
								// Create building placement cursor.
								startPlacing (Crd[getCrd (this.name, true)].entity);
							break;
							default:
								// Attempt to add the entry to the queue.
								if (attemptAddToBuildQueue (selection[0], Crd[getCrd (this.name, true)].entity, Crd[getCrd (this.name, true)].tab, Crd[getCrd (this.name, true)].list))
//										if (attemptAddToBuildQueue (selection[0], updateListEntityName, tab, list))
								{
//								// Create quantity container in entity's create list if necessary.
		//							if (!listGroup[Crd[getCrd (this.name, true)].list].quantity)
			//							listGroup[Crd[getCrd (this.name, true)].list].quantity = new Object(0);
									// Increment counter.
									listGroup[Crd[getCrd (this.name, true)].list].quantity++;
									// Set caption to counter.
									if (listGroup[Crd[getCrd (this.name, true)].list].quantity > 1)
										this.caption = listGroup[Crd[getCrd (this.name, true)].list].quantity-1;
									
									console.write (this.caption);
								}
							break;
						}
					}
*/			
/*
				}
				else
				{
					// Conceal this button.
					guiHide ("snStatusPaneCommand" + listCounter + "_" + parseInt(createLoop+1));
					// Ensure it doesn't have a stored entity to display in list.
//					Crd[getCrd ("snStatusPaneCommand" + listCounter + "_" + parseInt(createLoop+1), true)].entity = "";
					// Ensure it doesn't have a stored quantity of queued items.
//					Crd[getCrd ("snStatusPaneCommand" + listCounter + "_" + parseInt(createLoop+1), true)].quantity = 0;
				}
			}
		}
		listCounter++;
	}
}
*/

// ====================================================================
/*
function refreshCommandButtons()
{
	// Set start of tabs.
	listCounter	= 1;

	if ( selection[0].actions && shouldUpdateStat( "actions" ) )
	{
		// Update production lists (Construction, Train, Barter). (Tab button, persistent buttons, click them to do things.)
		if ( shouldUpdateStat( "actions.create" ) && shouldUpdateStat( "actions.create.list" ) )
		{
			// Everything in this block is tied to properties in
			// actions.create.list, the above check should limit the
			// number of times this update is needlessly made.

			// Get train/research/build lists by seeking through entity's creation list.
			listRoot = selection[0].actions.create.list;
			
			for (listTab in listRoot)
			{
				// Note: This check indicates the production lists are updated twice on some occasions. I don't know why (error from shouldUpdate()?).
				console.write (listTab + " " + listRoot[listTab]);
				if (listTab != "research")	// Do research later.
					updateList (listTab, listRoot[listTab]);
			}
		}
			
		// Update selection lists (formation, stance, trade). (Tab button holds current selection, click a button to select a new option and close the tab.)
			
		// Update research production list (which should always go last after all the other lists).
		if ( shouldUpdateStat( "actions.create" ) && shouldUpdateStat( "actions.create.list" ) 
		  && shouldUpdateStat( "actions.create.list.research" ) && selection[0].actions.create.list.research )
			{
				updateList ("research", selection[0].actions.create.list.research);
			}
			
		// Update commands. (Click "tab" button to do something; no list).
			
*/	
// This whole section needs to be rewritten (now list of XML attributes instead of semicolon-delimited string).
/*
			unitArray 	= UpdateList(action_tab_train, listCounter); 		if (unitArray != 0)	 listCounter++;
			structcivArray 	= UpdateList(action_tab_buildciv, listCounter);		if (structcivArray != 0) listCounter++;
			structmilArray 	= UpdateList(action_tab_buildmil, listCounter);		if (structmilArray != 0) listCounter++;
			techArray 	= UpdateList(action_tab_research, listCounter);		if (techArray != 0)	 listCounter++;
			
			formationArray 	= UpdateList(action_tab_formation, listCounter);	if (formationArray != 0) listCounter++;
			stanceArray 	= UpdateList(action_tab_stance, listCounter);		if (stanceArray != 0)	 listCounter++;
*/	
/*	
		if ( shouldUpdateStat( "actions" ) )
		{
			// Update commands.
			commandCounter = snStatusPaneCommand.tab.max;
*/			
/*		
			commandCounter = UpdateCommand(cellGroup["Command"]["attack"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Command"]["patrol"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Command"]["repair"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Gather"]["food"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Gather"]["wood"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Gather"]["stone"].id, commandCounter);
			commandCounter = UpdateCommand(cellGroup["Gather"]["ore"].id, commandCounter);
*/	
/*	
		}
	}

	if (listCounter > 0 && commandCounter > 0)
	{
		// Clear remaining buttons between them.
		for (commandClearLoop = listCounter; commandClearLoop <= commandCounter; commandClearLoop++)
		{
			guiHide ("snStatusPaneCommand" + commandClearLoop + "_1");
			// If this slot could possibly contain a list, hide that too.
			guiHide ("snStatusPaneCommand" + "Group" + commandClearLoop);
		}
	}
*/	
/*
	// Update production queue.
	GUIObject = getGUIObjectByName("snStatusPaneCommandProgress");
	// If the entity has a production item underway,
	if ( selection[0].actions.create && selection[0].actions.create.progress
	  && selection[0].actions.create.progress.valueOf()
          && selection[0].actions.create.progress.valueOf().current
          && selection[0].actions.create.queue.valueOf()
          && selection[0].actions.create.queue.valueOf()[0].traits.creation.time )
	{

		// Set the value of the production progress bar.
		GUIObject.caption = ((Math.round(Math.round(selection[0].actions.create.progress.valueOf().current)) * 100 ) / Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time));
		// Set position of progress bar.
		GUIObject.size = getGUIObjectByName("snStatusPaneCommand" + selection[0].actions.create.queue.valueOf()[0].tab + "_" + selection[0].actions.create.queue.valueOf()[0].list).size;
		// Set progress bar tooltip.
//		GUIObject.tooltip = "Training " + selection[0].actions.create.queue.valueOf()[0].traits.id.generic + " ... " + (Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time-Math.round(selection[0].actions.create.progress.valueOf().current)) + " seconds remaining.";
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
//}

// ====================================================================
/*
function UpdateCommand(listIcon, listCounter)
{
	// Similar to UpdateList, but without the list.
	// Updates a particular command button with a particular action.

	if ( selection[0].actions && (
            (listIcon == action_attack && selection[0].actions.attack)
         || (listIcon == action_patrol && selection[0].actions.patrol)
         || (listIcon == action_repair && selection[0].actions.repair)
         || (listIcon == action_gather_food && selection[0].actions.gather
				&& selection[0].actions.gather.resource && selection[0].actions.gather.resource.food)
         || (listIcon == action_gather_wood && selection[0].actions.gather
				&& selection[0].actions.gather.resource && selection[0].actions.gather.resource.wood)
         || (listIcon == action_gather_stone && selection[0].actions.gather
				&& selection[0].actions.gather.resource && selection[0].actions.gather.resource.stone)
         || (listIcon == action_gather_ore && selection[0].actions.gather
				&& selection[0].actions.gather.resource && selection[0].actions.gather.resource.ore)
           )
		)
	{	
		// Set appearance of tab.
		setPortrait("snStatusPaneCommand" + listCounter + "_1", "IconSheetCommand", "Button", listIcon);
		guiUnHide("snStatusPaneCommand" + listCounter + "_1");

		// Hide its list.
		guiHide("snStatusPaneCommand" + "Group" + listCounter);

		// Store content info in tab button for future reference.
		snStatusPaneCommand[listCounter][1].type = "command";
		snStatusPaneCommand[listCounter][1].last = 0;
		snStatusPaneCommand[listCounter][1].name = listIcon;

		return (listCounter-1);
	}
	else
		return (listCounter);
}
*/

// ====================================================================
/*
function pressCommandButton(commandButton)
{
	// Determine current object, tab, and list from command button name.
	tab = commandButton.name.substring (commandButton.name.lastIndexOf ("d")+1, commandButton.name.lastIndexOf ("_"));
	list = commandButton.name.substring (commandButton.name.lastIndexOf ("_")+1, commandButton.name.length);

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
								issueCommand (selection, NMT_PATROL, x, y);
							}
						);
					break;
					case action_attack:
						setCursor ("action-attack");
						selectEntity(
							function (target) {
								issueCommand (selection, NMT_ATTACK_MELEE, target);
							}
						);
					break;
				}
			}
		break;
		default:
			// Left-clicked list button.
			console.write("Clicked [" + tab + "," + list + "]: list of type " + snStatusPaneCommand[tab][list].type + "; " + snStatusPaneCommand[tab][list].name + " " + snStatusPaneCommand[tab][list].object);

			switch (snStatusPaneCommand[tab][list].name)
			{
				case action_tab_buildciv:
				case action_tab_buildmil:
					// Create building placement cursor.
					startPlacing(selection[0].traits.id.civ_code + "_" + snStatusPaneCommand[tab][list].object);
				break;
				default:
					// Attempt to add the entry to the queue.
					attemptAddToBuildQueue( selection[0], selection[0].traits.id.civ_code + "_" + snStatusPaneCommand[tab][list].object, tab, list);
				break;
			}
		break;
	}
}
*/


// ====================================================================
/*
function UpdateListold(listIcon, listCounter)
{
	// Populates a given column of command icons with appropriate build portraits for the selected object.
	// Returns an array of this selection.

	// Build unit list.
	if ( selection[0].traits.id.civ_code
	&& selection[0].actions
	&& selection[0].actions.create
	&& selection[0].actions.create.list )
	{
		list = null;

		switch (listIcon)
		{
			case action_tab_train:
				if ( selection[0].actions.create.list.unit && shouldUpdateStat( "actions.create.list.unit" ) )
					list = selection[0].actions.create.list.unit;
			break;
			case action_tab_buildciv:
				if ( selection[0].actions.create.list.structciv && shouldUpdateStat( "actions.create.list.structciv" ) )
					list = selection[0].actions.create.list.structciv;
			break;
			case action_tab_buildmil:
				if ( selection[0].actions.create.list.structmil && shouldUpdateStat( "actions.create.list.structmil" ) )
					list = selection[0].actions.create.list.structmil;
			break;
			case action_tab_research:
				if ( selection[0].actions.create.list.tech && shouldUpdateStat( "actions.create.list.tech" ) )
					list = selection[0].actions.create.list.tech;
			break;
			default:
				return 0;
			break;
		}

		if ( list )
		{
			// Enable tab portrait.
			guiUnHide("snStatusPaneCommand" + "Group" + listCounter);
			guiUnHide("snStatusPaneCommand" + listCounter + "_1");
			setPortrait("snStatusPaneCommand" + listCounter + "_1", "sheet_action", "", listIcon);
			// Reset list length.
			snStatusPaneCommand[listCounter][1].last = 0;
			// Store content info in tab button for future reference.
			snStatusPaneCommand[listCounter][1].type = "list";

			// Extract entity list into an array.
			listArray = [];
			for( i in list )
			{
				listArray[listArray.length] = i.toString();
			}

			// Populate appropriate command buttons.
			for (createLoop = 1; createLoop < snStatusPaneCommand.list.max; createLoop++)
			{
				if (createLoop < listArray.length)
				{
					// Get name of entity to display in list.
					UpdateListEntityName = selection[0].traits.id.civ_code + "_" + listArray[createLoop];

					getGUIObjectByName ("snStatusPaneCommand" + listCounter + "_" + (createLoop+1)).caption = "";

					guiUnHide("snStatusPaneCommand" + listCounter + "_" + (createLoop+1));
					setPortrait("snStatusPaneCommand" + listCounter + "_" + (createLoop+1), 
						getEntityTemplate(UpdateListEntityName).traits.id.icon, 
						selection[0].traits.id.civ_code, 
						getEntityTemplate(UpdateListEntityName).traits.id.icon_cell);
					
					// Store content info in tab button for future reference.
					snStatusPaneCommand[listCounter][createLoop+1].name = listIcon;
					snStatusPaneCommand[listCounter][createLoop+1].object = listArray[createLoop];
					snStatusPaneCommand[listCounter][createLoop+1].type = "list";
					snStatusPaneCommand[listCounter][createLoop+1].last++;
				}
				else
					guiHide("snStatusPaneCommand" + listCounter + "_" + parseInt(createLoop+1));
				}
			}
			return listArray;
		}
	}

	return 0;
}
*/

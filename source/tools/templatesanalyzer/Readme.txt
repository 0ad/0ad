Template Analyzer.

This python tool has been written by wraitii. Its purpose is to help with balancing and "rock-paper-scissors" mechanic and provide some easy-to-read data about civilizations and units. Take its results with a grain of salt: the in-game results can vary based on micromanaging, pathfinding issues, and sheer luck. However, my testings so far have shown it's usually not too wrong, and I think that assuming two equal players the data will be somewhat accurate.

Note that this doesn’t account for techs and building costs, or auras and things like that.


Mod makers that would like to compare your units with vanilla's: read "Customizing unitTables" below.

########################################################################
########################################################################
1. unitTables.py

This is the main script, the one which returns the comparison tables. It has 6 kinds of output, and many parameters.

------------------------------------------------------------
- Unit Tables -
Those are shown for generic templates and can be activated for civs (see below). It basically lists some unit stats so you don't have to check the templates.


------------------------------------------------------------
- Unit Comparison Matrix -
Those are the two side-by-side big tables. The left one uses a Green-Yellow-Red color Scheme, the right one a Red-white-Green.

The matrix on the left shows how strong a unit is compared to another one in fight. This calculation does not take costs into account, its purpose is to yield an indicator of who would win in an arena fight. Results are somewhat inaccurate for ranged units, particularly in ranged vs melee fights, as micro takes a really big importance for those.
A completely red value means the unit cannot win this fight, and that 100 units would only do minimal damage to the enemy, whilst being completely destroyed.
Yellow means that the two units are fairly equivalent, and a well microed fight would end up in a stalemate, more or less.
Green means the unit is stronger. Full Green means 4 times stronger, and any higher value would still be shown as full green. Elephants for example can sometimes be up to 10/20 stronger, which will not be shown.

The matrix on the right shows hardcoded counters. Greener means higher counter. A value of 2.5 is full green. Red values show units that are less effective at attacking other units. Full Red can mean that the unit cannot attack this kind of enemy (for example women can't attack anything, basically).


------------------------------------------------------------
- Unit Worthiness -
Those are the Red/Blue pyramids. Those show how powerful, cost-wise, a unit is. Thus the longer the bar, the better the unit will be cost-wise. Red bars show offensive power, while blue bars show resiliency (HP and armor). This being a simple scalar, it does not reflect really accurately how units behave, since it eg does not take hard-coded counters into account. It can be however used eg to see if a champion version of a unit is effectively worth its heightened cost, or not.

------------------------------------------------------------
- Unit Specializations -
This table compares units that inherit from a generic template to this generic template (note that this means that barracks-specific versions of champion units don't count.). The graph shows the difference with the generic template, and the last column is how much this changes the worth or the unit (according to the script).
If you want to data mine here, I recommend you copy/paste this table in Excel or Numbers to be able to sort it by column.

------------------------------------------------------------
- Roster Variety -
This shows in a simple manner which civs have units inheriting from which template, which is a simple but accurate way of showing the roster variety of a civ. The less units, the variety the civ has.

------------------------------------------------------------
- Civilization Comparison Tables -
Those tables compare civilizations using the Unit Comparison Matrixes. The intent is to give a portrait of how good a civ's units are against another civ's, to check at a glance if civilizations are properly balanced. Note that this is all statistical analysis, and that however well the tool will work, this requires interpretation. Don't take this at face value. In particular, this doesn't take technologies or buildings into account, so you need to keep that in mind. Perhaps a super OP unit requires an insanely expensive tech, making it more balanced.

The 3 left-most columns do not take costs into account (but do take the "ranged efficiency" factor into account.) The others do.
It is often interesting to compare the columns taking costs into account and those not doing so.

I'll refer to the Civ being looked at as "Civ A" and the ones listed in the table as "Civ B"

The "Average Efficiency" Columns show, on average, how strong Civ A units are against Civ B's. Since this uses the Unit Comparison Matrices, any value above "1" means that on average, Civ A units are stronger. However some units can bring this value up (such as elephants, which are really strong), even in the "cost adjusted" version. Likewise, a unit that's particularly weak or easily countered would bring average efficiency down.
Still, values far above 1 (>2…) should be investigated using the Unit Comparison Matrices themselves, and checking unit costs. You can remove some units specifically from the tables to check if it changes something (see below).

The "Maximal Minimal Efficiency" columns show how strong the weakest unit of Civ A is. If the value is close to 1 (or even above), this means that civ A has a unit that Civ B cannot counter, ie Civ A has a unit that would win a fight against any of Civ B's units (note that for the column taking costs into account, this means that for the same amount of resources spent, Civ A could always train an army that Civ B cannot defeat). Obviously this is fairly bad, and again though this tool isn't perfect, such data is a pretty good indication that civs are unbalanced. On the other hand, a very low value means that Civ A has a unit Civ B counters perfectly. This is not necessarily an issue.

The "Minimal Maximal Efficiency" columns show how weak the strongest unit of Civ A is. As you can expect, contrarily to "Maximal Minimal Efficiency", this column should be above "1". A value near "1" or even below it means that Civ A's strongest unit would lose a fight against any of Civ B's units. Again, this is a fairly good indicator that balance is wrong. Check "Range Efficiency" and costs to see if those factors explain this discrepancy.

The "Ranged Efficiency" column show how strong the ranged units of Civ A are, compared to Civ B. Values above 100% mean that Civ A has better archers/javelineers/slingers, and would thus probably have an advantage in open fights, which thus affects its unit efficiency. This can explain why a civ appears unbalanced (generally too strong or too weak).

The "Countered the least" and "Counters the least" show, respectively, which unit "Maximal Minimal Efficiency" and "Minimal Maximal Efficiency" refer to. You can use this to easily check in the Unit Comparison Matrices which unit is apparently too strong or too weak, which can help you remove some templates for a fairer comparison in some cases, or simply rebalance this unit. 

The "Balance" column gives an easy to read indicator of how balanced civs are. This is really just a basic reading of the values in the table, and should not be taken at face value.

Note that if "Max Min Efficiency" and "Min Max Efficiency" are both very close to "1", and average efficiency too, this most likely means the civs are fairly balanced, assuming techs and buildings do not mess this up.

------------------------------------------------------------
- Customizing UnitTables -
Since this script cannot hope to provide an objective verdict of how strong civs are against one another, it has a variety of easily adjustable parameters that you can use to get a more accurate idea of how civs behave.

The variables below "#Generic templates to load" can be used to load only some templates. They refer to the file names, and you can add your own. Those are for the generic templates.

"Civs" will tell the script which civilizations to compare. Modders, to add your own, add it here (and see below for changing BasePath)
"CivBuildings" will tell the script what buildings to look at. Those are the end of structure template files, of format {CivName}_{BuildingName}.xml
	> You can for example restrict this to ["barracks"] to only load units from the barracks.
"FilterOut" can be used to prune Civ templates. Any template which has any of these strings in its name will be removed. You can use this to remove specific templates, or whole collections (such as templates having "mechanical_siege" in their name.)

Graphic Parameters are as such:
"ComparativeSortByCav" will sort the Unit Comparison Matrices so that Cavalry and Infantry are separate instead of mixed.
"ComparativeSortByChamp" will sort the Unit Comparison Matrices so that Champions and Citizen Soldiers are separate instead of mixed.
"SortTypes" gives the order in which to sort units. This refers to their classes, so put the rare-most classes in front (eg Pike before Spear)
"ShowCivLists" will display the unit lists for civilizations too and not only generic templates.

Actual script parameters are as such:
"paramIncludePopCost" tells wether to include pop cost in the costs or not. Units that take more pop will be seen as weaker.
"paramFactorCounters" tells wether to count hardcoded counters or not. This can be used to see if counters have the desired effect.
"paramDPSImp" affects how important Damage Per Second (ie attack) is compared to HP and armour in calculating a unit's worthiness.
"paramHPImp" affects how important HP is compared to DPS in calculating a unit's worthiness. Lower is more important.
"paramRessImp" can be tweaked to make some resource types appear more costly. This can be used to check civ balance on maps where wood is scarce, for example.
"paramBuildTimeImp" (range [0-1], >1 is OK) affects how Build Time changes unit cost. With higher values, units that are slow to build are seen as weaker.
"paramSpeedImp" (range [0-1]) affects how important Speed is. With higher values, faster units will be stronger (this also affects unit comparison matrices.)
"paramRangedCoverage" (range [0-1] >1 is OK) affects how much "Ranged Efficiency" counts when comparing civs with one another. Higher values mean more importance.
"paramRangedMode" is used to know how to calculate a civilisation's Ranged Efficiency, either by averaging its units or taking the strongest one.

"paramMicroAutoSpeed" is particular: it is used to give a base advantage to ranged units over slower melee units. This parameters is the difference threshold at which this advantage takes place. For example, with a melee unit having a speed of 10, if this parameter is 2.5, only ranged units whose speed is 12.5 or more will benefit of this advantage. This can be used to simulate micro. Only affects Unit Comparison Matrices.

"paramMicroPerfectSpeed" is similar, only in this case the advantage is extremely high. This is used to simulate "perfect" micro, as ranged units can kill any much slower melee units without taking any hits if properly microed. If you want to check civs on the whole, set this really high to disable it as in the heat of the moment it's rare for micro to be this perfect. Only affects Unit Comparison Matrices.


Modders, change "BasePath" to something else if you want to use civs from another mod than "Public". To compare your civs with those in public, I recommend you copy the "simulation/templates" folder of the public mod somewhere else, copy your units in, and point BasePath to that folder.

########################################################################
########################################################################
2. CreateRMTest.py

This simple script takes two civilizations, and parameters which work as in unitTable.py, and creates a Random Map script with triggers to easily compare armies in-game. The created files are in maps/random, named "test_demobalance". You can then change the attacking and defending units to whatever you see fit, as well as change the size of armies. Not that this creates armies randomly based on a list of templates, so comparing whole civs will usually come down to which civs has the most of its strongest units. It is more accurate to compare one-of-a-kind armies, and even then you can only micro one side, which makes a lot of difference.


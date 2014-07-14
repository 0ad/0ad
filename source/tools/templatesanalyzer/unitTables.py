import xml.etree.ElementTree as ET
import os
import glob

# What data to use
AttackTypes = ["Hack","Pierce","Crush"]
Resources = ["food", "wood", "stone", "metal"]

#Generic templates to load
Class = ["_champion",""]
Types = ["infantry","cavalry"];
Subtypes = ["spearman","pikeman","swordsman", "archer","javelinist","slinger"];
Fix = ["melee_","ranged_",""]
AddedTemplates = ["template_unit_champion_elephant_melee.xml"]

# Those describe Civs to analyze and buildings to consider.
# That way you can restrict to some buildings specifically.
Civs = ["athen", "mace", "spart", "cart", "rome", "pers", "maur", "brit", "gaul", "iber"]
#Civs = ["rome","pers"]
CivBuildings = ["civil_centre", "barracks","gymnasion", "stables", "elephant_stables", "fortress", "embassy_celtic", "embassy_italiote", "embassy_iberian"]
#CivBuildings = ["civil_centre", "barracks"]

# Remote Civ templates with those strings in their name.
FilterOut = ["hero", "mecha", "support", "barracks"]

# Graphic parameters: affects only how the data is shown
ComparativeSortByCav = True
ComparativeSortByChamp = True
SortTypes = ["Support","Pike","Spear","Sword", "Archer","Javelin","Sling","Elephant"];	# Classes
ShowCivLists = False

# Parameters: affects the data
paramIncludePopCost = False
paramFactorCounters = True;	# False means attack bonuses are not counted.
paramDPSImp = 5
paramHPImp = 3	# Decrease to make more important
paramRessImp = { "food" : 1.0, "wood" : 1.0, "stone" : 2.0, "metal" : 1.5 }
paramBuildTimeImp = 0.5	# Sane values are 0-1, though more is OK.
paramSpeedImp = 0.2		# Sane values are 0-1, anything else will be weird.
paramRangedCoverage = 1.0  	# Sane values are 0-1, though more is OK.
paramRangedMode = "Average"	#Anything but "Average" is "Max"
paramMicroAutoSpeed = 2.5	# Give a small advantage to ranged units faster than melee units (by at least this parameter). Making this too high will disable this advantage. This simulates the advantage a bit of micro would give
paramMicroPerfectSpeed = 5.0	# Likewise, only this assumes perfect micro. Thus a ranged unit a lot faster is invincible. For example chariot v Elephant is a large win for elephants without this, but a large win for chariots with, since chariots can easily outmaneuver elephants if microed perfectly.

# This is the path to the /templates/ folder to consider. Change this for mod support.
basePath = os.path.realpath(__file__).replace("unitTables.py","") + "../../../binaries/data/mods/public/simulation/templates/"


def htbout(file, balise, value):
	file.write("<" + balise + ">" + value + "</" + balise + ">\n" )
def htout(file, value):
	file.write("<p>" + value + "</p>\n" )

def CalcUnit(UnitName, existingUnit = None):
	unit = { 'HP' : "0", "BuildTime" : "0", "Cost" : { 'food' : "0", "wood" : "0", "stone" : "0", "metal" : "0", "population" : "0"},
	'Attack' : { "Melee" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 }, "Ranged" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 } },
	'RepeatRate' : {"Melee" : "0", "Ranged" : "0"},'PrepRate' : {"Melee" : "0", "Ranged" : "0"}, "Armour" : {},
	"Ranged" : "false", "Classes" : [], "AttackBonuses" : {}, "Restricted" : [],
	"Civ" : None };
	
	if (existingUnit != None):
		unit = existingUnit

	Template = ET.parse(UnitName)
	
	# Recursively get data from our parent which we'll override.
	if (Template.getroot().get("parent") != None):
		unit = CalcUnit(Template.getroot().get("parent") + ".xml", unit)
		unit["Parent"] = Template.getroot().get("parent") + ".xml"

	if (Template.find("./Identity/Civ") != None):
		unit['Civ'] = Template.find("./Identity/Civ").text

	if (Template.find("./Health/Max") != None):
		unit['HP'] = Template.find("./Health/Max").text

	if (Template.find("./Cost/BuildTime") != None):
		unit['BuildTime'] = Template.find("./Cost/BuildTime").text
	
	if (Template.find("./Cost/Resources") != None):
		for type in list(Template.find("./Cost/Resources")):
			unit['Cost'][type.tag] = type.text

	if (Template.find("./Attack/Melee") != None):
		if (Template.find("./Attack/Melee/RepeatTime") != None):
			unit['RepeatRate']["Melee"] = Template.find("./Attack/Melee/RepeatTime").text
		if (Template.find("./Attack/Melee/PrepareTime") != None):
			unit['PrepRate']["Melee"] = Template.find("./Attack/Melee/PrepareTime").text
		for atttype in AttackTypes:
			if (Template.find("./Attack/Melee/"+atttype) != None):
				unit['Attack']['Melee'][atttype] = Template.find("./Attack/Melee/"+atttype).text
		if (Template.find("./Attack/Melee/Bonuses") != None):
			for Bonus in Template.find("./Attack/Melee/Bonuses"):
				Against = []
				CivAg = []
				if (Bonus.find("Classes") != None and Bonus.find("Classes").text != None):
					Against = Bonus.find("Classes").text.split(" ")
				if (Bonus.find("Civ") != None and Bonus.find("Civ").text != None):
					CivAg = Bonus.find("Civ").text.split(" ")
				Val = float(Bonus.find("Multiplier").text)
				unit["AttackBonuses"][Bonus.tag] = {"Classes" : Against, "Civs" : CivAg, "Multiplier" : Val}
		if (Template.find("./Attack/Melee/RestrictedClasses") != None):
			newClasses = Template.find("./Attack/Melee/RestrictedClasses").text.split(" ")
			for elem in newClasses:
				if (elem.find("-") != -1):
					newClasses.pop(newClasses.index(elem))
					if elem in unit["Restricted"]:
						unit["Restricted"].pop(newClasses.index(elem))
			unit["Restricted"] += newClasses


	if (Template.find("./Attack/Ranged") != None):
		unit['Ranged'] = "true"
		if (Template.find("./Attack/Ranged/MaxRange") != None):
			unit['Range'] = Template.find("./Attack/Ranged/MaxRange").text
		if (Template.find("./Attack/Ranged/RepeatTime") != None):
			unit['RepeatRate']["Ranged"] = Template.find("./Attack/Ranged/RepeatTime").text
		if (Template.find("./Attack/Ranged/PrepareTime") != None):
			unit['PrepRate']["Ranged"] = Template.find("./Attack/Ranged/PrepareTime").text
		for atttype in AttackTypes:
			if (Template.find("./Attack/Ranged/"+atttype) != None):
				unit['Attack']['Ranged'][atttype] = Template.find("./Attack/Ranged/"+atttype).text
		if (Template.find("./Attack/Ranged/Bonuses") != None):
			for Bonus in Template.find("./Attack/Ranged/Bonuses"):
				Against = []
				CivAg = []
				if (Bonus.find("Classes") != None and Bonus.find("Classes").text != None):
					Against = Bonus.find("Classes").text.split(" ")
				if (Bonus.find("Civ") != None and Bonus.find("Civ").text != None):
					CivAg = Bonus.find("Civ").text.split(" ")
				Val = float(Bonus.find("Multiplier").text)
				unit["AttackBonuses"][Bonus.tag] = {"Classes" : Against, "Civs" : CivAg, "Multiplier" : Val}
		if (Template.find("./Attack/Melee/RestrictedClasses") != None):
			newClasses = Template.find("./Attack/Melee/RestrictedClasses").text.split(" ")
			for elem in newClasses:
				if (elem.find("-") != -1):
					newClasses.pop(newClasses.index(elem))
					if elem in unit["Restricted"]:
						unit["Restricted"].pop(newClasses.index(elem))
			unit["Restricted"] += newClasses

	if (Template.find("./Armour") != None):
		for atttype in AttackTypes:
			if (Template.find("./Armour/"+atttype) != None):
				unit['Armour'][atttype] = Template.find("./Armour/"+atttype).text

	if (Template.find("./UnitMotion") != None):
		if (Template.find("./UnitMotion/WalkSpeed") != None):
				unit['WalkSpeed'] = Template.find("./UnitMotion/WalkSpeed").text

	if (Template.find("./Identity/VisibleClasses") != None):
		newClasses = Template.find("./Identity/VisibleClasses").text.split(" ")
		for elem in newClasses:
			if (elem.find("-") != -1):
				newClasses.pop(newClasses.index(elem))
				if elem in unit["Classes"]:
					unit["Classes"].pop(newClasses.index(elem))
		unit["Classes"] += newClasses
	
	if (Template.find("./Identity/Classes") != None):
		newClasses = Template.find("./Identity/Classes").text.split(" ")
		for elem in newClasses:
			if (elem.find("-") != -1):
				newClasses.pop(newClasses.index(elem))
				if elem in unit["Classes"]:
					unit["Classes"].pop(newClasses.index(elem))
		unit["Classes"] += newClasses


	return unit

def WriteUnit(Name, UnitDict):
	rstr = "<tr>"

	rstr += "<td style=\"text-align:right;\">" + Name + "</td>\n"
	
	rstr += "<td>" + UnitDict["HP"] + "</td>\n"

	rstr += "<td>" + UnitDict["BuildTime"] + "</td>\n"

	rstr += "<td>" + UnitDict["WalkSpeed"] + "</td>\n"
	
	rstr += "<td>" + UnitDict["Cost"]["food"] + "F / " + UnitDict["Cost"]["wood"] + "W / " + UnitDict["Cost"]["stone"] + "S / " + UnitDict["Cost"]["metal"] + "M</td>\n"

	if UnitDict["Ranged"] == "True":
		rstr += "<td>" + str(UnitDict["Attack"]["Ranged"]["Hack"]) + " / " + str(UnitDict["Attack"]["Ranged"]["Pierce"]) + " / " + str(UnitDict["Attack"]["Ranged"]["Crush"]) + "</td>\n"
	else:
		rstr += "<td>" + str(UnitDict["Attack"]["Melee"]["Hack"]) + " / " + str(UnitDict["Attack"]["Melee"]["Pierce"]) + " / " + str(UnitDict["Attack"]["Melee"]["Crush"]) + "</td>\n"
	
	rstr += "<td>" + str(UnitDict["Armour"]["Hack"]) + " / " + str(UnitDict["Armour"]["Pierce"]) + " / " + str(UnitDict["Armour"]["Crush"]) + "</td>\n"

	rstr += "<td style=\"text-align:left;\">"
	for classe in UnitDict["Classes"]:
		rstr += classe + " "
	rstr += "</td>"
	
	rstr += "<td style=\"text-align:left;\">"
	for Bonus in UnitDict["AttackBonuses"]:
		rstr += "["
		for classe in UnitDict["AttackBonuses"][Bonus]["Classes"]:
			rstr += classe + " "
		rstr += ': ' + str(UnitDict["AttackBonuses"][Bonus]["Multiplier"]) + "]  "
	rstr += "</td>"

	rstr += "<td>"
	for classe in UnitDict["Restricted"]:
		rstr += classe + " "
	rstr += "</td>"

	return "</tr>" + rstr

def CalcValue(UnitDict):
	Worth = {};

	# Try to estimate how "worth it" a unit is.
	# We can't really differentiate between attack types and armor types and counters here, so values might fluctuate
	# But this can be used to quickly estimate if some units are too costly.

	Attack = "Melee"
	if (UnitDict["Ranged"] == "true"):
		Attack = "Ranged"

	DPS = 0
	for att in AttackTypes:
		DPS += float(UnitDict['Attack'][Attack][att])
	if (Attack == "Ranged"):
		DPS += float(UnitDict["Range"]) / 10
	DPS /= (float(UnitDict['RepeatRate'][Attack])+float(UnitDict['PrepRate'][Attack])+1)/1000

	Worth["Attack"] = DPS * paramDPSImp;

	if "Infantry" in UnitDict["Restricted"]:
		Worth["Attack"] /= 5.0;
	if "Cavalry" in UnitDict["Restricted"]:
		Worth["Attack"] /= 5.0;

	armorTotal = float(UnitDict['Armour']["Hack"]) + float(UnitDict['Armour']["Pierce"]) + float(UnitDict['Armour']["Crush"])/2.0

	Worth["Defence"] = int(UnitDict['HP']) / (0.9**armorTotal)/paramHPImp;

	TotalCost = 0
	for ress in paramRessImp:
		TotalCost += int(UnitDict['Cost'][ress]) * paramRessImp[ress]
	
	if (paramIncludePopCost):
		TotalCost *= (int(UnitDict['Cost']["population"])+1)/2.0

	Worth["Cost"] = TotalCost/400.0;

	Worth["Cost"] = Worth["Cost"] * int(UnitDict['BuildTime'])/12.0 * paramBuildTimeImp + (1.0-paramBuildTimeImp) * Worth["Cost"];

	# Speed makes you way less evasive, and you can chase less.
	# It's not really a cost so just decrease Attack and Defence
	Worth["Attack"] = Worth["Attack"] * float(UnitDict["WalkSpeed"])/10.0 * paramSpeedImp + (1.0-paramSpeedImp) * Worth["Attack"];
	Worth["Defence"] = Worth["Defence"] * float(UnitDict["WalkSpeed"])/10.0 * paramSpeedImp + (1.0-paramSpeedImp) * Worth["Defence"];

	Worth["Total"] = (Worth["Attack"] + Worth["Defence"])/Worth["Cost"]

	return Worth

def canAttack(Attacker, Defender):
	ARestrictions = Attacker["Restricted"]
	BClasses = Defender["Classes"]

	for Classes in ARestrictions:
		if Classes in BClasses:
			return False
	return True

def GetAttackBonus(Attacker, Defender):
	if not canAttack(Attacker, Defender):
		return 0.0
	Abonuses = Attacker["AttackBonuses"]
	BClasses = Defender["Classes"]

	Bonus = 1.0;

	for BonusTypes in Abonuses:
		found = True
		for classe in Abonuses[BonusTypes]["Classes"]:
			if classe not in BClasses:
				found = False;
		for civi in Abonuses[BonusTypes]["Civs"]:
			if Defender["Civ"] not in civi:
				found = False;
		if found:
			Bonus *= Abonuses[BonusTypes]["Multiplier"]

	return Bonus;

# Returns how much stronger unit A is compared to unit B in a direct fight. Uses counters, speed. Not perfect, but not widely inaccurate either.
def Compare2(UnitDictA,UnitDictB):
	AWorth = 1.0;
	BWorth = 1.0;

	output = 1.0;

	# Get some DPS up in here. Use only melee or Ranged if available.
	AAttack = "Melee"
	if (UnitDictA["Ranged"] == "true"):
		AAttack = "Ranged"
	BAttack = "Melee"
	if (UnitDictB["Ranged"] == "true"):
		BAttack = "Ranged"

	ADPS = 0.0
	BDPS = 0.0
	# Exponential armor
	for att in AttackTypes:
		ADPS += float(UnitDictA['Attack'][AAttack][att]) * 0.9**float(UnitDictB['Armour'][att])
		BDPS += float(UnitDictB['Attack'][BAttack][att]) * 0.9**float(UnitDictA['Armour'][att])
	#Check if A is bonused against B and vice versa
	if (paramFactorCounters == True):
		ADPS *= GetAttackBonus(UnitDictA,UnitDictB)
		BDPS *= GetAttackBonus(UnitDictB,UnitDictA)

	if (AAttack == "Ranged"):
		ADPS += float(UnitDictA["Range"]) / float(UnitDictB["WalkSpeed"])
	if (BAttack == "Ranged"):
		BDPS += float(UnitDictB["Range"]) / float(UnitDictA["WalkSpeed"])

	ADPS /= (float(UnitDictA['RepeatRate'][AAttack])+float(UnitDictA['PrepRate'][AAttack])+1)/1000
	BDPS /= (float(UnitDictB['RepeatRate'][BAttack])+float(UnitDictB['PrepRate'][BAttack])+1)/1000

	#Ranged units can get a real advantage against slower enemy units.
	if AAttack == "Ranged" and BAttack != "Ranged" and float(UnitDictA["WalkSpeed"]) - paramMicroAutoSpeed > float(UnitDictB["WalkSpeed"]):
		ADPS += 20
	elif BAttack == "Ranged" and AAttack != "Ranged" and float(UnitDictB["WalkSpeed"]) - paramMicroAutoSpeed > float(UnitDictA["WalkSpeed"]):
		BDPS += 20

	if AAttack == "Ranged" and BAttack != "Ranged" and float(UnitDictA["WalkSpeed"]) - paramMicroPerfectSpeed > float(UnitDictB["WalkSpeed"]):
		ADPS += 5000
	elif BAttack == "Ranged" and AAttack != "Ranged" and float(UnitDictB["WalkSpeed"]) - paramMicroPerfectSpeed > float(UnitDictA["WalkSpeed"]):
		BDPS += 5000

	AWorth += int(UnitDictA['HP']) / (BDPS+1);
	BWorth += int(UnitDictB['HP']) / (ADPS+1);

	SpeedRatio = 1.0-paramSpeedImp + paramSpeedImp * float(UnitDictA["WalkSpeed"]) / float(UnitDictB["WalkSpeed"])

	return AWorth / BWorth * SpeedRatio

# Sort the templates dictionary.
def SortFn(A):
	sortVal = 0
	for classe in SortTypes:
		sortVal += 1
		if classe in A[1]["Classes"]:
			break
	if (ComparativeSortByChamp == True and A[0].find("champion") == -1):
		sortVal -= 20
	if (ComparativeSortByCav == True and A[0].find("cavalry") == -1):
		sortVal -= 10
	if (A[1]["Civ"] != None):
		sortVal += 100 * Civs.index(A[1]["Civ"])
	return sortVal

# Output a matrix of each units against one another, where the square is greener if the unit is stronger, redder if weaker.
def WriteComparisonTable(Units, OtherUnits = None):
	f.write("<table class=\"ComparisonTable\">")

	if OtherUnits == None:
		OtherUnits = Units

	sortedDict = sorted(Units.items(), key=SortFn)
	sortedOtherDict = sorted(OtherUnits.items(), key=SortFn)

	# First column: write them all
	f.write ("<tr class=\"vertical-text\">")
	f.write("<td></td>")
	for unitName in sortedOtherDict:
		f.write("<th>" + unitName[0] + "</th>")
	f.write("<td class=\"Separator\"></td>")
	for unitName in sortedOtherDict:
		f.write("<th>" + unitName[0] + "</th>")	
	f.write("</tr>")

	# Write the comparisons
	for unitName in sortedDict:
		f.write("<tr>\n<th>" + unitName[0] + "</th>")
		
		for otherUnitName in sortedOtherDict:
			outcome = Compare2(unitName[1],otherUnitName[1])
			green = max(0.0, min(4.0,outcome))
			if (green > 1.0):
				f.write("<td style=\"background-color: rgb(" + str(int(85*(4.0-green))) + ",255,0)\"></td>")
			else:
				f.write("<td style=\"background-color: rgb(255," + str(int(255*green)) + ",0" + ")\"></td>")

		f.write("<td class=\"Separator\"></td>")

		for otherUnitName in sortedOtherDict:
			outcome = GetAttackBonus(unitName[1],otherUnitName[1])
			green = max(0.0, min(3.0,outcome))
			if (green > 1.0):
				f.write("<td style=\"background-color: rgb(" + str(int(126*(3.0-green))) + ",255,"  + str(int(126*(3.0-green))) + ")\"></td>")
			else:
				f.write("<td style=\"background-color: rgb(255," + str(int(255*green)) + "," + str(int(255*green)) + ")\"></td>")
		f.write("</tr>")

	f.write("</table>")


def WriteWorthList(Units):

	def SortFn(A):
		return CalcValue(A[1])["Total"]

	sortedDict = sorted(Units.items(), key=SortFn, reverse=True)

	f.write("<table class=\"WorthList\">")
	for unitName in sortedDict:
		value = CalcValue(unitName[1])
		valueAtt = int(2*value["Attack"]/value["Cost"])
		valueDef = int(2*value["Defence"]/value["Cost"])
		f.write("<tr><th>" + unitName[0] + ": </th><td><span style=\"text-align:right; float:right; background-color:#DA0000; width:" + str(30 + valueAtt/5) + "px;\">" + str(valueAtt) + "</span></td>")
		f.write("<td><span style=\"background-color:#0000DD; width:" + str(30 + valueDef/5) + "px;\">" + str(valueDef) + "</span></td>")
		f.write("</tr>")

	f.write("</table>")

def CivUnitComparisons(CivA, CivB):
	averageEffNoCost = 0
	MaxMinEffNoCost = 0
	MinMaxEffNoCost = 999999
	averageEff = 0
	MaxMinEff = 0
	MinMaxEff = 999999
	MaxMinEffUnit = ""
	MinMaxEffUnit = ""

	for theirUnit in CivB["Units"]:
		unitAverageNoCost = 0
		unitMaxNoCost = 0
		unitAverage = 0
		unitMax = 0
		for myUnit in CivA["Units"]:
			eff = Compare2(CivA["Units"][myUnit],CivB["Units"][theirUnit])
			eff = (eff * CivA["RangedUnitsWorth"] / CivB["RangedUnitsWorth"] * paramRangedCoverage) + eff * (1.0-paramRangedCoverage)
			unitAverageNoCost += eff
			if (eff > unitMaxNoCost): unitMaxNoCost = eff
			eff /= CalcValue(CivA["Units"][myUnit])["Cost"] / CalcValue(CivB["Units"][theirUnit])["Cost"]
			unitAverage += eff
			if (eff > unitMax): unitMax = eff
		unitAverageNoCost /= len(CivB["Units"])
		unitAverage /= len(CivB["Units"])
		averageEffNoCost += unitAverageNoCost
		averageEff += unitAverage
		if (unitMax < MinMaxEff):
			MinMaxEff = unitMax
			MinMaxEffUnit = theirUnit
		if (unitMaxNoCost < MinMaxEffNoCost): MinMaxEffNoCost = unitMaxNoCost

	for myUnit in CivA["Units"]:
		unitMinNoCost = 9999999
		unitMin = 9999999
		for theirUnit in CivB["Units"]:
			eff = Compare2(CivA["Units"][myUnit],CivB["Units"][theirUnit])
			eff = (eff * CivA["RangedUnitsWorth"] / CivB["RangedUnitsWorth"] * paramRangedCoverage) + eff * (1.0-paramRangedCoverage)
			if (eff < unitMinNoCost): unitMinNoCost = eff
			eff /= CalcValue(CivA["Units"][myUnit])["Cost"] / CalcValue(CivB["Units"][theirUnit])["Cost"]
			if (eff < unitMin): unitMin = eff
		if (unitMin > MaxMinEff):
			MaxMinEff = unitMin
			MaxMinEffUnit = myUnit
		if (unitMinNoCost > MaxMinEffNoCost): MaxMinEffNoCost = unitMinNoCost

	f.write("<tr><th>" + oCiv + "</th>")
	averageEffNoCost /= len(CivA["Units"])
	averageEff /= len(CivA["Units"])
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (averageEffNoCost-1) * 90)) + "," + str(int((averageEffNoCost-1) * 90)) 	+ ",0,0.7);\">" + str("%.2f" % averageEffNoCost) + "</td>")
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (MaxMinEffNoCost-0.3) * 360)) + "," + str(int((MaxMinEffNoCost-0.3) * 360))+ ",0,0.7);\">" + str("%.2f" % MaxMinEffNoCost) + "</td>")
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (MinMaxEffNoCost-1) * 130)) + "," + str(int((MinMaxEffNoCost-1) * 130)) 	+ ",0,0.7);\">" + str("%.2f" % MinMaxEffNoCost) + "</td>")
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (averageEff-0.5) * 110)) + "," + str(int((averageEff-0.5) * 110)) 			+ ",0,0.7);\">" + str("%.2f" % averageEff) + "</td>")
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (MaxMinEff-0.3) * 360)) + "," + str(int((MaxMinEff-0.3) * 360)) 			+ ",0,0.7);\">" + str("%.2f" % MaxMinEff) + "</td>")
	f.write("<td style=\"background-color:rgba(" + str(int(255 - (MinMaxEff-1) * 130)) + "," + str(int((MinMaxEff-1) * 130)) 				+ ",0,0.7);\">" + str("%.2f" % MinMaxEff) + "</td>")
	if MaxMinEff <= MinMaxEff and abs(MaxMinEff * MinMaxEff - 1.0) < 0.1:
		f.write("<td style=\"background-color:rgb(0,255,0);\">Surely</td>")
	elif MaxMinEff <= MinMaxEff and abs(MaxMinEff * MinMaxEff - 1.0) < 0.2:
		f.write("<td style=\"background-color:rgb(255,160,0);\">Probably</td>")
	elif MaxMinEff > MinMaxEff or MaxMinEff * MinMaxEff > 1.0:
		f.write("<td style=\"background-color:rgb(255,0,0);\">No, too strong</td>")
	else:
		f.write("<td style=\"background-color:rgb(255,0,0);\">No, too weak</td>")
	f.write("<td class=\"Separator\">" + str(int(CivA["RangedUnitsWorth"] / CivB["RangedUnitsWorth"]*100)) + "%</td><td class=\"UnitTd\">" + MaxMinEffUnit + "</td><td class=\"UnitTd\">" + MinMaxEffUnit + "</td></tr>")

f = open(os.path.realpath(__file__).replace("unitTables.py","") + 'unit_summary_table.html', 'w')

f.write("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n<html>\n<head>\n	<title>Unit Tables</title>\n	<link rel=\"stylesheet\" href=\"style.css\">\n</head>\n<body>")
htbout(f,"h1","Unit Summary Table")
f.write("\n")

os.chdir(basePath)

#Load values up.
templates = {};	# Values

############################################################
#Whilst loading I also write them.
htbout(f,"h2", "Units")
f.write("<table class=\"UnitList\">")
f.write("<tr><th></th><th>HP</th><th>BuildTime</th><th>Speed</th><th>Costs</th><th>Attack (H/P/C)</th><th>Armour(H/P/C)</th><th>Classes</th><th>Efficient against</th><th>Cannot Attack</th></tr>")
for clss in Class:
	for tp in Types:
		for sbt in Subtypes:
			for x in Fix:
				template = "template_unit" + clss + "_" + tp + "_" + x + sbt + ".xml"
				if (os.path.isfile(template)):
					templates[template] = CalcUnit(template)
					f.write(WriteUnit(template, templates[template]))
for Tmp in AddedTemplates:
	if (os.path.isfile(Tmp)):
		templates[Tmp] = CalcUnit(Tmp)
		f.write(WriteUnit(Tmp, templates[Tmp]))
f.write("</table>")

f.write("<h2>Unit Comparison Table</h2>\n")
f.write("<p class=\"desc\">The first table shows how strong a unit is against another one. Full green means 4 times as strong, Yellow means equal, Full red means infinitely worse.<br/>The second table shows hardcoded attack counters. Full Green is 3.0. White is 1.0. Red is 0.0.</p>")
if (not paramFactorCounters):
	f.write("<p class=\"desc\">The left graph does not account for hardcoded counters here.</p>")
WriteComparisonTable(templates)

f.write("<h2>Worthiness</h2>\n")
f.write("<p class=\"desc\">This gives a representation of how efficient a unit is cost-wise.<br/>Red bars are offensive capabilities, blue bars defensive capabilites.</p>")
f.write("<p class=\"desc\">Costs are estimated as the sum of Food*" + str(paramRessImp["food"]) + " / Wood*" + str(paramRessImp["wood"]) + " / Stone*" + str(paramRessImp["stone"]) + " / Metal*" + str(paramRessImp["metal"]) + "<br/>")
f.write("Population cost is " + ("counted" if paramIncludePopCost else "not counted") + " in the costs.<br/>")
WriteWorthList(templates)

############################################################
# Load Civ specific templates
# Will be loaded by loading buildings, and templates will be set the Civ building, the Civ and in general.
# Allows to avoid special units.
CivData = {};
CivTemplates = {};

for Civ in Civs:
	if ShowCivLists:
		htbout(f,"h2", Civ)
		f.write("<table class=\"UnitList\">")
	CivData[Civ] = { "Units" : {}, "Buildings" : {}, "RangedUnits" : {} }
	for building in CivBuildings:
		CivData[Civ]["Buildings"][building] = {}
		bdTemplate = "./structures/" + Civ + "_" + building + ".xml"
		TrainableUnits = []
		if (os.path.isfile(bdTemplate)):
			Template = ET.parse(bdTemplate)
			if (Template.find("./ProductionQueue/Entities") != None):
				TrainableUnits = Template.find("./ProductionQueue/Entities").text.replace(" ","").replace("\t","").split("\n")
		# We have the templates this building can train.
		for UnitFile in TrainableUnits:
			breakIt = False
			for filter in FilterOut:
				if UnitFile.find(filter) != -1: breakIt = True
			if breakIt: continue

			if (os.path.isfile(UnitFile + ".xml")):
				templates[UnitFile + ".xml"] = CalcUnit(UnitFile + ".xml")	# Parse
				CivData[Civ]["Buildings"][building][UnitFile + ".xml"] = templates[UnitFile + ".xml"]
				if UnitFile + ".xml" not in CivData[Civ]["Units"]:
					CivTemplates[UnitFile + ".xml"] = templates[UnitFile + ".xml"]
					CivData[Civ]["Units"][UnitFile + ".xml"] = templates[UnitFile + ".xml"]
					if ShowCivLists:
						f.write(WriteUnit(UnitFile + ".xml", templates[UnitFile + ".xml"]))
					if "Ranged" in templates[UnitFile + ".xml"]["Classes"]:
						CivData[Civ]["RangedUnits"][UnitFile + ".xml"] = templates[UnitFile + ".xml"]
	f.write("</table>")

############################################################
f.write("\n\n<h2>Units Specializations</h2>\n")
f.write("<p class=\"desc\">This table compares each template to its parent, showing the differences between the two.<br/>Note that like any table, you can copy/paste this in Excel (or Numbers or ...) and sort it.</p>")
TemplatesByParent = {}

#Get them in the array
for CivUnitTemplate in CivTemplates:
	parent = CivTemplates[CivUnitTemplate]["Parent"]
	if parent in templates and templates[parent]["Civ"] == None:
		if parent not in TemplatesByParent:
			TemplatesByParent[parent] = []
		TemplatesByParent[parent].append( (CivUnitTemplate,CivTemplates[CivUnitTemplate]))

#Sort them by civ and write them in a table.
f.write("<table class=\"TemplateParentComp\">\n")
f.write("<tr class=\"Label\"><td></td><td></td><td style=\"border-left:1px black solid;\">HP</td><td style=\"border-left:1px black solid;\">BuildTime</td><td style=\"border-left:1px black solid;\">Speed(walk)</td><td colspan=\"3\" style=\"border-left:1px black solid;\">Armour</td><td colspan=\"3\" style=\"border-left:1px black solid;\">Attack</td><td colspan=\"5\" style=\"border-left:1px black solid;\">Costs</td><td style=\"border-left:1px black solid;\">Civ</td><td style=\"border-left:1px black solid;\">Worth</td></tr>\n")
f.write("<tr class=\"Label\" style=\"border-bottom:1px black solid;\"><td></td><td></td><td style=\"border-left:1px black solid;\"></td><td style=\"border-left:1px black solid;\"></td><td style=\"border-left:1px black solid;\"></td><td style=\"border-left:1px black solid;\">H</td><td>P</td><td>C</td><td style=\"border-left:1px black solid;\">H</td><td>P</td><td>C</td><td style=\"border-left:1px black solid;\">F</td><td>W</td><td>S</td><td>M</td><td>P</td><td style=\"border-left:1px black solid;\"></td><td style=\"border-left:1px black solid;\"></td></tr>\n<tr>")
for parent in TemplatesByParent:
	TemplatesByParent[parent].sort(key=lambda x : Civs.index(x[1]["Civ"]))
	f.write("<th rowspan=\"" + str(len(TemplatesByParent[parent])) + "\">" + parent + "</th>")
	for tp in TemplatesByParent[parent]:
		f.write("<td class=\"Sub\">" + tp[0] + "</td>")
		diff = int(tp[1]["HP"]) - int(templates[parent]["HP"])
		f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")
		diff = int(tp[1]["BuildTime"]) - int(templates[parent]["BuildTime"])
		f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff > 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")

		diff = float(tp[1]["WalkSpeed"]) - float(templates[parent]["WalkSpeed"])
		f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0 else "0,150,0")) + ");\">" + str("%.1f" % diff) + "</span></td>")

		for atype in AttackTypes:
			diff = float(tp[1]["Armour"][atype]) - float(templates[parent]["Armour"][atype])
			f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")

		attType = ("Ranged" if tp[1]["Ranged"] else "Melee")
		for atype in AttackTypes:
			diff = float(tp[1]["Attack"][attType][atype]) - float(templates[parent]["Attack"][attType][atype])
			f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")

		for rtype in Resources:
			diff = float(tp[1]["Cost"][rtype]) - float(templates[parent]["Cost"][rtype])
			f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff > 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")
		
		diff = float(tp[1]["Cost"]["population"]) - float(templates[parent]["Cost"]["population"])
		f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff > 0 else "0,150,0")) + ");\">" + str(int(diff)) + "</span></td>")

		f.write("<td>" + tp[1]["Civ"] + "</td>")

		diff = float(CalcValue(tp[1])["Total"]) / float(CalcValue(templates[parent])["Total"])-1.0
		f.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0.0 else "0,150,0")) + ");\">" + str(int(100*diff)) + "%</span></td>")

		f.write("</tr>\n<tr>")
f.write("<table/>")

# Table of unit having or not having some units.
f.write("\n\n<h2>Roster Variety</h2>\n")
f.write("<p class=\"desc\">This table show which civilizations have units who derive from each loaded generic template.<br/>Green means 1 deriving unit, blue means 2, black means 3 or more.<br/>The total is the total number of loaded units for this civ, which may be more than the total of units inheriting from loaded templates.</p>")
f.write("<table class=\"CivRosterVariety\">\n")
f.write("<tr><th>Template</th>\n")
for civ in Civs:
	f.write("<td class=\"vertical-text\">" + civ + "</td>\n")
f.write("</tr>\n")

sortedDict = sorted(templates.items(), key=SortFn)

for tp in sortedDict:
	if tp[0] not in TemplatesByParent:
		continue
	f.write("<tr><th>" + tp[0] +"</th>\n")
	for civ in Civs:
		found = 0
		for temp in TemplatesByParent[tp[0]]:
			if temp[1]["Civ"] == civ:
				found += 1
		if found == 1:
			f.write("<td style=\"background-color:rgb(0,230,0);\"></td>")
		elif found == 2:
			f.write("<td style=\"background-color:rgb(0,0,200);\"></td>")
		elif found >= 3:
			f.write("<td style=\"background-color:rgb(0,0,0);\"></td>")
		else:
			f.write("<td style=\"background-color:rgb(235,0,0);\"></td>")
	f.write("</tr>\n")
f.write("<tr style=\"margin-top:2px;border-top:2px #aaa solid;\"><th style=\"text-align:right; padding-right:10px;\">Total:</th>\n")
for civ in Civs:
	f.write("<td style=\"text-align:center;\">" + str(len(CivData[civ]["Units"])) + "</td>\n")
f.write("</tr>\n")
f.write("<table/>")

############################################################
# Writing Civ Specific Comparisons.
f.write("<h2>Civilisation Comparisons</h2>\n")

f.write("<p class=\"desc\">The following graphs attempt to do some analysis of civilizations against each other. ")
f.write("Different kinds of data: A matrix of units efficiency in a 1v1 fight, a graph of unit worthiness (power/cost), and a statistical analysis of civilizations against each other:<br/>")
f.write("In this last one in particular, \"Minimal Maximal Efficiency\" is low if there is a unit this civ cannot counter, and \"Maximal Minimal Efficiency\" is high if there is a unit that cannot be countered. If the (minimal) maximal efficiency and the average maximal efficiency are close, this means there are several units for that civ that counter the enemy civ's unit.<br/>The Balance column analyses those two to give some verdict (consider the average effectiveness too though)<br/><br/>Note that \"Counter\" in this data means \"Any unit that is strong against another unit\", so a unit that's simply way stronger than another counts as \"countering it\".<br/>If a civ's average efficiency and average efficiency disregarding costs are very different, this can mean that its costly units are particularly strong (Mauryans fit the bill with elephants).</p>")

for Civ in Civs:
	CivData[Civ]["RangedUnitsWorth"] = 0
	for unit in CivData[Civ]["RangedUnits"]:
		value = CalcValue(CivData[Civ]["RangedUnits"][unit])["Total"]
		if paramRangedMode == "Average": CivData[Civ]["RangedUnitsWorth"] += value
		elif value > CivData[Civ]["RangedUnitsWorth"]: CivData[Civ]["RangedUnitsWorth"] = value
	if (paramRangedMode == "Average" and len(CivData[Civ]["RangedUnits"]) > 0):
		CivData[Civ]["RangedUnitsWorth"] /= len(CivData[Civ]["RangedUnits"])

for Civ in Civs:
	f.write("<h3>" + Civ + "</h3>")

	z = {}
	for oCiv in Civs:
		if (oCiv != Civ):
			z.update(CivData[oCiv]["Units"])
	WriteComparisonTable(CivData[Civ]["Units"],z)

	WriteWorthList(CivData[Civ]["Units"])

	f.write("<p>This Civilization has " + str(len(CivData[Civ]["RangedUnits"])) + " ranged units for a worth of " + str(CivData[Civ]["RangedUnitsWorth"]) + " (method " + ("Average" if paramRangedMode == "Average" else "Max") + "), individually:<br/>")
	for unit in CivData[Civ]["RangedUnits"]:
		value = CalcValue(CivData[Civ]["RangedUnits"][unit])["Total"]
		f.write(unit + " : " + str(value) + "<br/>")

	f.write("<p class=\"desc\"><br/>The following table uses the value in the comparison matrix above, factoring in costs (on the 3 columns on the right), and the average efficiency of its ranged units (per ParamRangedCoverage - " + str(paramRangedCoverage) + " right now).</p>")
	f.write("<table class=\"AverageComp\">")
	f.write("<tr><td></td><td>Average Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Maximal Minimal Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Minimal Maximal Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Average Efficiency</td><td>Maximal Minimal Efficiency</td><td>Minimal Maximal Efficiency</td><td>Balance</td><td>Ranged Efficiency</td><td>Countered the least</td><td>Counters the least</td></tr>")
	for oCiv in Civs:
		if (oCiv == Civ):
			continue
		CivUnitComparisons(CivData[Civ],CivData[oCiv])
	f.write("</table>")

f.write("</body>\n</html>")
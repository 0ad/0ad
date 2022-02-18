#!/usr/bin/env python3
# -*- mode: python-mode; python-indent-offset: 4; -*-
#
# Copyright (C) 2022 Wildfire Games.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.



import xml.etree.ElementTree as ET
import os
import glob

# TODO: Add other damage types.
AttackTypes = ["Hack", "Pierce", "Crush"]
Resources = ["food", "wood", "stone", "metal"]

# Generic templates to load
# The way this works is it tries all generic templates
# But only loads those who have one of the following parents
# EG adding "template_unit.xml" will load all units.
LoadTemplatesIfParent = [
    "template_unit_infantry.xml",
    "template_unit_cavalry.xml",
    "template_unit_champion.xml",
    "template_unit_hero.xml",
]

# Those describe Civs to analyze.
# The script will load all entities that derive (to the nth degree) from one of
# the above templates.
Civs = [
    "athen",
    "brit",
    "cart",
    "gaul",
    "iber",
    "kush",
    "mace",
    "maur",
    "pers",
    "ptol",
    "rome",
    "sele",
    "spart",
    # "gaia",
]

# Remote Civ templates with those strings in their name.
FilterOut = ["marian", "thureophoros", "thorakites", "kardakes"]

# In the Civilization specific units table, do you want to only show the units
# that are different from the generic templates?
showChangedOnly = True

# Sorting parameters for the "roster variety" table
ComparativeSortByCav = True
ComparativeSortByChamp = True
ClassesUsedForSort = [
    "Support",
    "Pike",
    "Spear",
    "Sword",
    "Archer",
    "Javelin",
    "Sling",
    "Elephant",
]

# Disable if you want the more compact basic data. Enable to allow filtering and
# sorting in-place.
AddSortingOverlay = True

# This is the path to the /templates/ folder to consider. Change this for mod
# support.
basePath = (
    os.path.realpath(__file__).replace("unitTables.py", "")
    + "../../../binaries/data/mods/public/simulation/templates/"
)

# For performance purposes, cache opened templates files.
globalTemplatesList = {}


def showChild(root):
    """root is of ElementTree.getroot() type"""
    # Not used; use it for debugging
    print("----------------  Children:  ----------------")
    [print(child.tag, child.attrib) for child in root]

    print("\n--------------  Neighbours:  --------------")
    [print(neighbor.attrib) for neighbor in root.iter("neighbor")]


def htbout(file, balise, value):
    file.write("<" + balise + ">" + value + "</" + balise + ">\n")


def htout(file, value):
    file.write("<p>" + value + "</p>\n")


def fastParse(templateName):
    """Run ET.parse() with memoising in a global table."""
    if templateName in globalTemplatesList:
        return globalTemplatesList[templateName]
    globalTemplatesList[templateName] = ET.parse(templateName)
    return globalTemplatesList[templateName]


# This function checks that a template has the given parent.
def hasParentTemplate(UnitName, parentName):
    Template = fastParse(UnitName)

    found = False
    Name = UnitName

    # parent_str = 'parent '
    while found != True and Template.getroot().get("parent") != None:
        longName = Template.getroot().get("parent") + ".xml"
        # 0ad started using unit class/category prefixed to the unit name
        # separated by |, known as mixins since A25 (rP25223)
        #
        # We strip these categories for now. The | syntax
        # gives a unit its "category" (like merc_cav, merc_inf, hoplite,
        # builder, shrine, civ/athen). This can be used later for
        # classification

        Name = longName.split("|")[-1]

        mixins = {x.replace('civ/', '') for x in longName.split("|")[0:-1]}
        civ = set(Civs).intersection(mixins)
        if len(civ) > 0:
            # mixin category contains a civ name
            # we honor mixin civ hierarchy
            # This assumes a unit only belongs to a single civ parent
            unit_civ = list(civ)[0] + '.xml'
            # unit_civ is not used in this function for now

        if Name == parentName:
            return True

        Template = ET.parse(Name)

        # parent_str += 'parent '

    return False


def NumericStatProcess(unitValue, templateValue):
    val = float(templateValue.text)
    if not "op" in templateValue.attrib:
        return val
    if templateValue.attrib["op"] == "add":
        unitValue += val
    elif templateValue.attrib["op"] == "sub":
        unitValue -= val
    elif templateValue.attrib["op"] == "mul":
        unitValue *= val
    elif templateValue.attrib["op"] == "mul_round":
        unitValue = round(unitValue * val)
    elif templateValue.attrib["op"] == "div":
        unitValue /= val
    return unitValue


def CalcUnit(UnitName, existingUnit=None):
    """Parse the entity values recursively through fastParse()."""
    unit = {
        "HP": "0",
        "BuildTime": "0",
        "Cost": {
            "food": "0",
            "wood": "0",
            "stone": "0",
            "metal": "0",
            "population": "0",
        },
        "Attack": {
            "Melee": {"Hack": 0, "Pierce": 0, "Crush": 0},
            "Ranged": {"Hack": 0, "Pierce": 0, "Crush": 0},
        },
        "RepeatRate": {"Melee": "0", "Ranged": "0"},
        "PrepRate": {"Melee": "0", "Ranged": "0"},
        "Resistance": {"Hack": 0, "Pierce": 0, "Crush": 0},
        "Ranged": False,
        "Classes": [],
        "AttackBonuses": {},
        "Restricted": [],
        "WalkSpeed": 0,
        "Range": 0,
        "Spread": 0,
        "Civ": None,
    }

    if existingUnit != None:
        unit = existingUnit

    Template = fastParse(UnitName)

    # Recursively get data from our parent which we'll override.
    unit_civ = None
    if Template.getroot().get("parent") != None:
        # 0ad started using unit class/category prefixed to the unit name
        # separated by |, known as mixins since A25 (rP25223)
        # We strip these categories for now
        # This can be used later for classification
        longName = Template.getroot().get("parent")
        Name = longName.split("|")[-1] + ".xml"

        mixins = {x.replace('civ/', '') for x in longName.split("|")[0:-1]}
        civ = set(Civs).intersection(mixins)
        if len(civ) > 0:
            # mixin category contains a civ name
            # we honor mixin civ hierarchy
            # This assumes a unit only belongs to a single civ parent
            unit_civ = list(civ)[0]


        unit = CalcUnit(Name, unit)
        unit["Parent"] = Name

    if unit_civ:
        unit["Civ"] = unit_civ
    elif Template.find("./Identity/Civ") != None:
        unit["Civ"] = Template.find("./Identity/Civ").text



    if Template.find("./Health/Max") != None:
        unit["HP"] = NumericStatProcess(
            unit["HP"], Template.find("./Health/Max"))

    if Template.find("./Cost/BuildTime") != None:
        unit["BuildTime"] = NumericStatProcess(
            unit["BuildTime"], Template.find("./Cost/BuildTime")
        )

    if Template.find("./Cost/Resources") != None:
        for type in list(Template.find("./Cost/Resources")):
            unit["Cost"][type.tag] = NumericStatProcess(
                unit["Cost"][type.tag], type)

    if Template.find("./Cost/Population") != None:
        unit["Cost"]["population"] = NumericStatProcess(
            unit["Cost"]["population"], Template.find("./Cost/Population")
        )

    if Template.find("./Attack/Melee") != None:
        if Template.find("./Attack/Melee/RepeatTime") != None:
            unit["RepeatRate"]["Melee"] = NumericStatProcess(
                unit["RepeatRate"]["Melee"],
                Template.find("./Attack/Melee/RepeatTime")
            )
        if Template.find("./Attack/Melee/PrepareTime") != None:
            unit["PrepRate"]["Melee"] = NumericStatProcess(
                unit["PrepRate"]["Melee"],
                Template.find("./Attack/Melee/PrepareTime")
            )
        for atttype in AttackTypes:
            if Template.find("./Attack/Melee/Damage/" + atttype) != None:
                unit["Attack"]["Melee"][atttype] = NumericStatProcess(
                    unit["Attack"]["Melee"][atttype],
                    Template.find("./Attack/Melee/Damage/" + atttype),
                )
        if Template.find("./Attack/Melee/Bonuses") != None:
            for Bonus in Template.find("./Attack/Melee/Bonuses"):
                Against = []
                CivAg = []
                if Bonus.find("Classes") != None \
                   and Bonus.find("Classes").text != None:
                    Against = Bonus.find("Classes").text.split(" ")
                if Bonus.find("Civ") != None and Bonus.find("Civ").text != None:
                    CivAg = Bonus.find("Civ").text.split(" ")
                Val = float(Bonus.find("Multiplier").text)
                unit["AttackBonuses"][Bonus.tag] = {
                    "Classes": Against,
                    "Civs": CivAg,
                    "Multiplier": Val,
                }
        if Template.find("./Attack/Melee/RestrictedClasses") != None:
            newClasses = Template.find("./Attack/Melee/RestrictedClasses")\
                                 .text.split(" ")
            for elem in newClasses:
                if elem.find("-") != -1:
                    newClasses.pop(newClasses.index(elem))
                    if elem in unit["Restricted"]:
                        unit["Restricted"].pop(newClasses.index(elem))
            unit["Restricted"] += newClasses

    if Template.find("./Attack/Ranged") != None:
        unit["Ranged"] = True
        if Template.find("./Attack/Ranged/MaxRange") != None:
            unit["Range"] = NumericStatProcess(
                unit["Range"], Template.find("./Attack/Ranged/MaxRange")
            )
        if Template.find("./Attack/Ranged/Spread") != None:
            unit["Spread"] = NumericStatProcess(
                unit["Spread"], Template.find("./Attack/Ranged/Spread")
            )
        if Template.find("./Attack/Ranged/RepeatTime") != None:
            unit["RepeatRate"]["Ranged"] = NumericStatProcess(
                unit["RepeatRate"]["Ranged"],
                Template.find("./Attack/Ranged/RepeatTime"),
            )
        if Template.find("./Attack/Ranged/PrepareTime") != None:
            unit["PrepRate"]["Ranged"] = NumericStatProcess(
                unit["PrepRate"]["Ranged"],
                Template.find("./Attack/Ranged/PrepareTime")
            )
        for atttype in AttackTypes:
            if Template.find("./Attack/Ranged/Damage/" + atttype) != None:
                unit["Attack"]["Ranged"][atttype] = NumericStatProcess(
                    unit["Attack"]["Ranged"][atttype],
                    Template.find("./Attack/Ranged/Damage/" + atttype),
                )
        if Template.find("./Attack/Ranged/Bonuses") != None:
            for Bonus in Template.find("./Attack/Ranged/Bonuses"):
                Against = []
                CivAg = []
                if Bonus.find("Classes") != None \
                   and Bonus.find("Classes").text != None:
                    Against = Bonus.find("Classes").text.split(" ")
                if Bonus.find("Civ") != None and Bonus.find("Civ").text != None:
                    CivAg = Bonus.find("Civ").text.split(" ")
                Val = float(Bonus.find("Multiplier").text)
                unit["AttackBonuses"][Bonus.tag] = {
                    "Classes": Against,
                    "Civs": CivAg,
                    "Multiplier": Val,
                }
        if Template.find("./Attack/Melee/RestrictedClasses") != None:
            newClasses = Template.find("./Attack/Melee/RestrictedClasses")\
                                 .text.split(" ")
            for elem in newClasses:
                if elem.find("-") != -1:
                    newClasses.pop(newClasses.index(elem))
                    if elem in unit["Restricted"]:
                        unit["Restricted"].pop(newClasses.index(elem))
            unit["Restricted"] += newClasses

    if Template.find("Resistance") != None:
        # Resistance lives insdie a new node Entity, e.g.
        # list(ET.parse('template_unit_cavalry.xml').find('Resistance/Entity/Damage'))

        for atttype in AttackTypes:
            extracted_resistance = Template.find(
                "./Resistance/Entity/Damage/" + atttype
            )
            if extracted_resistance != None:
                unit["Resistance"][atttype] = NumericStatProcess(
                    unit["Resistance"][atttype], extracted_resistance
                )

    if Template.find("./UnitMotion") != None:
        if Template.find("./UnitMotion/WalkSpeed") != None:
            unit["WalkSpeed"] = NumericStatProcess(
                unit["WalkSpeed"], Template.find("./UnitMotion/WalkSpeed")
            )

    if Template.find("./Identity/VisibleClasses") != None:
        newClasses = Template.find("./Identity/VisibleClasses").text.split(" ")
        for elem in newClasses:
            if elem.find("-") != -1:
                newClasses.pop(newClasses.index(elem))
                if elem in unit["Classes"]:
                    unit["Classes"].pop(newClasses.index(elem))
        unit["Classes"] += newClasses

    if Template.find("./Identity/Classes") != None:
        newClasses = Template.find("./Identity/Classes").text.split(" ")
        for elem in newClasses:
            if elem.find("-") != -1:
                newClasses.pop(newClasses.index(elem))
                if elem in unit["Classes"]:
                    unit["Classes"].pop(newClasses.index(elem))
        unit["Classes"] += newClasses

    return unit


def WriteUnit(Name, UnitDict):
    ret = "<tr>"
    ret += '<td class="Sub">' + Name + "</td>"
    ret += "<td>" + str(int(UnitDict["HP"])) + "</td>"
    ret += "<td>" + str("%.0f" % float(UnitDict["BuildTime"])) + "</td>"
    ret += "<td>" + str("%.1f" % float(UnitDict["WalkSpeed"])) + "</td>"

    for atype in AttackTypes:
        PercentValue = 1.0 - (0.9 ** float(UnitDict["Resistance"][atype]))
        ret += (
            "<td>"
            + str("%.0f" % float(UnitDict["Resistance"][atype]))
            + " / "
            + str("%.0f" % (PercentValue * 100.0))
            + "%</td>"
        )

    attType = "Ranged" if UnitDict["Ranged"] == True else "Melee"
    if UnitDict["RepeatRate"][attType] != "0":
        for atype in AttackTypes:
            repeatTime = float(UnitDict["RepeatRate"][attType]) / 1000.0
            ret += (
                "<td>"
                + str("%.1f" % (
                    float(UnitDict["Attack"][attType][atype]) / repeatTime
                )) + "</td>"
            )

        ret += (
            "<td>"
            + str("%.1f" % (float(UnitDict["RepeatRate"][attType]) / 1000.0))
            + "</td>"
        )
    else:
        for atype in AttackTypes:
            ret += "<td> - </td>"
        ret += "<td> - </td>"

    if UnitDict["Ranged"] == True and UnitDict["Range"] > 0:
        ret += "<td>" + str("%.1f" % float(UnitDict["Range"])) + "</td>"
        spread = float(UnitDict["Spread"])
        ret += "<td>" + str("%.1f" % spread) + "</td>"
    else:
        ret += "<td> - </td><td> - </td>"

    for rtype in Resources:
        ret += "<td>" + str("%.0f" %
                            float(UnitDict["Cost"][rtype])) + "</td>"

    ret += "<td>" + str("%.0f" %
                        float(UnitDict["Cost"]["population"])) + "</td>"

    ret += '<td style="text-align:left;">'
    for Bonus in UnitDict["AttackBonuses"]:
        ret += "["
        for classe in UnitDict["AttackBonuses"][Bonus]["Classes"]:
            ret += classe + " "
        ret += ": %s]  " % UnitDict["AttackBonuses"][Bonus]["Multiplier"]
    ret += "</td>"

    ret += "</tr>\n"
    return ret


# Sort the templates dictionary.
def SortFn(A):
    sortVal = 0
    for classe in ClassesUsedForSort:
        sortVal += 1
        if classe in A[1]["Classes"]:
            break
    if ComparativeSortByChamp == True and A[0].find("champion") == -1:
        sortVal -= 20
    if ComparativeSortByCav == True and A[0].find("cavalry") == -1:
        sortVal -= 10
    if A[1]["Civ"] != None and A[1]["Civ"] in Civs:
        sortVal += 100 * Civs.index(A[1]["Civ"])
    return sortVal


def WriteColouredDiff(file, diff, isChanged):
    """helper to write coloured text.
    diff value must always be computed as a unit_spec - unit_generic.
    A positive imaginary part represents advantageous trait.
    """

    def cleverParse(diff):
        if float(diff) - int(diff) < 0.001:
            return str(int(diff))
        else:
            return str("%.1f" % float(diff))

    isAdvantageous = diff.imag > 0
    diff = diff.real
    if diff != 0:
        isChanged = True
    else:
        # do not change its value if one parameter is not changed (yet)
        # some other parameter might be different
        pass

    if diff == 0:
        rgb_str = "200,200,200"
    elif isAdvantageous and diff > 0:
        rgb_str = "180,0,0"
    elif (not isAdvantageous) and diff < 0:
        rgb_str = "180,0,0"
    else:
        rgb_str = "0,150,0"

    file.write(
        """<td><span style="color:rgb({});">{}</span></td>
        """.format(
            rgb_str, cleverParse(diff)
        )
    )
    return isChanged


def computeUnitEfficiencyDiff(TemplatesByParent, Civs):
    efficiency_table = {}
    for parent in TemplatesByParent:
        TemplatesByParent[parent].sort(key=lambda x: Civs.index(x[1]["Civ"]))

        for tp in TemplatesByParent[parent]:
            # HP
            diff = -1j + (int(tp[1]["HP"]) - int(templates[parent]["HP"]))
            efficiency_table[(parent, tp[0], "HP")] = diff
            efficiency_table[(parent, tp[0], "HP")] = diff

            # Build Time
            diff = +1j + (int(tp[1]["BuildTime"]) -
                          int(templates[parent]["BuildTime"]))
            efficiency_table[(parent, tp[0], "BuildTime")] = diff

            # walk speed
            diff = -1j + (
                float(tp[1]["WalkSpeed"]) -
                float(templates[parent]["WalkSpeed"])
            )
            efficiency_table[(parent, tp[0], "WalkSpeed")] = diff

            # Resistance
            for atype in AttackTypes:
                diff = -1j + (
                    float(tp[1]["Resistance"][atype])
                    - float(templates[parent]["Resistance"][atype])
                )
                efficiency_table[(parent, tp[0], "Resistance/" + atype)] = diff

            # Attack types (DPS) and rate.
            attType = "Ranged" if tp[1]["Ranged"] == True else "Melee"
            if tp[1]["RepeatRate"][attType] != "0":
                for atype in AttackTypes:
                    myDPS = float(tp[1]["Attack"][attType][atype]) / (
                        float(tp[1]["RepeatRate"][attType]) / 1000.0
                    )
                    parentDPS = float(
                        templates[parent]["Attack"][attType][atype]) / (
                        float(templates[parent]["RepeatRate"][attType]) / 1000.0
                    )
                    diff = -1j + (myDPS - parentDPS)
                    efficiency_table[
                        (parent, tp[0], "Attack/" + attType + "/" + atype)
                    ] = diff
                diff = -1j + (
                    float(tp[1]["RepeatRate"][attType]) / 1000.0
                    - float(templates[parent]["RepeatRate"][attType]) / 1000.0
                )
                efficiency_table[
                    (parent, tp[0], "Attack/" + attType + "/" + atype +
                     "/RepeatRate")
                ] = diff
                # range and spread
                if tp[1]["Ranged"] == True:
                    diff = -1j + (
                        float(tp[1]["Range"]) -
                        float(templates[parent]["Range"])
                    )
                    efficiency_table[
                        (parent, tp[0], "Attack/" + attType + "/Ranged/Range")
                    ] = diff

                    diff = (float(tp[1]["Spread"]) -
                            float(templates[parent]["Spread"]))
                    efficiency_table[
                        (parent, tp[0], "Attack/" + attType + "/Ranged/Spread")
                    ] = diff

            for rtype in Resources:
                diff = +1j + (
                    float(tp[1]["Cost"][rtype])
                    - float(templates[parent]["Cost"][rtype])
                )
                efficiency_table[(parent, tp[0], "Resources/" + rtype)] = diff

            diff = +1j + (
                float(tp[1]["Cost"]["population"])
                - float(templates[parent]["Cost"]["population"])
            )
            efficiency_table[(parent, tp[0], "Population")] = diff

    return efficiency_table


def computeTemplates(LoadTemplatesIfParent):
    """Loops over template XMLs and selectively insert into templates dict."""
    pwd = os.getcwd()
    os.chdir(basePath)
    templates = {}
    for template in list(glob.glob("template_*.xml")):
        if os.path.isfile(template):
            found = False
            for possParent in LoadTemplatesIfParent:
                if hasParentTemplate(template, possParent):
                    found = True
                    break
            if found == True:
                templates[template] = CalcUnit(template)
                # f.write(WriteUnit(template, templates[template]))
    os.chdir(pwd)
    return templates


def computeCivTemplates(template: dict, Civs: list):
    """Load Civ specific templates"""
    # NOTE: whether a Civ can train a certain unit is not recorded in the unit
    # .xml files, and hence we have to get that info elsewhere, e.g. from the
    # Civ tree. This should be delayed until this whole parser is based on the
    # Civ tree itself.

    # This function must always ensure that Civ unit parenthood works as
    # intended, i.e. a unit in a Civ indeed has a 'Civ' field recording its
    # loyalty to that Civ. Check this when upgrading this script to keep
    # up with the game engine.
    pwd = os.getcwd()
    os.chdir(basePath)

    CivTemplates = {}

    for Civ in Civs:
        CivTemplates[Civ] = {}
        # Load all templates that start with that civ indicator
        # TODO: consider adding mixin/civs here too
        civ_list = list(glob.glob("units/" + Civ + "/*.xml"))
        for template in civ_list:
            if os.path.isfile(template):

                # filter based on FilterOut
                breakIt = False
                for filter in FilterOut:
                    if template.find(filter) != -1:
                        breakIt = True
                if breakIt:
                    continue

                # filter based on loaded generic templates
                breakIt = True
                for possParent in LoadTemplatesIfParent:
                    if hasParentTemplate(template, possParent):
                        breakIt = False
                        break
                if breakIt:
                    continue

                unit = CalcUnit(template)

                # Remove variants for now
                if unit["Parent"].find("template_") == -1:
                    continue

                # load template
                CivTemplates[Civ][template] = unit

    os.chdir(pwd)
    return CivTemplates


def computeTemplatesByParent(templates: dict, Civs: list, CivTemplates: dict):
    """Get them in the array"""
    # Civs:list -> CivTemplates:dict -> templates:dict -> TemplatesByParent
    TemplatesByParent = {}
    for Civ in Civs:
        for CivUnitTemplate in CivTemplates[Civ]:
            parent = CivTemplates[Civ][CivUnitTemplate]["Parent"]

            # We have the following constant equality
            # templates[*]["Civ"] === gaia
            # if parent in templates and templates[parent]["Civ"] == None:
            if parent in templates:
                if parent not in TemplatesByParent:
                    TemplatesByParent[parent] = []
                TemplatesByParent[parent].append(
                    (CivUnitTemplate, CivTemplates[Civ][CivUnitTemplate])
                )

    # debug after CivTemplates are non-empty
    return TemplatesByParent


############################################################
## Pre-compute all tables
templates = computeTemplates(LoadTemplatesIfParent)
CivTemplates = computeCivTemplates(templates, Civs)
TemplatesByParent = computeTemplatesByParent(templates, Civs, CivTemplates)

# Not used; use it for your own custom analysis
efficiencyTable = computeUnitEfficiencyDiff(
    TemplatesByParent, Civs
)


############################################################
def writeHTML():
    """Create the HTML file"""
    f = open(
        os.path.realpath(__file__).replace("unitTables.py", "")
        + "unit_summary_table.html",
        "w",
    )

    f.write(
        """
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
<head>
        <title>Unit Tables</title>
        <link rel="stylesheet" href="style.css">
</head>
<body>
        """
    )
    htbout(f, "h1", "Unit Summary Table")
    f.write("\n")

    # Write generic templates
    htbout(f, "h2", "Units")
    f.write(
        """
<table id="genericTemplates">
  <thead>
    <tr>
      <th> </th> <th>HP </th> <th>BuildTime </th> <th>Speed(walk) </th>
          <th colspan="3">Resistance </th>
          <th colspan="6">Attack (DPS) </th>
          <th colspan="5">Costs </th>
          <th>Efficient Against </th>
        </tr>
    <tr class="Label" style="border-bottom:1px black solid;">
      <th> </th> <th> </th> <th> </th> <th> </th>
          <th>H </th> <th>P </th> <th>C </th>
          <th>H </th> <th>P </th> <th>C </th>
      <th>Rate </th> <th>Range </th> <th>Spread (/100m) </th>
          <th>F </th> <th>W </th> <th>S </th> <th>M </th> <th>P </th>
          <th> </th>
        </tr>
</thead>
        """
    )
    for template in templates:
        f.write(WriteUnit(template, templates[template]))
    f.write("</table>")

    # Write unit specialization
    # Sort them by civ and write them in a table.
    #
    # TODO: pre-compute the diffs then render, filtering out the non-interesting
    # ones
    #
    f.write(
        """
<h2>Units Specializations
</h2>

<p class="desc">This table compares each template to its parent, showing the
differences between the two.
  <br/>Note that like any table, you can copy/paste this in Excel (or Numbers or
  ...) and sort it.
</p>

<table id="TemplateParentComp">
  <thead>
    <tr>
      <th> </th> <th> </th> <th>HP </th> <th>BuildTime </th>
          <th>Speed (/100m) </th>
          <th colspan="3">Resistance </th>
          <th colspan="6">Attack </th>
          <th colspan="5">Costs </th>
          <th>Civ </th>
        </tr>
    <tr class="Label" style="border-bottom:1px black solid;">
      <th> </th> <th> </th> <th> </th> <th> </th> <th> </th>
          <th>H </th> <th>P </th> <th>C </th>
          <th>H </th> <th>P </th> <th>C </th>
      <th>Rate </th> <th>Range </th> <th>Spread </th>
          <th>F </th> <th>W </th> <th>S </th> <th>M </th> <th>P </th>
          <th> </th>
        </tr>
  </thead>
        """
    )
    for parent in TemplatesByParent:
        TemplatesByParent[parent].sort(key=lambda x: Civs.index(x[1]["Civ"]))
        for tp in TemplatesByParent[parent]:
            isChanged = False
            ff = open(
                os.path.realpath(__file__).replace("unitTables.py", "") +
                ".cache", "w"
            )

            ff.write("<tr>")
            ff.write(
                "<th style='font-size:10px'>"
                + parent.replace(".xml", "").replace("template_", "")
                + "</th>"
            )
            ff.write(
                '<td class="Sub">'
                + tp[0].replace(".xml", "").replace("units/", "")
                + "</td>"
            )

            # HP
            diff = -1j + (int(tp[1]["HP"]) - int(templates[parent]["HP"]))
            isChanged = WriteColouredDiff(ff, diff, isChanged)

            # Build Time
            diff = +1j + (int(tp[1]["BuildTime"]) -
                          int(templates[parent]["BuildTime"]))
            isChanged = WriteColouredDiff(ff, diff, isChanged)

            # walk speed
            diff = -1j + (
                float(tp[1]["WalkSpeed"]) -
                float(templates[parent]["WalkSpeed"])
            )
            isChanged = WriteColouredDiff(ff, diff, isChanged)

            # Resistance
            for atype in AttackTypes:
                diff = -1j + (
                    float(tp[1]["Resistance"][atype])
                    - float(templates[parent]["Resistance"][atype])
                )
                isChanged = WriteColouredDiff(ff, diff, isChanged)

            # Attack types (DPS) and rate.
            attType = "Ranged" if tp[1]["Ranged"] == True else "Melee"
            if tp[1]["RepeatRate"][attType] != "0":
                for atype in AttackTypes:
                    myDPS = float(tp[1]["Attack"][attType][atype]) / (
                        float(tp[1]["RepeatRate"][attType]) / 1000.0
                    )
                    parentDPS = float(
                        templates[parent]["Attack"][attType][atype]) / (
                        float(templates[parent]["RepeatRate"][attType]) / 1000.0
                    )
                    isChanged = WriteColouredDiff(
                        ff, -1j + (myDPS - parentDPS), isChanged
                    )
                isChanged = WriteColouredDiff(
                    ff,
                    -1j
                    + (
                        float(tp[1]["RepeatRate"][attType]) / 1000.0
                        - float(templates[parent]["RepeatRate"][attType]) / 1000.0
                    ),
                    isChanged,
                )
                # range and spread
                if tp[1]["Ranged"] == True:
                    isChanged = WriteColouredDiff(
                        ff,
                        -1j
                        + (float(tp[1]["Range"]) -
                           float(templates[parent]["Range"])),
                        isChanged,
                    )
                    mySpread = float(tp[1]["Spread"])
                    parentSpread = float(templates[parent]["Spread"])
                    isChanged = WriteColouredDiff(
                        ff, +1j + (mySpread - parentSpread), isChanged
                    )
                else:
                    ff.write("<td></td><td></td>")
            else:
                ff.write("<td></td><td></td><td></td><td></td><td></td><td></td>")

            for rtype in Resources:
                isChanged = WriteColouredDiff(
                    ff,
                    +1j
                    + (
                        float(tp[1]["Cost"][rtype])
                        - float(templates[parent]["Cost"][rtype])
                    ),
                    isChanged,
                )

            isChanged = WriteColouredDiff(
                ff,
                +1j
                + (
                    float(tp[1]["Cost"]["population"])
                    - float(templates[parent]["Cost"]["population"])
                ),
                isChanged,
            )

            ff.write("<td>" + tp[1]["Civ"] + "</td>")
            ff.write("</tr>\n")

            ff.close()  # to actually write into the file
            with open(
                os.path.realpath(__file__).replace("unitTables.py", "") +
                    ".cache", "r"
            ) as ff:
                unitStr = ff.read()

            if showChangedOnly:
                if isChanged:
                    f.write(unitStr)
            else:
                # print the full table if showChangedOnly is false
                f.write(unitStr)

    f.write("<table/>")

    # Table of unit having or not having some units.
    f.write(
        """
<h2>Roster Variety
</h2>

<p class="desc">This table show which civilizations have units who derive from
each loaded generic template.
  <br/>Grey means the civilization has no unit derived from a generic template;
  <br/>dark green means 1 derived unit, mid-tone green means 2, bright green
  means 3 or more.
  <br/>The total is the total number of loaded units for this civ, which may be
  more than the total of units inheriting from loaded templates.
</p>
<table class="CivRosterVariety">
  <tr>
    <th>Template </th>
"""
    )
    for civ in Civs:
        f.write('<td class="vertical-text">' + civ + "</td>\n")
    f.write("</tr>\n")

    sortedDict = sorted(templates.items(), key=SortFn)

    for tp in sortedDict:
        if tp[0] not in TemplatesByParent:
            continue
        f.write("<tr><td>" + tp[0] + "</td>\n")
        for civ in Civs:
            found = 0
            for temp in TemplatesByParent[tp[0]]:
                if temp[1]["Civ"] == civ:
                    found += 1
            if found == 1:
                f.write('<td style="background-color:rgb(0,90,0);"></td>')
            elif found == 2:
                f.write('<td style="background-color:rgb(0,150,0);"></td>')
            elif found >= 3:
                f.write('<td style="background-color:rgb(0,255,0);"></td>')
            else:
                f.write('<td style="background-color:rgb(200,200,200);"></td>')
        f.write("</tr>\n")
    f.write(
        '<tr style="margin-top:2px;border-top:2px #aaa solid;">\
        <th style="text-align:right; padding-right:10px;">Total:</th>\n'
    )
    for civ in Civs:
        count = 0
        for units in CivTemplates[civ]:
            count += 1
        f.write('<td style="text-align:center;">' + str(count) + "</td>\n")

    f.write("</tr>\n")

    f.write("<table/>")

    # Add a simple script to allow filtering on sorting directly in the HTML
    # page.
    if AddSortingOverlay:
        f.write(
            """
<script src="tablefilter/tablefilter.js"></script>
<script data-config>
var cast = function (val) {
console.log(val);                       if (+val != val)
                return -999999999999;
        return +val;
}


var filtersConfig = {
    base_path: "tablefilter/",
    col_0: "checklist",
    alternate_rows: true,
    rows_counter: true,
    btn_reset: true,
    loader: false,
    status_bar: false,
    mark_active_columns: true,
    highlight_keywords: true,
    col_number_format: Array(22).fill("US"),
    filters_row_index: 2,
    headers_row_index: 1,
    extensions: [
        {
            name: "sort",
            types: ["string",
                    ...Array(6).fill("us"),
                    ...Array(6).fill("mytype"),
                    ...Array(5).fill("us"),
                    "string",
                   ],
            on_sort_loaded: function (o, sort) {
                sort.addSortType("mytype", cast);
            },
        },
    ],
    col_widths: [...Array(18).fill(null), "120px"],
};

var tf = new TableFilter('genericTemplates', filtersConfig,2);
tf.init();

var secondFiltersConfig = {
    base_path: "tablefilter/",
    col_0: "checklist",
    col_19: "checklist",
    alternate_rows: true,
    rows_counter: true,
    btn_reset: true,
    loader: false,
    status_bar: false,
    mark_active_columns: true,
    highlight_keywords: true,
    col_number_format: [null, null, ...Array(17).fill("US"), null],
    filters_row_index: 2,
    headers_row_index: 1,
    extensions: [
        {
            name: "sort",
            types: ["string", "string",
                    ...Array(6).fill("us"),
                    ...Array(6).fill("typetwo"),
                    ...Array(5).fill("us"),
                    "string",
                   ],
            on_sort_loaded: function (o, sort) {
                sort.addSortType("typetwo", cast);
            },
        },
    ],
    col_widths: Array(20).fill(null),
};


var tf2 = new TableFilter('TemplateParentComp', secondFiltersConfig,2);
tf2.init();

</script>
        """
        )

    f.write("</body>\n</html>")


if __name__ == "__main__":
    writeHTML()

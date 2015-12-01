Template Analyzer.

This python tool has been written by wraitii. Its purpose is to help with unit and civ balancing by allowing quick comparison between important template data.

Run it using "python unitTables.py" or "pypy unitTables.py" if you have pypy installed.

The output will be located in an HTML file called "unit_summary_table.html" in this folder.


The script gives 3 informations:
-A comparison table of generic templates.
-A comparison table of civilization units (it shows the differences with the generic templates)
-A comparison of civilization rosters.

The script can be customized to change the units that are considered, since loading all units make sit slightly unreadable.
By default it loads all citizen soldiers and all champions.

To change this, change the "LoadTemplatesIfParent" variable.
You can also consider only some civilizations.
You may also filter some templates based on their name, if you want to remove specific templates.


The HTML page comes with a JS extension that allows to filter and sort in-place, to help with comparisons. You can disable this by disabling javascript or by changing the "AddSortingOverlay" parameter in the script.

This extension, called TableFilter, is released under the MIT license. The version I used was the one found at https://github.com/koalyptus/TableFilter/

All contents of this folder are under the MIT License.

Enjoy!
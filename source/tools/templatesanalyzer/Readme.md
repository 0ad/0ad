## Template Analyzer

This python tool has been written by wraitii and updated to 0ad A25 by hyiltiz.
Its purpose is to help with unit and civ balancing by allowing quick comparison
between important template data.

Run it using `python unitTables.py` or `pypy unitTables.py` (if you have pypy
installed). The output will be located in an HTML file called
`unit_summary_table.html` in this folder.

The script generates 3 informative tables:
- A comparison table of generic templates;
- A comparison table of civilization units (it shows the differences with the
  generic templates);
- A comparison of civilization rosters.

You can customize the script by changing the units to include (loading all units
might make it slightly unreadable). To change this, change the
`LoadTemplatesIfParent` variable. You can also consider only some civilizations.
You may also filter some templates based on their name, if you want to remove
specific templates. By default it loads all citizen soldiers and all champions,
and ignores non-interesting units for the comparison/efficienicy table (2nd
table).

The HTML page comes with a JavaScript extension that allows to filter and sort
in-place, to help with comparisons. You can disable this by disabling javascript
or by changing the `AddSortingOverlay` parameter in the script. This JS
extension, called TableFilter, is released under the MIT license. The version
used can be found at https://github.com/koalyptus/TableFilter/

All contents of this folder are under the MIT License.


## Contributing

The script intentionally only relies on Python 3 Standard Library to avoid
installing 3rd party libraries as dependencies. However, you might want to
install a few packages to make hacking around easier.

### Debugging
IPython can be used as a good REPL and to easily insert a debug break point.
Install it with:

    pip3 install ipython
    
then to insert a break point, simply insert the following line to the script where
you want execution to pause:

    import IPython; IPython.embed()
    
Then, run the script as normal. Once you hit the breakpoint, you can use IPython
as a normal REPL. A useful IPython magic is `whos`, which shows all local
variables.

### Exploration
To understand the internal logic, generating a function call dependency graph
can be helpful by providing a quick visual overview. Use the following code to
create the function call dependency graph. It is dynamic, and allows quickly
getting familiarized with the analyzer. Note that you'll need `dot` engine provided 
by the `graphviz` package. You can install `graphviz` using your system's package manager.

    pip3 install pyan3==1.1.1
    python3 -m pyan unitTables.py --uses --no-defines --colored --grouped --annotated --html > fundeps.html

Alternatively, only create the `.dot` file using the following line, and render it with an online renderer like http://viz-js.com/

    python3 -m pyan unitTables.py --uses --no-defines --colored --grouped --annotated --dot > fundeps.dot

Enjoy!

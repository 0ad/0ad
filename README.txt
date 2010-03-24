 0 A.D. Introductory Information
=================================

0 A.D. (pronounced "zero ey-dee") is a free, open-source, cross-platform
real-time strategy game of ancient warfare.

This is currently an incomplete, under-development version of the game.
We're always interested in getting more people involved, to help bring the game
towards completion and to share the interesting experience of developing a
project of this scope.

There are several ways to contact us and find more information:

  Web site: http://wildfiregames.com/0ad/

  Forums: http://www.wildfiregames.com/forum/

  Trac (development info, bug tracker): http://trac.wildfiregames.com/

  IRC: #0ad on irc.quakenet.org


---------------------------------------
Running precompiled binaries on Windows
---------------------------------------

Open the "binaries\system" folder.

To launch the game: Run pyrogenesis.exe

To launch the map editor: Run Atlas.bat


-----------------------------------
Compiling the game from source code
-----------------------------------

The instructions for compiling the game on Windows, Linux and OS X are at
http://trac.wildfiregames.com/wiki/BuildInstructions


------------------
Reporting problems
------------------

Bugs should be reported on Trac: http://trac.wildfiregames.com/

On Windows: If the game crashes, it should generate 'crashlog' files that will
help us debug the problem. Run OpenLogsFolder.bat to find the files
("%appdata%\0ad\logs\"), and attach crashlog.txt and crashlog.dmp to the bug
report.

On Linux / OS X: If the game detects an error, it should generate crashlog.txt
in ~/.config/0ad/logs/ . If it doesn't, it should at least print some
information on stdout. Attach crashlog.txt or the other information to the
bug report. It might also help to run the game in gdb and copy the output of
the "bt full" command, if possible.

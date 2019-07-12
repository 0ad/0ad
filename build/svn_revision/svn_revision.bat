@ECHO OFF
REM Generate a Unicode string constant from svnversion's output and
REM write it to svn_revision.txt
FOR /F "tokens=*" %%i IN ('..\bin\svnversion ..\..\source -n') DO SET output=%%i
SET output=L"%output%"
ECHO %output% > svn_revision.txt

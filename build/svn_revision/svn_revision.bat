@ECHO OFF
REM Generate a Unicode string constant from svnversion's output and
REM write it to svn_revision.txt
SET output=L"
FOR /F "tokens=*" %%i IN ('..\..\build\bin\svnversion . -n') DO set output=%output%%%i
set output=%output%"
echo %output% > svn_revision.txt

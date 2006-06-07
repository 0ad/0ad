del Coverage\*.dyn
cd ..\..\..\binaries\system
..\..\source\i18n\tests\Coverage\tests
cd ..\..\source\i18n\tests\Coverage
profmerge
codecov -comp ../comp.txt -ccolor #f6f6ff -mname "Philip Taylor" -maddr excors@gmail.com
start CODE_COVERAGE.HTML
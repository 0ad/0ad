del Coverage\*.dyn
Coverage\tests
cd Coverage
profmerge
codecov -comp ../comp.txt -ccolor #f6f6ff -mname "Philip Taylor" -maddr excors@gmail.com
start CODE_COVERAGE.HTML
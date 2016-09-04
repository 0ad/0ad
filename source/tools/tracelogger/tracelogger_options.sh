#!/bin/sh

# Source this script with `. tl-options.sh` in order to set up environment variables for the
# SpiderMonkey tracelogger.
# Once the variables are set, run ./pyrogenesis in binaries/system with the options you need and
# the tracelogging data will be saved to source/tools/tracelogger/

# After the run, use this tool: https://github.com/h4writer/tracelogger to display the data.
# The last tested version of the tool is 1c67e97e794b5039d0cae95f72ea0c76e4aa4696,
# it can be used if more recent versions cause trouble.

# Use semicolons to separate values on Windows.
# If that produces bogus output, you can try with commas instead.
if [ "${OS}" = "Windows_NT" ]
then
  export TLLOG="Defaults;IonCompiler"
  export TLOPTIONS="EnableMainThread;EnableOffThread;EnableGraph"
else
  export TLLOG=Defaults,IonCompiler
  export TLOPTIONS=EnableMainThread,EnableOffThread,EnableGraph
fi

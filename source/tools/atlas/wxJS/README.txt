Based on wxJavaScript r741, with some modifications:

# Fix line endings
for i in `find . -name '*.cpp' -or -name '*.h'`; do dos2unix $i; done
for i in `find . -name '*.cpp' -or -name '*.h'`; do svn propset svn:eol-style native $i; done

# Add '#include "precompiled.h"' to every .cpp file
for i in `find common ext gui io -name '*.cpp'`; do mv $i $i~; ( echo -e "#include \"precompiled.h\"\n" ; cat $i~ ) >$i; rm $i~; done

# Fix JS include paths
for i in `grep -lr '<jsapi.h>' .`; do sed -i 's/<jsapi.h>/<js\/jsapi.h>/' $i; done
for i in `grep -lr '<jsdate.h>' .`; do sed -i 's/<jsdate.h>/<js\/jsdate.h>/' $i; done

# Rename common filenames to prevent naming conflicts when we compile everything together
for i in io ext gui; do
  for j in init constant main; do
    mv $i/$j.cpp $i/${i}_$j.cpp 2>/dev/null;
  done;
done



gui/misc/app.cpp: delete
  "IMPLEMENT_APP_NO_MAIN(App)"

io/io_constant.cpp: replace
  "JSConstDoubleSpec wxGlobalMap[] =
   {
      WXJS_SIMPLE_CONSTANT(wxNOT_FOUND)
      { 0 }
   };"
with
  "extern JSConstDoubleSpec wxGlobalMap[];"


...and some other minor things
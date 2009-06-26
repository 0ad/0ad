Based on wxJavaScript 0.9.71, with some modifications:

svn co -r818 https://wxjs.svn.sourceforge.net/svnroot/wxjs/trunk/wxJS2/src/ temp_src_dir
# Copy {common,ext,gui,io} source files into tools/atlas/wxJS
# Keep non-standard files gui/{control/{notebook,bookctrl},event/notebookevt,misc/timer}.{cpp,h}

# Rename common filenames to prevent naming conflicts when we compile everything together
for i in io ext gui; do
  for j in init constant main; do
    mv $i/$j.cpp $i/${i}_$j.cpp 2>/dev/null;
  done;
done

# Make sure new files are added to our SVN

# Fix line endings
for i in `find . -name '*.cpp' -or -name '*.h'`; do dos2unix $i; done
for i in `find . -name '*.cpp' -or -name '*.h'`; do svn propset svn:eol-style native $i; done

# Add '#include "precompiled.h"' to every .cpp file
for i in `find common ext gui io -name '*.cpp'`; do
  if [[ ! ( `grep precompiled.h $i` ) ]]; then
    mv $i $i~; ( echo -e "#include \"precompiled.h\"\n" ; cat $i~ ) >$i; rm $i~;
  fi
done

# Fix JS include paths
for i in `grep -lr '<jsapi.h>' .`; do sed -i 's/<jsapi.h>/<js\/jsapi.h>/' $i; done
for i in `grep -lr '<jsdate.h>' .`; do sed -i 's/<jsdate.h>/<js\/jsdate.h>/' $i; done

io/io_constant.cpp: replace
  "JSConstDoubleSpec wxGlobalMap[] =
   {
      WXJS_SIMPLE_CONSTANT(wxNOT_FOUND)
      { 0 }
   };"
with
  "extern JSConstDoubleSpec wxGlobalMap[];"
(since it conflicts with wxGlobalMap in gui/misc/constant.cpp)

gui/gui_init.cpp: add
    #include "control/bookctrl.h"
    #include "control/notebook.h"
    #include "misc/timer.h"

    obj = BookCtrlBase::JSInit(cx, global, Control::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxBookCtrlBase prototype creation failed"));
    if (! obj )
        return false;

    obj = Notebook::JSInit(cx, global, BookCtrlBase::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxNotebook prototype creation failed"));
    if (! obj )
        return false;

    obj = Timer::JSInit(cx, global);
    wxASSERT_MSG(obj != NULL, wxT("wxTimer prototype creation failed"));
    if (! obj )
        return false;

gui/event/jsevent.cpp: add
    obj = NotebookEvent::JSInit(cx, global, NotifyEvent::GetClassPrototype());
    wxASSERT_MSG(obj != NULL, wxT("wxNotebookEvent prototype creation failed"));
    if (! obj )
        return false;

TODO: add back tooltips into window.cpp

gui/sizer: add 'clear'
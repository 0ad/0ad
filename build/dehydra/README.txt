Dehydra is a tool that allows custom static analysis of C++ code, with analysis code written in JavaScript, running as a GCC plugin.

This directory has some analysis scripts. The setup is a bit ad hoc and not well tested or integrated into the build system or anything, so use at your own risk.

General usage instructions:

 * Run Linux. (It might work on OS X too.)

 * Install Dehydra, as per https://developer.mozilla.org/En/Dehydra/Installing_Dehydra

 * Build 0 A.D. from build/workspaces/gcc:
    export CXX="$HOME/gcc-dehydra/installed/bin/g++ -fplugin=$HOME/gcc-dehydra/dehydra/gcc_treehydra.so -fplugin-arg-gcc_treehydra-script=../../dehydra/printf-type-check.js -DCONFIG_DEHYDRA=1"
    make
    # (or "make test -j3 -k" to build the engine and tests and to do 3 files in parallel and continue past errors, etc)

 * Wait (it's quite slow) and look for the new compiler warnings/errors.

The "tests" directory doesn't actually contain any proper tests, just some example files and expected outputs for rough sanity checking.


Some Dehydra fixes might be needed, depending on what version you use, like:

diff -r 9871caaedb8f dehydra.c
--- a/dehydra.c Sat Mar 12 13:55:41 2011 -0500
+++ b/dehydra.c Wed May 18 19:27:23 2011 +0100
@@ -123,7 +123,7 @@
   };
 
   this->fndeclMap = pointer_map_create ();
-  this->rt = JS_NewRuntime (0x32L * 1024L * 1024L);
+  this->rt = JS_NewRuntime (0x128L * 1024L * 1024L);
   if (this->rt == NULL)
     exit(1);
 
diff -r 9871caaedb8f libs/treehydra.js
--- a/libs/treehydra.js Sat Mar 12 13:55:41 2011 -0500
+++ b/libs/treehydra.js Wed May 18 19:27:23 2011 +0100
@@ -209,6 +209,9 @@
       walk_tree (i.stmt (), func, guard, stack);
     }
     break;
+  case EXPR_STMT:
+    walk_tree (TREE_OPERAND(t, 0), func, guard, stack);
+    break;
   case TEMPLATE_PARM_INDEX:
   case PTRMEM_CST:
   case USING_DECL:


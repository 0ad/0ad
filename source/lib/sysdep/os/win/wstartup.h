/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * windows-specific startup code
 */

// linking with this component should automatically arrange for winit's
// functions to be called at the appropriate times.
//
// the current implementation manages to trigger initialization in-between
// calls to CRT init and the static C++ ctors. that means wpthread etc.
// APIs are safe to use from ctors, and winit initializers are allowed
// to use non-stateless CRT functions such as atexit.
//
// IMPORTANT NOTE: if compiling this into a static lib and not using VC8's
// "use library dependency inputs" linking mode, the object file will be
// discarded because it does not contain any symbols that resolve another
// module's undefined external(s). for a discussion of this topic, see:
// http://forums.microsoft.com/MSDN/ShowPost.aspx?PostID=144087
// workaround: in the main EXE project, reference a symbol from this module,
// thus forcing it to be linked in. example:
// #pragma comment(linker, "/include:_wstartup_InitAndRegisterShutdown")
// (doing that in this module isn't sufficient, because it would only
// affect the librarian and its generation of the static lib that holds
// this file. instead, the process of linking the main EXE must be fixed.)

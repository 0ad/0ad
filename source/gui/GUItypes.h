#if 0
=pod /* (These C++ comments in Perl code are to please my syntax highlighter)

	 
This file is used by all bits of GUI code that need to repeat some code
for a variety of types (to avoid repeating the list of types in half a dozen
places, and to make it much easier to add a new type). Just do
		#define TYPE(T) your_code_involving_T;
		#include "GUItypes.h"
		#undef TYPE
to handle every possible type.

If you want to exclude a particular type, define e.g. GUITYPE_IGNORE_CStr

To alter this file, adjust the types in the indented list below, then run
"perl GUITypes.h" to regenerate it. (Or if you want to do it manually, make
sure you update the four mentions of each typename in this file.)


=cut */

my @types = qw(


			bool 
			int 
			float 
			CColor 
			CClientArea 
			CGUIString 
			CGUISpriteInstance 
			CStr 
			CStrW 
			EAlign 
			EVAlign
			CPos


);

#// Extract everything from this file, above the /********/ line
open IN, $0 or die "Error opening $0: $!";
my $out = "";
while (<IN>)
{
	last if $_ eq "/********/\n";
	$out .= $_;
}
$out .= "/********/\n";
$out .= "#ifndef GUITYPE_IGNORE_$_\nTYPE($_)\n#endif\n" for @types;

#// and some minor hacks to make autocompleting things happier:
$out .= "#ifdef PLEASE_DO_NOT_DEFINE_THIS\n// See IGUIObject.h for 'enum EGUISettingType'\nenum {" . (join ',', map "GUIST_$_", @types) . "};\n#endif\n";

#// Overwrite the current program with the newly-generated contents
close IN;
open OUT, ">$0" or die "Error opening >$0: $!"; #// TODO: Find whether it's safe for a program to overwrite itself. (It seems to work, at least on Windows)
print OUT $out;
close OUT;

__END__
#endif


/********/
#ifndef GUITYPE_IGNORE_bool
TYPE(bool)
#endif
#ifndef GUITYPE_IGNORE_int
TYPE(int)
#endif
#ifndef GUITYPE_IGNORE_float
TYPE(float)
#endif
#ifndef GUITYPE_IGNORE_CColor
TYPE(CColor)
#endif
#ifndef GUITYPE_IGNORE_CClientArea
TYPE(CClientArea)
#endif
#ifndef GUITYPE_IGNORE_CGUIString
TYPE(CGUIString)
#endif
#ifndef GUITYPE_IGNORE_CGUISpriteInstance
TYPE(CGUISpriteInstance)
#endif
#ifndef GUITYPE_IGNORE_CStr
TYPE(CStr)
#endif
#ifndef GUITYPE_IGNORE_CStrW
TYPE(CStrW)
#endif
#ifndef GUITYPE_IGNORE_EAlign
TYPE(EAlign)
#endif
#ifndef GUITYPE_IGNORE_EVAlign
TYPE(EVAlign)
#endif
#ifndef GUITYPE_IGNORE_CPos
TYPE(CPos)
#endif
#ifdef PLEASE_DO_NOT_DEFINE_THIS
// See IGUIObject.h for 'enum EGUISettingType'
enum {GUIST_bool,GUIST_int,GUIST_float,GUIST_CColor,GUIST_CClientArea,GUIST_CGUIString,GUIST_CGUISpriteInstance,GUIST_CStr,GUIST_CStrW,GUIST_EAlign,GUIST_EVAlign,GUIST_CPos};
#endif

# Used to make variations of the launcher exe, which is necessary because drag-and-drop
# onto .bat files appears to not run in the right directory (and so there's no way of it
# finding the actual textureconv.exe)
#
# It just compiles launch.c, then replaces the REPLACEME string with other strings

use strict;
use warnings;

my %settings = (
	TexConv_Normal => "",
	TexConv_WithAlpha => "-alphablock",
	TexConv_WithoutAlpha => "-noalphablock",
	TexConv_ToTGA => "-tga",
);

system("cl /Od launch.c shell32.lib");
die $? if $?;

open A, 'launch.exe' or die $!;
binmode A;
my $d = do { local $/; <A> };

my $str = "REPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEME";

for my $k (keys %settings) {
	die if length $settings{$k} > length $str;
	my $d2 = $d;
	$d2 =~ s/$str/ $settings{$k} . ("\0" x (length($str) - length($settings{$k}))) /e or die;
	open O, '>', "$k.exe" or die $!;
	binmode O;
	print O $d2;
}
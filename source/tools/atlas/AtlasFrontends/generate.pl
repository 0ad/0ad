use strict;
use warnings;

my @files = qw( .cpp .vcproj .rc );

my @progs = (
	{
		PROJECT_NAME	=> "ActorEditor",
		GUID			=> "",
		EXE_NAME		=> "ActorEditor",
		WINDOW_NAME		=> "ActorEditor",
	},
	{
		PROJECT_NAME	=> "ColourTester",
		GUID			=> "",
		EXE_NAME		=> "ColourTester",
		WINDOW_NAME		=> "ColourTester",
	},
	{
		PROJECT_NAME	=> "FileConverter",
		GUID			=> "",
		EXE_NAME		=> "FileConverter",
		WINDOW_NAME		=> "FileConverter",
	},
	{
		PROJECT_NAME	=> "ArchiveViewer",
		GUID			=> "",
		EXE_NAME		=> "ArchiveViewer",
		WINDOW_NAME		=> "ArchiveViewer",
	},

);

for my $p (@progs) {
	for my $f (@files) {
		open IN, "<", "_template$f" or die "Error opening _template$f: $!";
		open OUT, ">", "$p->{PROJECT_NAME}$f" or die "Error opening $p->{PROJECT_NAME}$f: $!";
		while (<IN>) {
			s/\$\$([A-Z_]+)\$\$/ $p->{$1} /eg;
			print OUT;
		}
	}	
}

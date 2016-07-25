use strict;
use warnings;

my @files = qw( .cpp .rc );

my @progs = (
	{
		PROJECT_NAME	=> "ActorEditor",
		WINDOW_NAME		=> "ActorEditor",
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

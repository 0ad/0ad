use strict;
use warnings;

package StringExtract::JSCode;

use StringExtract::Utils;

use Regexp::Common qw(delimited comment);

our $data = {

	file_roots => [
		'binaries/data/mods/official/gui',
	],

	file_types => qr/\.js$/i,

	readfile_func => sub { extract(StringExtract::Utils::read_text(@_)); },
};

sub extract {
	my ($data) = @_;

	$data =~ s/$RE{comment}{Java}//g;

	my @strings;

	while ($data =~ /translate\s*\(\s*($RE{delimited}{-delim=>'"'})/g) {
		my $str = $1;
		# Remove surrounding quotes
		$str =~ s/^"(.*)"$/$1/;
		# Translate \" sequences
		$str =~ s/\\"/"/g;
		# and \n
		$str =~ s/\\n/\n/g;
		# and \\
		$str =~ s/\\\\/\\/g;

		push @strings, [ $1, "GUI script" ];
	}

	return @strings;
}

1;
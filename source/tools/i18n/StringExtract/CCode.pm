use strict;
use warnings;

package StringExtract::CCode;

use StringExtract::Utils;

use Regexp::Common qw(delimited comment);

our $data = {

	file_roots => [
		'source',
	],
	# TO DO:
	# Limit the extent of the search, so it doesn't check e.g. the i18n test system

	file_types => qr/\.(?:cpp|h)$/i,

	readfile_func => sub { extract(StringExtract::Utils::read_text(@_)); },
};

sub extract {
	my ($data) = @_;

	$data =~ s/$RE{comment}{'C++'}//g;

	my @strings;

	while ($data =~ /translate\s*\(\s*L\s*($RE{delimited}{-delim=>'"'})\s*\)/g) {
		my $str = $1;
		# Remove surrounding quotes
		$str =~ s/^"(.*)"$/$1/;
		# Translate \" sequences
		$str =~ s/\\"/"/g;
		# and \n
		$str =~ s/\\n/\n/g;
		# and \\
		$str =~ s/\\\\/\\/g;

		push @strings, [ $1, "C++ code" ];
	}

	return @strings;
}

1;
use strict;
use warnings;

package StringExtract::CCode;

use StringExtract::Utils;

use Regexp::Common qw(delimited);

our $data = {

	file_roots => [
		'source',
	],

	file_roots_ignore => [
		'source/i18n/tests',
		'source/tools',
	],

	file_types => qr/\.(?:cpp|h)$/i,

	readfile_func => sub { extract(StringExtract::Utils::read_text(@_)); },
};

sub extract {
	my ($data, $text) = @_;

	$text = StringExtract::Utils::strip_comments($text, 'C++');

	my @strings;

	while ($text =~ /translate\s*\(\s*L\s*($RE{delimited}{-delim=>'"'})\s*\)/g) {
		my $str = $1;

		# Remove surrounding quotes
		$str =~ s/^"(.*)"$/$1/;
		# Translate \" sequences
		$str =~ s/\\"/"/g;
		# and \n
		$str =~ s/\\n/\n/g;
		# and \\
		$str =~ s/\\\\/\\/g;

		(my $filename = $data->{filename}) =~ s~.*?source/~~; # make the filenames a bit neater

		push @strings, [ "phrase:".$str, "C++ $filename:".(1+StringExtract::Utils::count_newlines(substr $text, 0, pos $text)) ];
	}

	return @strings;
}

1;
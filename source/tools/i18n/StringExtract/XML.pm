use strict;
use warnings;

package StringExtract::XML;

use StringExtract::Utils;
use StringExtract::JSCode;

our $data = {

	file_roots => [
		'binaries/data/mods/official/art',
		'binaries/data/mods/official/entities',
		'binaries/data/mods/official/gui',
	],

	file_types => qr/\.xml$/i,

	readfile_func => sub { extract(StringExtract::Utils::read_xml(@_)); },
};

sub extract {
	my ($xmldata) = @_;
	my ($root, %elements) = ($xmldata->[0], %{$xmldata->[1]});

	my @strings;

	# Entities
	if ($root eq 'entity') {

		push @strings, map [ $_->{content}, "Entity name" ], @{$elements{name}};

	# Actors
	} elsif ($root eq 'object') {

		push @strings, map [ $_->{content}, "Actor name" ], @{$elements{name}};

	# GUI objects
	} elsif ($root eq 'objects') {

		recursive_extract_guiobject(\@strings, [\%elements]);#$elements{object});

	# GUI setup
	} elsif ($root eq 'setup') {
		# Do nothing

	# GUI sprites
	} elsif ($root eq 'sprites') {
		# Do nothing

	# GUI styles
	} elsif ($root eq 'styles') {
		# Do nothing

	} else {
		die "Unrecognised XML file type '$root'";
	}
	return @strings;

}

sub recursive_extract_guiobject {
	my ($strings, $elements) = @_;
	for my $element (@$elements) {
		push @$strings, [ $element->{tooltip}, "GUI tooltip" ] if defined $element->{tooltip};
		push @$strings, [ $element->{content}, "GUI text"    ] if defined $element->{content};

		if ($element->{script}) {
			push @$strings, StringExtract::JSCode::extract($_->{content}) for @{$element->{script}};
		}
		if ($element->{action}) {
			push @$strings, StringExtract::JSCode::extract($_->{content}) for @{$element->{action}};
		}

		recursive_extract_guiobject($strings, $element->{object}) if $element->{object};
	}
}

1;
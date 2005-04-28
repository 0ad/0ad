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
	return unless $xmldata;
	
	my ($root, $filename, %elements) = (@{$xmldata}[0, 1], %{$xmldata->[2]});

	my @strings;

	# Entities
	if ($root eq 'entity') {

		push @strings, map [ "noun:".$_->{content}, "Entity name ($filename)" ], @{$elements{name}};

	# Actors
	} elsif ($root eq 'object') {

		push @strings, map [ "noun:".$_->{content}, "Actor name ($filename)" ], @{$elements{name}};

	# Materials
	} elsif ($root eq 'material') {

	# GUI objects
	} elsif ($root eq 'objects') {

		recursive_extract_guiobject($filename, \@strings, [\%elements]);

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
		warn "Unrecognised XML file type '$root' - ignoring";
	}
	return @strings;

}

sub recursive_extract_guiobject {
	my ($filename, $strings, $elements) = @_;
	for my $element (@$elements) {
		push @$strings, [ "phrase:".$element->{tooltip}, "GUI tooltip ($filename)" ] if defined $element->{tooltip};
		push @$strings, [ "phrase:".$element->{content}, "GUI text ($filename)"    ] if defined $element->{content};

		if ($element->{script}) {
			push @$strings, StringExtract::JSCode::extract($_->{content}) for @{$element->{script}};
		}
		if ($element->{action}) {
			push @$strings, StringExtract::JSCode::extract($_->{content}) for @{$element->{action}};
		}

		recursive_extract_guiobject($filename, $strings, $element->{object}) if $element->{object};
	}
}

1;
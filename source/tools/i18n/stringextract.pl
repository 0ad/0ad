=pod

String extraction utility


=cut

use strict;
use warnings;

use File::Find;


use StringExtract::XML;
use StringExtract::JSCode;
use StringExtract::CCode;

my %all_strings;

for my $type (
	$StringExtract::XML::data,
	$StringExtract::JSCode::data,
	$StringExtract::CCode::data,
) {

	# Get the list of files that the module wants to handle
	my @files;
	find(sub {
		push @files, $File::Find::name if /$type->{file_types}/;
	}, map "../../../$_", @{$type->{file_roots}});

#@files=@files[0..2];
	
	my @strings = map $type->{readfile_func}->($_), @files;

	for (@strings) {
		$all_strings{$_->[0]} ||= [];
		push @{$all_strings{$_->[0]}}, $_->[1];
	}
}

use Data::Dumper; print join "\n", map join(' :: ', $_, @{$all_strings{$_}}), sort keys %all_strings;
#print join ", ", keys %all_strings;
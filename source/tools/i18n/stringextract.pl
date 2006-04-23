=pod

String extraction utility.


=cut

use strict;
use warnings;

our %config;
do "config.pl";

use File::Find;

# Include appropriate pieces of code
use StringExtract::XML;
use StringExtract::JSCode;
use StringExtract::CCode;

use DataFiles;
use Strings;

my %all_strings;

# Repeat for each data type that needs to be parsed
for my $type (
	$StringExtract::XML::data,
#	$StringExtract::JSCode::data,
#	$StringExtract::CCode::data,
) {

	# Get the list of files that the module wants to handle

	my @files;

	# Search each file_roots, relative to ../../../
	my $prefix = "../../../";
	my @dirs = map $prefix.$_, @{$type->{file_roots}};
	find({
		preprocess => sub {
			# Trim the ../../../ prefix
			my $path = substr($File::Find::dir, length $prefix);
			# Ignore all directories that are called '.svn', or whose paths are any of file_roots_ignore
			grep not ($_ eq '.svn' or contains($type->{file_roots_ignore}, $path.'/'.$_)), @_;
		},
		wanted => sub {
			# Include files that match the file_types regexp
			push @files, $File::Find::name if /$type->{file_types}/;
		},
		no_chdir => 1,
	}, @dirs);


	# Call the appropriate read function on every matching file	
	my @strings = map $type->{readfile_func}->($_), @files;

	# Now @strings = ( [stringid, context], ... )
	# where context eq 'Entity (whatever.xml:123)'

	# Build %all_strings = (stringid => [context, context, ...], ...)
	for (@strings) {
		# Make sure the value is an array ref
		$all_strings{$_->[0]} ||= [];
		# Push the string's context data onto the array ref
		push @{$all_strings{$_->[0]}}, $_->[1];
	}
}

# Transform into %all_strings = (stringid => { context => '...' })
for (keys %all_strings) {
	$all_strings{$_} = { context => join "\n", uniq(sort @{$all_strings{$_}}) };
}

# Merge the string data with any existing information
Strings::merge($config{strings_filename}, %all_strings);

sub contains { $_[1] eq $_ && return 1 for @{$_[0]}; 0 }

sub uniq { # Uniquify sorted lists
	my @r;
	for (@_) { push @r, $_ unless @r and $r[-1] eq $_ };
	@r;
}
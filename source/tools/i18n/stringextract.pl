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

my %all_strings;

# Repeat for each data type that needs to be parsed
for my $type (
#	$StringExtract::XML::data,
#	$StringExtract::JSCode::data,
	$StringExtract::CCode::data,
) {

	# Get the list of files that the module wants to handle

	my @files;

	# Search each file_roots, relative to ../../../
	my $prefix = "../../../";
	my @dirs = map $prefix.$_, @{$type->{file_roots}};
	find({
		preprocess => sub {
			my $path = substr($File::Find::dir, length $prefix);
			grep !contains($type->{file_roots_ignore}, $path.'/'.$_), @_;
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

# Merge the string data with any existing information
merge_strings($config{strings_filename}, %all_strings);

sub merge_strings {
	my ($filename, %new_strings) = @_;
	
	# Read the earlier string data
	my $strings = DataFiles::read_file($filename, ignoremissing=>1);
	
	for my $old (@$strings) {
		my $stringid = $old->[STR_ID];
		
		if ($new_strings{$stringid}) {
			# String already exists; just update the context
			$old->[STR_CONTEXT] = join "\n", @{ $new_strings{$stringid} };

			# Make sure it's not obsolete now
			flag_set(\$old->[STR_FLAGS], 'obsolete', 0);

			# Remove it from this list, so the unprocessed ones can be found later
			delete $new_strings{$stringid};

		} else {
			# String has been removed; set obsolete flag
			flag_set(\$old->[STR_FLAGS], 'obsolete', 1);
		}
	}
	
	for (keys %new_strings) {
		# Newly added strings
		push @$strings, [ $_, join("\n", @{ $new_strings{$_} }), "?", "" ];
	}
	
	DataFiles::write_file($filename, $strings);
}


sub flag_set {
	my ($str, $flagname, $value) = @_;
	my @flags = split / /, $$str;
	if ($value) {
		push @flags, $flagname unless grep $_ eq $flagname, @flags;	
	} else {
		@flags = grep $_ ne $flagname, @flags;	
	}
	$$str = join ' ', @flags;
}

sub contains { $_[1] eq $_ && return 1 for @{$_[0]}; 0 }
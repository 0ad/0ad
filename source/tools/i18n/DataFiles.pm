use strict;
use warnings;

package DataFiles;

# Field numbers, so the arrays which (read|write)_file use can be accessed more pleasantly
use constant STR_ID => 0;
use constant STR_CONTEXT => 1;
use constant STR_DESCRIPTION => 2;
use constant STR_FLAGS => 3;

# Export the field numbers
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(STR_ID STR_CONTEXT STR_DESCRIPTION STR_FLAGS);


# Read a file containing \n-separated fields and \n\n-separated records
sub read_file {
	my ($filename, %params) = @_;

	# If 'ignoremissing' has been passed, handle non-existent files as if they were empty
	if ($params{ignoremissing} and not -e $filename) {
		return [];
	}
	
	open my $f, '<', $filename or die "Error opening $filename for input: $!";

	my (@records, @fields);
	while (<$f>) {
		chomp;
		if (length) {
			# Each line stores a separate field
			push @fields, unescape($_);
		} else {
			# Split records by empty lines
			push @records, [@fields];
			@fields = ();
		}
	}
	push @records, [@fields] if @fields;
	
	return \@records;
}

# Inverse of read_file
sub write_file {
	my ($filename, $data) = @_;	
	open my $f, '>', $filename or die "Error opening $filename for output: $!";
	for (@$data) {
		print $f escape($_), "\n" for @$_;
		print $f "\n";	
	}
}

# Convert \ to \\, newlines to \n, blank lines to \
sub unescape {
	my $t = $_[0];
	return "" if $t eq "\\";
	$t =~ s/\\n/\n/g;
	$t =~ s/\\\\/\\/g;
	$t;
}
sub escape {
	my $t = $_[0];
	return "\\" unless length $t;
	$t =~ s/\\/\\\\/g;
	$t =~ s/\n/\\n/g;
	$t;
}

1;
use strict;
use warnings;

package Strings;

use DataFiles;

sub merge {
	my ($filename, %new_strings) = @_;
	
	# Read the earlier string data
	my $strings = DataFiles::read_file($filename, ignoremissing=>1);
	
	for my $old (@$strings) {
		my $stringid = $old->[STR_ID];
		
		if (exists $new_strings{$stringid}) {
			# String already exists; just update the content
			if ($new_strings{$stringid}{context}) {
				$old->[STR_CONTEXT] = $new_strings{$stringid}{context};
			}
			if ($new_strings{$stringid}{description}) {
				$old->[STR_DESCRIPTION] = $new_strings{$stringid}{description};
			}

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
		push @$strings, [ $_, len_or($new_strings{$_}{context}, "?"), len_or($new_strings{$_}{description}, "?"), "" ];
	}
	
	DataFiles::write_file($filename, $strings);
}

sub len_or { (defined $_[0] and length $_[0]) ? $_[0] : $_[1] }


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


1;

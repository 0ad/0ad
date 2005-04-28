use strict;
use warnings;

package Translations;

use DataFiles;

sub merge {
	my ($filename, %new_translations) = @_;
	
	# Read the earlier translation data
	my $translations = DataFiles::read_file($filename, ignoremissing=>1);
	
	for my $old (@$translations) {
		my $stringid = $old->[0];
		
		if (exists $new_translations{$stringid}) {
			# Translation already exists; just update the content
			$old->[1] = $new_translations{$stringid};

			# Remove it from this list, so the unprocessed ones can be found later
			delete $new_translations{$stringid};

		} else {
			# String has been removed; leave it for now
		}
	}
	
	for (keys %new_translations) {
		# Newly added translations
		push @$translations, [ $_, len_or($new_translations{$_}, "") ];
	}
	
	DataFiles::write_file($filename, $translations);
}

sub len_or { (defined $_[0] and length $_[0]) ? $_[0] : $_[1] }


1;

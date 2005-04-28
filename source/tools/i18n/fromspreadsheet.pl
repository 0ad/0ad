use strict;
use warnings;

my $language = "english";

our %config;
do "config.pl";

use DataFiles;
use Strings;
use Translations;

use Spreadsheet::ParseExcel;

fromspreadsheet($config{strings_filename}, $config{data_path}."/$language/translations.txt", $config{data_path}."/$language/work.xls");

sub fromspreadsheet {
	my ($strings_filename, $translations_filename, $spreadsheet_filename) = @_;

	# Read data from spreadsheet:
	
	my $strings = DataFiles::read_file($strings_filename);
	my $translations = DataFiles::read_file($translations_filename, ignoremissing=>1);
	
	my $workbook = Spreadsheet::ParseExcel::Workbook->Parse($spreadsheet_filename);

	my %strings; # Data associated with each string. (type:id => { description => '...' }, ...)
	my %translations; # Data associated with each translation. (type:id => translation, ...)
	
	for my $worksheet (@{$workbook->{Worksheet}}) {
		my $type = $worksheet->{Name};
		
		my @rows;
		for my $row ($worksheet->{MinRow} .. $worksheet->{MaxRow}) {
			next if $row == 0;
			push @rows, [];
			for my $col ($worksheet->{MinCol} .. $worksheet->{MaxCol}) {
				push @{$rows[-1]}, $worksheet->{Cells}[$row][$col]->{Val};
			}
		}
		
		for (@rows) {
			my $id = $type.':'.$_->[0];

			if ($strings{$id}) {
				warn "Duplicated string $id!";	
			}
			$strings{$id} = { description => $_->[2] };
			$translations{$id} = $_->[1];
		}
	}

	Strings::merge($strings_filename, %strings);
		
	Translations::merge($translations_filename, %translations);

}

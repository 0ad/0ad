use strict;
use warnings;

my $language = "english";

our %config;
do "config.pl";

use DataFiles;

use Spreadsheet::ParseExcel;

fromspreadsheet($config{strings_filename}, $config{data_path}."/$language/translations.txt", $config{data_path}."/$language/work.xls");

sub fromspreadsheet {
	my ($strings_filename, $translations_filename, $spreadsheet_filename) = @_;
	
	my $strings = DataFiles::read_file($strings_filename);
	my $translations = DataFiles::read_file($translations_filename, ignoremissing=>1);
	
	my $workbook = Spreadsheet::ParseExcel::Workbook->Parse($spreadsheet_filename);

	my @stringdata; # = ( [type:id, description], ...)
	my @transdata; # = ( [type:id, translation], ...)
	
	for my $worksheet (@{$workbook->{Worksheet}}) {
		my $type = $worksheet->{Name};
		
		my @rows;
		for my $col ($worksheet->{MinCol} .. $worksheet->{MaxCol}) {
			push @cells, [];
			for my $row ($worksheet->{MinRow} .. $worksheet->{MaxRow}) {
				push @{$rows[-1]}, $worksheet->{Cells}[$row][$col]->{Val};
			}
		}
		
		for (@rows) {
			my $id = $type.':'.$_->[0];
			push @transsdata, [$id, $_->[1]];
			push @stringsdata, [$id, $_->[2]];
		}
	}
		
		
}

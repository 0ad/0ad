use strict;
use warnings;

my $language = "english";

our %config;
do "config.pl";

use DataFiles;

use Spreadsheet::WriteExcel;

tospreadsheet($config{strings_filename}, $config{data_path}."/$language/translations.txt", $config{data_path}."/$language/work.xls");

sub tospreadsheet {
	my ($strings_filename, $translations_filename, $spreadsheet_filename) = @_;
	
	my $strings = DataFiles::read_file($strings_filename);
	my $translations = DataFiles::read_file($translations_filename, ignoremissing=>1);
	
	my %data; # = ("phrase" => [ [id, translation, description, context, flags ], ... ], "noun" => ...)
	
	for my $string (@$strings) {
		my ($type, $id) = split /:/, $string->[STR_ID], 2;

		my $translation = find_translation($translations, $string->[STR_ID]);
		
		push @{$data{$type}}, [ $id, $translation, @$string[STR_DESCRIPTION, STR_CONTEXT, STR_FLAGS] ];
	}
	
	
	my $workbook = new Spreadsheet::WriteExcel ($spreadsheet_filename);
	for my $type (reverse sort keys %data) {

		my $worksheet = $workbook->add_worksheet($type);
		
		my $row = 0;

		for my $string (@{ $data{$type} }) {

			my $col = 0;
			$worksheet->write($row, $col++, $_) for @$string;
			
			++$row;
		}
		
	}
}

sub find_translation {
	$_->[0] eq $_[1] && return $_->[1] for @{$_[0]};
	"";
}

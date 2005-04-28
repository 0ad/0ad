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
	my %translations = map @$_, DataFiles::read_file($translations_filename, ignoremissing=>1);
	
	my %data; # = ("phrase" => [ [id, translation, description, context, flags ], ... ], "noun" => ...)
	
	for my $string (@$strings) {
		my ($type, $id) = split /:/, $string->[STR_ID], 2;

		my $translation = $translations{$string->[STR_ID]};
		
		push @{$data{$type}}, [ $id, $translation, @$string[STR_DESCRIPTION, STR_CONTEXT, STR_FLAGS] ];
	}
	
	
	my $workbook = new Spreadsheet::WriteExcel ($spreadsheet_filename);
	for my $type (reverse sort keys %data) {

		my $worksheet = $workbook->add_worksheet($type);
		
		my $headerformat = $workbook->add_format();
		$headerformat->set_bold(1);
		
		$worksheet->write(0,0, "String", $headerformat);
		$worksheet->write(0,1, "Translation", $headerformat);
		$worksheet->write(0,2, "Description", $headerformat);
		$worksheet->write(0,3, "Used by", $headerformat);
		
		$worksheet->freeze_panes(1, 0);
		
		my $mainformat = $workbook->add_format();
		$mainformat->set_align('top');
		$mainformat->set_text_wrap();
		
		$worksheet->set_column(0,0, 27, $mainformat);
		$worksheet->set_column(1,1, 45, $mainformat);
		$worksheet->set_column(2,2, 33, $mainformat);
		$worksheet->set_column(3,3, 33, $mainformat);
		
		my $row = 1;

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

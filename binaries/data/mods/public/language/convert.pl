use strict;
use warnings;

use Spreadsheet::ParseExcel;
use Encode;

opendir my $root, '.' or die "Error opening current directory: $!";
for my $d (grep { /^[a-z]+$/ and -d $_ } readdir $root) {
	opendir my $dir, $d or die "Error opening directory '$d': $!";
	for (grep /\.xls$/, readdir $dir) {
		/(.*)\.xls$/;
		print "* Converting $d/$1.xls\n";
		convert("$d/$1");
	}
}
print "* Completed\n\n";

sub convert {

	my $filename_in = "$_[0].xls";
	my $filename_base = $_[0];

	$_[0] =~ /([^\/]+)$/;
	my $table_name = $1;
	
	my $workbook = Spreadsheet::ParseExcel::Workbook->Parse($filename_in);
	die unless $workbook;
	my $worksheet = $workbook->{Worksheet}[0];
	
	my @data;
	
	my %encodings = (
		# Excel name => Encode name
		ucs2 => 'ucs2-be',
		ascii => 'ascii',
	);
	
	for my $r ($worksheet->{MinRow} .. $worksheet->{MaxRow}) {
		push @data, [];
		for my $c ($worksheet->{MinCol} .. $worksheet->{MaxCol}) {
			my $cell = $worksheet->{Cells}[$r][$c];
			if ($cell) {
				my $code = defined $cell->{Code} ? $cell->{Code} : 'ascii';
				if (not exists $encodings{$code}) {
					die "Unrecognised encoding '$code'";
				}
				push @{$data[-1]}, decode($encodings{$code}, $cell->{Val});
			} else {
				push @{$data[-1]}, '';
			}
		}
	}
	
	if (@data < 3) {
		die "Too few rows of data";	
	}
	
	# Remove the top line
	shift @data;
	
	if (1 != grep length, @{$data[0]}) {
		die "Second row must contain a single cell, either 'phrases' or the name of the word-table (e.g. 'nouns')";
	}
	
	my $title = (shift @data)->[0];
	
	# Remove blank lines
	while (@data and not grep length, @{$data[0]}) {
		shift @data;
	}
	
	if ($title eq 'phrases') {
		convert_phrases($filename_base, \@data);
	} else {
		convert_words($filename_base, $title, \@data);
	}
}

sub convert_words {
	
	my $filename_out = "$_[0].wrd";
	my $title = $_[1];
	my @data = @{$_[2]};

	my @keys = map lc, @{ shift @data };
	# Remove the English name from the list of keys
	shift @keys;

=pod

struct {
	u8 TitleLength;
	u16* Title;

	u8 PropCount;
	struct {
		u8 KeyLength;
		u16* Key;
	}* PropNames;

	u16 ValueCount;
	struct {
		u8 WordLength;
		u16* Word;
		struct {
			u8 PropLength;
			u16* Property;
		} Properties[ProprCount];
	} Words[ValueCount];
}

=cut

	open my $o, '>', $filename_out or die "Error opening $filename_out: $!";
	binmode $o;
	
	print $o pack 'C/a*', encode('utf16-le', $title);

	print $o pack 'C', scalar @keys;
	print $o pack 'C/a*', encode('utf16-le', $_) for @keys;
	
	print $o pack 'S', scalar @data;
	for (@data) {
		print $o pack 'C/a*', encode('utf16-le', $_) for @$_[0..@keys]; # 1 more than @keys, because the first 'property' is the English name
	}
	
	close $o;
}

sub convert_phrases {
	
	my $filename_out = "$_[0].lng";
	my @data = @{$_[1]};
	
	# Allow simple error reporting
	our $errors = 0;
	sub error($) { print STDERR "$_[0]\n"; ++$errors; }
	
	# Split the input file on "--" lines, and store an array ref
	# for each section containing the lines
	
	my @phrase_lines;
	my $line_no = 3;
	for (@data) {
		push @phrase_lines, [ [ $line_no, $_->[0] ], [ $line_no, $_->[1] ] ];
		++$line_no;
	}
	
	# Build phrase_data, being a list of [ raw key, parsed key, parsed translation ]
	
	my @phrase_data;
	
	for (@phrase_lines) {
		next unless @$_;
		my $key = shift @$_;
		if (not @$_) {
			error "Error in line $key->[0]: no translation specified for key '$key->[1]'";
			next;
		}
		my $translation = join "\n", map $_->[1], @$_;
	
		push @phrase_data, [ $key->[1], parse_translation($key->[1], $translation) ];
	}

	if ($errors) {
		die "ABORTING: $::errors errors found\n";
	}
	
	#use Data::Dumper; print Dumper \@phrase_data; exit;
	
=pod

Disk format:

file {
	u16 phrase_count
	phrase* phrases;
}

phrase {
	u16 key_length;
	u16* key_string; // not null-terminated
	
	u8 variable_count; // just for validating its use
	
	u8 section_count;
	translation_section* sections;
}

translation_section {
	u8 type;
	
	// Type 0: (string)
	u16 length;
	u16* text; // not null-terminated

	// OR Type 1: (variable)
	u8 id; // referring to the position in the key phrase

	// OR Type 2: (function)
	u8 namelength;
	u8* name;
	u8 paramcount;
	func_param* params;
}

func_param {
	u8 type;

	// Type 0: (string)
	u8 length;
	u16* text;

	// OR Type 1: (variable)
	u8 id; // referring to the position in the key phrase

	// OR Type 2: (int)
	u32 value;

	// (doubles/etc will just be stringified)
}

=cut
	
	# Output the language file to disk
	
	open my $o, '>', $filename_out or die "Error opening $filename_out: $!";
	binmode $o;
	
	print $o pack 'S', scalar @phrase_data;
	for (@phrase_data) {
		# $_ eq [ key, var count, [ sections... ] ]
	
		my $key = encode('utf16-le', $_->[0]);
		print $o pack 'S/a*C', $key, $_->[1];
	
		print $o pack 'C', scalar @{ $_->[2] };
	
		for (@{ $_->[2] }) {
			# $_ eq [ type, data... ]
	
			if ($_->[0] eq 'str') {
				print $o pack 'CS/a*', 0, encode('utf16-le', $_->[1]);
	
			} elsif ($_->[0] eq 'var') {
				print $o pack 'CC', 1, $_->[1];
	
			} elsif ($_->[0] eq 'code') {
				my ($name, @params) = @{$_->[1]};
				print $o pack 'CC/a*C', 2, $name, scalar @params;
	
				for (@params) {
	
					if ($_->[0] eq 'str') {
						print $o pack 'CC/a*', 0, encode('utf16-le', $_->[1]);
	
					} elsif ($_->[0] eq 'var') {
						print $o pack 'CC', 1, $_->[1];
	
					} elsif ($_->[0] eq 'int') {
						print $o pack 'Cl', 2, $_->[1];
	
					} else {
						die "Invalid func param type $_->[0]";
					}
	
				}
	
			} else {
				die "Invalid sec type $_->[0]";
			}
		}
	}
}

sub parse_basic {
	# Parse "\$[a-zA-Z0-9_]+" (allowing "$$" to represent "$")
	my @parts;
	for (split /(\$(?:\$|[a-zA-Z0-9_]+))/, $_[0]) {
		if (/\$([a-zA-Z0-9_]+)/) {
			push @parts, [ var => $1 ];
		} else {
			$_ = '$' if $_ eq '$$';
			if (@parts and $parts[-1][0] eq 'str') {
				$parts[-1][1] .= $_;
			} else {
				push @parts, [ str => $_ ];
			}
		}
	}
	return \@parts;
}

sub parse_complex {
	# Parse "\$[a-zA-Z0-9_]+" (allowing "$$" to represent "$")
	# as well as [...] code lumps

	my @chars = split //, $_[0];
	my @parts = [ str => '' ];

	while (@chars) {
		$_ = shift @chars;
		if ($_ eq '$') {
			if (@chars==0 or $chars[0] eq '$') {
				$parts[-1][1] .= '$';
			} else {
				my $varname = '';
				while (@chars and $chars[0] =~ /[a-zA-Z0-9_]/) {
					$varname .= shift @chars;
				}
				push @parts, [ var => $varname ];
				push @parts, [ str => '' ];
			}
		} elsif ($_ eq '[') {
			my $codetext;
			while (@chars and $chars[0] ne ']') {
				$codetext .= shift @chars;
			}
			shift @chars;
			push @parts, [ code => parse_code($codetext) ];
			push @parts, [ str => '' ];

		} else {
			$parts[-1][1] .= $_;
		}
	}

	@parts = grep $_->[1] ne '', @parts;
	return \@parts;
}


sub parse_code {
	my @rawparts = split /,\s*/, $_[0];
	my $name = shift @rawparts;
	my @parts;
	for (@rawparts) {
		if (/^\d+$/) {
			push @parts, [ 'int' => $_ ];
		} elsif (/^\$(.*)/) {
			push @parts, [ 'var' => $1 ];
		} else {
			push @parts, [ 'str' => $_ ];
		}
	}
	return [ $name, @parts ];
}


sub parse_translation {
	my ($key, $str) = @_;

	# Parse key to extract the variable sections
	$key = parse_basic($key);

	# Fill variables with ( var0 => 0, var1 => 1, var2 => 2 )
	my %variables;
	for (@$key) {
		$variables{$_->[1]} = scalar keys %variables if $_->[0] eq 'var';
	}
	my %unused_variables = %variables;

	# Parse the translated string for
	$str = parse_complex($str);

	# Replace variable strings with the appropriate numbers
	for (@$str) {
		if ($_->[0] eq 'var') {
			if (not exists $variables{$_->[1]}) {
				die "Unrecognised variable '$_->[1]'\n";
			}
			delete $unused_variables{$_->[1]};
			$_->[1] = $variables{$_->[1]};

		} elsif ($_->[0] eq 'code') {

			for (@{$_->[1]}[1 .. $#{$_->[1]}]) {

				if ($_->[0] eq 'var') {
					if (not exists $variables{$_->[1]}) {
						die "Unrecognised variable '$_->[1]'\n";
					}
					delete $unused_variables{$_->[1]};
					$_->[1] = $variables{$_->[1]};
				}
			}
		}
	}
	for (keys %unused_variables) {
		warn "Warning: variable '$_' unreferenced in translated string\n";
	}

	return (scalar keys %variables, $str);
}

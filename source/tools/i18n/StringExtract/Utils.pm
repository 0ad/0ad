use strict;
use warnings;

package StringExtract::Utils;

use XML::Simple();

use Regexp::Common qw(comment);

sub read_xml {
	my ($filename) = $_;

	my $xml = new XML::Simple(
					ForceArray => 1,
					KeepRoot => 1,
					KeyAttr => [],
					ForceContent => 1,
				);

	my $filedata = do { open my $f, '<', $filename or die $!; local $/; <$f> };
	my ($dir) = ($filename =~ /(.*)[\\\/]/);
	$filedata =~ s {SYSTEM "(?!/)} {SYSTEM "$dir/};
	$filedata =~ s {SYSTEM "/} {SYSTEM "../../../binaries/data/mods/official/};
	my $data = $xml->XMLin($filedata);
	recursive_process($data);
	
	my $root = (keys %$data)[0];
	return [ $root, @{$data->{$root}} ];
}

sub recursive_process {
	if (ref($_[0]) eq 'HASH') {

		# Force keys to lowercase
		my %temp;
		@temp{map lc, keys %{$_[0]}} = values %{$_[0]};
		%{$_[0]} = %temp;
		
		# Trim whitespace in content
		$_[0]{content} =~ s/^\s*(.*?)\s*$/$1/s if exists $_[0]{content};
		
		# Recurse through sub-elements
		recursive_process($_) for values %{$_[0]};

	} elsif (ref($_[0]) eq 'ARRAY') {

		# Recurse through sub-elements
		recursive_process($_) for @{$_[0]};
	}
}


sub read_text {
	my ($filename) = $_;

	open my $file, '<', $filename or die "Error opening $filename: $!";
	my $data = do { local $/; <$file> };

	return ({ filename => $filename }, $data);
}


sub strip_comments {
	my ($data, $lang) = @_;
	
	#$data =~ s/($RE{comment}{$lang})/ "\n" x count_newlines($1) /eg;
	
	# TODO: Make it work. (The above code fails on lines like 'translate(L"Go to http://oops, this shouldn't be a comment)"')
	
	$data;
}


sub count_newlines {
	return $_[0] =~ tr/\n//;	
}

1;
use strict;
use warnings;

=pod

The archive builder attempts to collect all files loaded by the game, and build them into a single zip file. It also sorts the files based on the order in which they're loaded.

It currently handles only files inside mods/official, since all other data-sets are likely to be small enough to not benefit significantly from anything more complex than WinZip, although it should be very easy to extend this program to handle any number of mods if necessary.


Basic overview:

* Get a list of all files that the game can use from a particular mod.
* Clean up the list by removing unnecessary files.
* Sort them, using the file-lists that the game generates.
* Create the new archive.


More detailed overview:

* Get a list of files:

  Check the mods/<modname> directory recursively for files. Files are added in alphabetic order. When a zip file is encountered in the root directory, all its contents are added.

  (Files need to be remembered as:
    virtual path
    real path, possibly in an archive
    priority [currently always zero])

  When adding a file, which may have the same virtual path as another file:
    If one file has higher priority than the other, use the higher priority one.
    If the files are the same (mtime and size), and one is in an archive while the other isn't, use the archived one.
    Otherwise, use the most recently added one. (This should only happen when two archives contain the same file.)

* Clean up the list:

  Partly done in the above stage:
    Prune CVS and .svn directories
  After generating the whole list, tidy up XMBs:
    If multiple XMBs exist for an XML file, ignore all but the one which corresponds to the current version of the XML.

* Sort them:

  Read all filelist.txt files.
  Sort the files based on those lists, using the most recent as the most influential.

* Create the new archive. If it's replacing an older archive built with this tool, delete that older one.

=cut


use File::Find;
use Archive::Zip;
use Class::Struct;

struct( FileData => [
	archive  => '$', # archive object, or undef if loose
	location => '$', # archive member object, or filename if loose
	priority => '$'  # numeric priority
] );

my $XMB_VERSION = 'A'; # TODO: Find a way of not having to update this manually


generate_archive("../../../binaries", "official");



sub generate_archive {
	my ($binaries, $modname) = @_;

	print "Generating list of files...\n";

	my $virtual_files = get_files("$binaries/data/mods/$modname");

	print "Tidying file list...\n";

	tidy_files($virtual_files);

	print "Sorting files...\n";

	my $sorted_files = sort_files($virtual_files, "$binaries/logs");

	print "Creating archive...\n";

	create_archive("$binaries/data/mods/$modname/$modname.zip", $sorted_files);

	print "Completed.\n";
}



### File retrieval code ###


# Gets a list of all data files under $root, reading archives and handling priorities
sub get_files {
	my ($root) = @_;

	my %virtual_files;

	# Code that gets called for every file under $root:
	my $wanted = sub {
		if (/(\.svn|CVS)$/) { # ignore .svn and CVS directories
			$File::Find::prune = 1;
		} else {
			# Ignore anything except plain files
			return unless -f;

			# Check for valid zip files in the root directory
			if ($File::Find::dir eq $root and is_zip($_)) {
				# Pass those zips on to add_zip, to add all its contents
				add_zip($_, \%virtual_files);
			} else {
				# Otherwise it's a normal file
				add_normal_file($_, \%virtual_files, $root);
			}
		}
	};

	# Make sure directories are handled in the correct order
	my $preprocess = sub { sort @_ };

	find({ wanted => $wanted, preprocess => $preprocess, no_chdir => 1 }, $root);

	return \%virtual_files;
}


# Trims a leading $root from $loc
sub get_virtual_path {
	my ($loc, $root) = @_;
	$loc =~ s~^\Q$root/~~; # remove the root directory from the path
	return $loc;
}


# Tests whether a file looks like a zip
sub is_zip {
	my ($filename) = @_;

	# Check for the four-byte header to make sure it's a zip
	open my $f, '<', $filename or die "Error opening $filename for input: $!";
	binmode $f;
	my $head;
	return 1 if read($f, $head, 4) == 4 and $head eq "PK\3\4";
	return 0;
}

# Adds the contents of a zip to $files
sub add_zip {
	my ($filename, $files) = @_;

	my $zip = Archive::Zip->new($filename) or die "Error opening zip file $filename";

	add_zipped_file($_, $files, $zip) for grep { not $_->isDirectory } $zip->members();
}


# Adds a single file from a zip to $files, taking care of varying priorities
sub add_zipped_file {
	my ($member, $files, $zip) = @_;

	my $virtual_path = $member->fileName;

	my $new_file = FileData->new(archive => $zip, location => $member, priority => 0);
	my $old_file = $files->{$virtual_path};

	if (should_override($new_file, $old_file)) {
		$files->{$virtual_path} = $new_file;
	}
}

# Like add_zipped_file, but for non-zipped files
sub add_normal_file {
	my ($filename, $files, $root) = @_;

	my $virtual_path = get_virtual_path($filename, $root);

	my $new_file = FileData->new(archive => undef, location => $filename, priority => 0);
	my $old_file = $files->{$virtual_path};

	if (should_override($new_file, $old_file)) {
		$files->{$virtual_path} = $new_file;
	}
}

# Work out whether $new_file ought to override $old_file, considering priorities and archivedness
sub should_override {
	my ($new_file, $old_file) = @_;

	# Use the new file if there's no old one
	return 1 if not $old_file;

	# Higher priority overrides lower priority
	return 1 if $new_file->priority > $old_file->priority;
	return 0 if $new_file->priority < $old_file->priority;

	if (files_equivalent($new_file, $old_file)) {

		# Later archives override earlier archives
		return 1 if $new_file->archive and $old_file->archive;

		# Archives override non-archives
		return 1 if $new_file->archive;
		return 0 if $old_file->archive;
	}

	# Later files override earlier files
	return 1;
}

# Test whether files are equivalent (i.e. probably the same file), based on their mtimes and sizes
sub files_equivalent {
	my ($new_file, $old_file) = @_;
	return get_mtime($new_file)==get_mtime($old_file) and get_size($new_file)==get_size($old_file);
}

sub get_mtime { # rounded down to 2 seconds. TODO: Work out what effect timezones / DST have.
	my ($file) = @_;
	return $file->archive ? $file->location->lastModTime : (stat $file->location)[9] & ~1;
}

sub get_size {
	my ($file) = @_;
	return $file->archive ? $file->location->uncompressedSize : (stat $file->location)[7];
}



### File list tidying code ###

sub tidy_files {
	my ($virtual_files) = @_;

	# Remove unnecessary XMBs:

	for my $filename (keys %$virtual_files) {
		# Check if this is an XMB
		if ($filename =~ /\.xmb$/i) {
			# If so, check for a corresponding XML:
			my $xmlfilename = $filename;
			if ($xmlfilename =~ s/_([0-9a-f]{16})$XMB_VERSION\.xmb$/.xml/o) {
				my $date_size = $1;
				if (exists $virtual_files->{$xmlfilename}
					and sprintf('%08x%08x', get_mtime($virtual_files->{$xmlfilename}), get_size($virtual_files->{$xmlfilename})) eq $date_size)
				{
					# It exists - fine
					next;
				}
			}
			# No matching XML found, so remove this XMB
			delete $virtual_files->{$filename};
		}
	}

=pod
	# Check for XMLs that don't have XMBs:

	for my $filename (keys %$virtual_files) {
		if ($filename =~ /\.xml$/i) {
			my $date_size = sprintf('%08x%08x', get_mtime($virtual_files->{$filename}), get_size($virtual_files->{$filename}));
			my $xmbfilename = $filename;
			$xmbfilename =~ s/\.xml$/_$date_size$XMB_VERSION.xmb/;
			if (not exists $virtual_files->{$xmbfilename}) {
				print "No XMB found for $filename\n";
			}
		}
	}
=cut
}




### File list sorting code ###


# Takes the $virtual_files hash, and returns an array of the files in an appropriate order
sub sort_files {
	my ($virtual_files, $logsdir) = @_;

	my %unhandled_files = %$virtual_files;

	my @filelist = read_filelist($logsdir);

	my @sorted_files;

	# Add files to @sorted_files, in the order they appear in @filelist
	for (@filelist) {
		if (exists $virtual_files->{$_}) {
			push @sorted_files, [ $_, $virtual_files->{$_} ];
			delete $virtual_files->{$_};
		}
	}

	# Add any remaining files (which weren't referenced by the filelists), just sorted alphabetically
	push @sorted_files, map [ $_, $virtual_files->{$_} ], sort keys %$virtual_files;

	return \@sorted_files;
}

# Returns a list of files in the order that they're loaded by the engine
sub read_filelist {
	my ($logsdir) = @_;

	my @files;

	# TODO: Multiple file-lists
	open my $f, '<', "$logsdir/filelist.txt" or die "Error opening $logsdir/filelist.txt: $!";
	while (<$f>) {
		chomp;
		push @files, $_;
	}
	return @files;
}




### Archive building code ###


# Builds an archive from the list of $files
sub create_archive {
	my ($filename, $files) = @_;

#	print "$_->[0]\n" for @$files; return;

	my $zip = new Archive::Zip;

	for my $file (@$files) {
		if ($file->[1]->archive) {
			$zip->addMember($file->[1]->location) or die "Error adding zipped file $file->[0] to zip";
		} else {
			$zip->addFile($file->[1]->location, $file->[0]) or die "Error adding file $file->[0] to zip";
		}
	}

	my $err = $zip->overwriteAs($filename); $err == Archive::Zip::AZ_OK or die "Error saving zip file $filename ($err)";
}
use strict;
use warnings;

use File::Find;

++$|;

# Relative to build/bin or build/xmbcleanup
my $dir = '../../binaries/data/mods/official';

my @xmlfiles;
find({
	wanted => sub { push @xmlfiles, $File::Find::name if /\.xml$/; $File::Find::prune=1 if $_ eq '.svn' },
 }, $dir);

my $count = 0;

# First, delete old-style XMB files
for (@xmlfiles) {
	# Turn "etc.xml" into "etc.xmb"
	(my $f = $_) =~ s/\.xml$/.xmb/ or die "Internal error: invalid filename '$_'";
	# If it exists, delete it
	if (-e $f) {
		print "Deleting $f\n";
		unlink $f;
		++$count;
	}
}

for (@xmlfiles) {
	# Turn "etc.xml" into "etc_<'?' x 17>.xmb"
	(my $f = $_) =~ s/\.xml$/_?????????????????.xmb/ or die "Internal error: invalid filename '$_'";
	$f =~ s/ /\\ /g;

	# Find all such files
	my @xmbfiles = glob $f;

	# If there are two or more, delete all but the newest
	if (@xmbfiles > 1) {
		@xmbfiles = sort @xmbfiles; # files are "etc_<TIMESTAMP><SIZE><VERSION>xmb", with TIMESTAMP
									#  hex-encoded, so sorting will put the oldest files first.
		# Remove the newest
		pop @xmbfiles;
		# Delete the rest
		print "DELETING @xmbfiles\n";
		unlink @xmbfiles;
		$count += @xmbfiles;
	}
}

print "\n\nCompleted - $count ".($count==1?'file':'files')." deleted. Press enter to exit.";
<>
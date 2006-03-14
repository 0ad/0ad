use DDS;
use Data::Dumper;
use File::Find;

my @dds;
find({
	wanted => sub {
		if (/(\.svn|CVS)$/)
		{
			$File::Find::prune = 1;
		}
		else
		{
			push @dds, $File::Find::name if /\.dds$/;
		}
	},
	no_chdir => 1,
}, "../../../binaries/data/mods/official/art/textures/");


=pod
for my $f (@dds)
{
	my $dds = new DDS($f);
	print "$f\t", $dds->getType(), "\n";
}
=cut

#=pod
my @c;
for my $f (@dds)
{
	my $dds = new DDS($f);
	if ($dds->getType() eq 'ARGB')
	{
		$f =~ /(.*).dds/ or die;
		push @c, $1;
	}
}
print "textureconv -tga ".(join ' ', map "$_.dds", @c)."\n";
print "textureconv -abgr ".(join ' ', map "$_.tga", @c)."\n";
=cut

=pod
my $dds = new DDS("../../../binaries/data/mods/official/art/textures/ui/session/status_pane.dds");
print Dumper $dds;
=cut
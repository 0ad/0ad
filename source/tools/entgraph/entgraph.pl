use strict;
use warnings;

use File::Find;
use Graph;
use Graph::Writer::Dot;

my $dir = '../../../binaries/data/mods/official/entities';
my @xml;
find({wanted=>sub{
	push @xml, $_ if /\.xml$/;
}, no_chdir=>1}, $dir);
s~\Q$dir/~~ for @xml;

my $graph = new Graph;

for my $f (@xml) {

	my $parent;
	open I, "$dir/$f" or die "error opening $dir/$f: $!";
	while (<I>) {
		$parent = $1 if /Parent="(.*?)"/;
	}
	close I;
	
	$f =~ m~(?:.*/|^)(.*)\.xml~ or die "invalid filename $f";
	my $name = $1;
	
	if (defined $parent) {
		$graph->add_edge($parent, $name);
	} else {
		$graph->add_vertex($name);
	}
}

Graph::Writer::Dot->new()->write_graph($graph, "entities.dot");

system("dot.exe", "-Tpng", "entities.dot", "-o", "entities.png");
use strict;
use warnings;

use File::Find;

my $dir = '../../../binaries/data/mods/official/entities';
my @xml;
find({wanted=>sub{
	push @xml, $_ if /\.xml$/;
}, no_chdir=>1}, $dir);
s~\Q$dir/~~ for @xml;

my %nodes;

for my $f (@xml) {

	$f =~ m~(?:.*/|^)(.*)\.xml~ or die "invalid filename $f";
	my $name = $1;

	open I, "$dir/$f" or die "error opening $dir/$f: $!";
	my $data = do { local $/; <I> };
	close I;

	my $parent;
	$parent = $1 if $data =~ /Parent="(.*?)"/;
	
	my ($upgrade, $rank);
	$upgrade = $1 if $data =~ /<Entity>\s*(.*?)\s*</s;
	$rank = $1 if $data =~ /Up.*rank="(.*?)"/s;

	my $actor;
	$actor = $1 if $data =~ /<Actor>\s*(.*?)\s*</;
	
	undef $upgrade unless defined $upgrade and length $upgrade;

	$nodes{$parent} ||= {} if defined $parent;
	$nodes{$upgrade} ||= {} if defined $upgrade;
	$nodes{$name} = { def=>1, parent=>$parent, upgrade=>[$upgrade, $rank], actor=>$actor };
}

open O, ">", "entities.dot" or die $!;

print O <<EOF;
digraph g
{
  graph [nodesep=.1];
  edge [fontname=ArialN fontsize=8];
EOF

print O "  /* entities without actors */
  node [fontname=ArialN fontsize=10 shape=ellipse];
";
for (sort keys %nodes) {
	print O qq{  "$_";\n} if keys %{$nodes{$_}} and not defined $nodes{$_}{actor};
}

print O "  /* entities with actors */
  node [shape=box];
";
for (sort keys %nodes) {
	print O qq{  "$_";\n} if keys %{$nodes{$_}} and defined $nodes{$_}{actor};
}

print O "  /* undefined entities */
  node [color=red];
";
for (sort keys %nodes) {
	print O qq{  "$_";\n} if not keys %{$nodes{$_}};
}

print O "\n  /* inheritance edges */\n";
for (sort keys %nodes) {
	print O qq{  "$nodes{$_}{parent}" -> "$_";\n} if defined $nodes{$_}{parent};
}

print O "\n  /* upgrade edges */\n";
print O "  edge [color=red fontcolor=red]\n";
for (sort keys %nodes) {
	if (defined $nodes{$_}{upgrade}[0]) {
		print O qq{  "$_" -> "$nodes{$_}{upgrade}[0]"};
		print O qq{ [label="from rank $nodes{$_}{upgrade}[1]"]} if defined $nodes{$_}{upgrade}[1];
		print O qq{;\n};
	}
}
print O "}\n";
close O;

system("dot.exe", "-Tpng", "entities.dot", "-o", "entities.png");
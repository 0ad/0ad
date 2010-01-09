use strict;
use warnings;

use File::Find;
use XML::Simple;
use Data::Dumper;

my $dir = '../../../binaries/data/mods/public/entities';
my $dir2 = '../../../binaries/data/mods/public/simulation/templates';
my @xml;
find({ wanted => sub {
    push @xml, $_ if /\.xml$/ and !/template_entity(|_full|_quasi)\.xml$/;
}, no_chdir => 1 }, $dir);
s~\Q$dir/~~ for @xml;

my %xml = ('template_entity_full.xml' => 1, 'template_entity_quasi.xml' => 1);
$xml{$_} = 1 for @xml;

my (%dot_actor, %dot_inherit);

for my $xml (@xml) {
    print "$xml\n";

    my $name = $xml;
    $name =~ s/\.xml$//;

    my %opt = (KeyAttr => []);

    my $data = XMLin("$dir/$xml", %opt, ForceArray => 1);
    my $c = convert($name, $data);
    my $out = $c;
    print "$out\n\n";
    open my $fo, "> $dir2/$xml" or die $!; print $fo $out;
}

sub convert {
    my ($name, $data) = @_;
    #print Dumper $data;
    my $out = qq{<?xml version="1.0" encoding="utf-8"?>\n};

    my $i = "  ";

    $out .= qq{<Entity};
    if ($data->{Parent}) {
        my $p = $data->{Parent};
        $p = "units/$p" if $p =~ /^(hele|celt)_(cavalry|infantry)/;
        warn "Unknown parent $p\n" unless $xml{"$p.xml"};
        $out .= qq{ parent="$p"};
        $dot_inherit{$name}{$p} = 1;
    }
    $out .= qq{>\n};

    $dot_actor{$name} = $data->{Actor};

    if ($data->{Actor}) {
        $out .= qq{$i<VisualActor>\n};
        $out .= qq{$i$i<Actor>$data->{Actor}[0]</Actor>\n};
        $out .= qq{$i</VisualActor>\n};
    }

    $out .= qq{</Entity>\n};
    return $out;
}

open my $dot, '> entities.dot' or die $!;
print $dot <<EOF;
digraph g
{
  graph [nodesep=.1];
  node [fontname=ArialN fontsize=8];
  node [shape=rectangle];
EOF
for (sort grep { not $dot_actor{$_} } keys %dot_actor) {
    print $dot qq{"$_";\n};
}
print $dot qq{node [style=filled fillcolor=lightgray]\n};
for (sort grep { $dot_actor{$_} } keys %dot_actor) {
    print $dot qq{"$_";\n};
}
print $dot qq{node [style=solid shape=ellipse]\n};
for (sort grep { $dot_actor{$_} } keys %dot_actor) {
    print $dot qq{"$dot_actor{$_}[0]" -> "$_";\n};
}
for my $p (sort keys %dot_inherit) {
    for my $c (sort keys %{$dot_inherit{$p}}) {
        print $dot qq{"$p" -> "$c";\n};
    }
}
print $dot "}\n";

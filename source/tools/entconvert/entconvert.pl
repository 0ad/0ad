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
#    print "$out\n\n";
    open my $fo, "> $dir2/$xml" or die $!; print $fo $out;
}

sub convert {
    my ($name, $data) = @_;
    #print Dumper $data if $name eq 'template_unit_infantry';
    my $out = qq{<?xml version="1.0" encoding="utf-8"?>\n};

    my $i = "  ";

    $out .= qq{<Entity};
    if ($data->{Parent}) {
        my $p = $data->{Parent};
        $p = "units/$p" if $p =~ /^(celt|cart|hele|iber|pers|rome)_(cavalry|infantry)/;
        warn "Unknown parent $p\n" unless $xml{"$p.xml"};
        $out .= qq{ parent="$p"};
        $dot_inherit{$name}{$p} = 1;
    }
    $out .= qq{>\n};

    my $civ;
    $civ = $1 if $name =~ /^units\/([a-z]{4})_/;
    my $needs_explicit_civ = ($civ and $data->{Parent} !~ /^${civ}_/);
    if ($data->{Traits}[0]{Id} or $needs_explicit_civ) {
        $out .= qq{$i<Identity>\n};
        $out .= qq{$i$i<Civ>$civ</Civ>\n} if $needs_explicit_civ;
        my @map = (
            [Generic => 'GenericName'],
            [Specific => 'SpecificName'],
            [Icon => 'IconSheet'],
            [Icon_Cell => 'IconCell'],
        );
        for my $m (@map) {
            $out .= qq{$i$i<$m->[1]>$data->{Traits}[0]{Id}[0]{$m->[0]}[0]</$m->[1]>\n} if $data->{Traits}[0]{Id}[0]{$m->[0]};
        }
        for my $k (keys %{$data->{Traits}[0]{Id}[0]}) {
            next if $k =~ /^(Civ)$/; # we do civ based on the filename instead, since it's more reliable
            next if $k =~ /^(Tooltip|History|Internal_Only|Classes|Rollover|Civ_Code)$/; # TODO: convert these somehow
            warn "Unrecognised field <Id><$k>" unless grep $_->[0] eq $k, @map;
        }
        $out .= qq{$i</Identity>\n};
    }

    if ($name eq 'template_unit') {
        $out .= qq{$i<UnitMotion/>\n};
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>1</Population>\n};
        $out .= qq{$i</Cost>\n};
    }

    if ($data->{Traits}[0]{Population}) {
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>$data->{Traits}[0]{Population}[0]{Rem}[0]</Population>\n} if $data->{Traits}[0]{Population}[0]{Rem}
            and $data->{Traits}[0]{Population}[0]{Rem}[0] != 1;
        $out .= qq{$i$i<PopulationBonus>$data->{Traits}[0]{Population}[0]{Add}[0]</PopulationBonus>\n} if $data->{Traits}[0]{Population}[0]{Add};
        $out .= qq{$i</Cost>\n};
    }

    $dot_actor{$name} = $data->{Actor};

    if ($data->{Actor}) {
        $out .= qq{$i<VisualActor>\n};
        $out .= qq{$i$i<Actor>$data->{Actor}[0]</Actor>\n};
        $out .= qq{$i</VisualActor>\n};
    }

    if ($data->{Actions}[0]{Create}[0]{List}[0]{StructCiv} or $data->{Actions}[0]{Create}[0]{List}[0]{StructMil}) {
        $out .= qq{$i<Builder>\n};
        $out .= qq{$i$i<Entities datatype="tokens">\n};
        for (sort (keys %{$data->{Actions}[0]{Create}[0]{List}[0]{StructCiv}[0]}, keys %{$data->{Actions}[0]{Create}[0]{List}[0]{StructMil}[0]})) {
            my $n = "structures/" . ($civ || "{civ}") . "_" . (lc $_);
            $out .= qq{$i$i$i$n\n};
        }
        $out .= qq{$i$i</Entities>\n};
        $out .= qq{$i</Builder>\n};
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

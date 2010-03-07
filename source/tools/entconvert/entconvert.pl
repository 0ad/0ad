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
        $out .= qq{$i<UnitAI/>\n};
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>1</Population>\n};
        $out .= qq{$i</Cost>\n};
    }

    if ($data->{Traits}[0]{Population} or $data->{Traits}[0]{Creation}[0]{Resource}) {
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>$data->{Traits}[0]{Population}[0]{Rem}[0]</Population>\n} if $data->{Traits}[0]{Population}[0]{Rem}
            and $data->{Traits}[0]{Population}[0]{Rem}[0] != 1;
        $out .= qq{$i$i<PopulationBonus>$data->{Traits}[0]{Population}[0]{Add}[0]</PopulationBonus>\n} if $data->{Traits}[0]{Population}[0]{Add};
        if ($data->{Traits}[0]{Creation}[0]{Resource}) {
            $out .= qq{$i$i<Resources>\n};
            for (qw(Food Wood Stone Metal)) {
                $out .= qq{$i$i$i<\l$_>$data->{Traits}[0]{Creation}[0]{Resource}[0]{$_}[0]</\l$_>\n} if $data->{Traits}[0]{Creation}[0]{Resource}[0]{$_}[0];
            }
            $out .= qq{$i$i</Resources>\n};
        }
        $out .= qq{$i</Cost>\n};
    }

    if ($data->{Traits}[0]{Supply} and $name =~ /template_gaia/) {
        $out .= qq{$i<Selectable/>\n};
    }

    if ($data->{Traits}[0]{Supply}) {
        $out .= qq{$i<ResourceSupply>\n};
        $out .= qq{$i$i<Amount>$data->{Traits}[0]{Supply}[0]{Max}[0]</Amount>\n};
        $out .= qq{$i$i<Type>$data->{Traits}[0]{Supply}[0]{Type}[0]</Type>\n};
        $out .= qq{$i$i<Subtype>$data->{Traits}[0]{Supply}[0]{SubType}[0]</Subtype>\n} if $data->{Traits}[0]{Supply}[0]{SubType};
        $out .= qq{$i</ResourceSupply>\n};
    }

    if ($data->{Actions}[0]{Gather}) {
        $out .= qq{$i<ResourceGatherer>\n};
        $out .= qq{$i$i<BaseSpeed>$data->{Actions}[0]{Gather}[0]{Speed}[0]</BaseSpeed>\n};
        if ($data->{Actions}[0]{Gather}[0]{Resource}) {
            $out .= qq{$i$i<Rates>\n};
            my $r = $data->{Actions}[0]{Gather}[0]{Resource}[0];
            for my $t (sort keys %$r) {
                if (ref $r->{$t}[0]) {
                    for my $s (sort keys %{$r->{$t}[0]}) {
                        $out .= qq{$i$i$i<\L$t.$s>$r->{$t}[0]{$s}[0]</$t.$s>\n};
                    }
                } else {
                    $out .= qq{$i$i$i<\L$t>$r->{$t}[0]</$t>\n};
                }
            }
            $out .= qq{$i$i</Rates>\n};
        }
        $out .= qq{$i</ResourceGatherer>\n};
    }

    if ($data->{Traits}[0]{Health}) {
        $out .= qq{$i<Health>\n};
        $out .= qq{$i$i<Max>$data->{Traits}[0]{Health}[0]{Max}[0]</Max>\n} if $data->{Traits}[0]{Health}[0]{Max};
        $out .= qq{$i$i<RegenRate>$data->{Traits}[0]{Health}[0]{RegenRate}[0]</RegenRate>\n} if $data->{Traits}[0]{Health}[0]{RegenRate};
        $out .= qq{$i</Health>\n};
    }

    if ($data->{Traits}[0]{Armour}) {
        $out .= qq{$i<Armour>\n};
        for my $n (qw(Hack Pierce Crush)) {
            $out .= qq{$i$i<$n>$data->{Traits}[0]{Armour}[0]{$n}[0]</$n>\n} if $data->{Traits}[0]{Armour}[0]{$n};
        }
        $out .= qq{$i</Armour>\n};
    }

    if ($data->{Actions}[0]{Move}) {
        $out .= qq{$i<UnitMotion>\n};
        $out .= qq{$i$i<WalkSpeed>$data->{Actions}[0]{Move}[0]{Speed}[0]</WalkSpeed>\n} if $data->{Actions}[0]{Move}[0]{Speed};
        $out .= qq{$i</UnitMotion>\n};
    }

    die if $data->{Actions}[0]{Attack}[0]{Melee} and $data->{Actions}[0]{Attack}[0]{Ranged}; # only allow one at once
    my $attack = $data->{Actions}[0]{Attack}[0]{Melee} || $data->{Actions}[0]{Attack}[0]{Ranged};
    if ($attack) {
        $out .= qq{$i<Attack>\n};
        for my $n (qw(Hack Pierce Crush Range MinRange ProjectileSpeed)) {
            $out .= qq{$i$i<$n>$attack->[0]{$n}[0]</$n>\n} if $attack->[0]{$n};
        }
        if ($attack->[0]{Speed}) {
            my $s = $attack->[0]{Speed}[0];
            # TODO: are these values sane?
            if ($s eq '1000') {
                $out .= qq{$i$i<PrepareTime>600</PrepareTime>\n};
                $out .= qq{$i$i<RepeatTime>1000</RepeatTime>\n};
            } elsif ($s eq '1500' or $s eq '1520' or $s eq '1510') {
                $out .= qq{$i$i<PrepareTime>900</PrepareTime>\n};
                $out .= qq{$i$i<RepeatTime>1500</RepeatTime>\n};
            } elsif ($s eq '2000') {
                $out .= qq{$i$i<PrepareTime>1200</PrepareTime>\n};
                $out .= qq{$i$i<RepeatTime>2000</RepeatTime>\n};
            } else {
                die $s;
            }
        }
        $out .= qq{$i</Attack>\n};
    }

    $dot_actor{$name} = $data->{Actor};

    if ($data->{Actor}) {
        $out .= qq{$i<VisualActor>\n};
        $out .= qq{$i$i<Actor>$data->{Actor}[0]</Actor>\n};
        $out .= qq{$i</VisualActor>\n};
    }

    if ($data->{Traits}[0]{Footprint}) {
        $out .= qq{$i<Footprint>\n};
        if ($data->{Traits}[0]{Footprint}[0]{Radius}) {
            $out .= qq{$i$i<Circle radius="$data->{Traits}[0]{Footprint}[0]{Radius}[0]"/>\n};
        }
        if ($data->{Traits}[0]{Footprint}[0]{Width}) {
            $out .= qq{$i$i<Square width="$data->{Traits}[0]{Footprint}[0]{Width}[0]" depth="$data->{Traits}[0]{Footprint}[0]{Depth}[0]"/>\n};
        }
        if ($data->{Traits}[0]{Footprint}[0]{Height}) {
            $out .= qq{$i$i<Height>$data->{Traits}[0]{Footprint}[0]{Height}[0]</Height>\n};
        }
        $out .= qq{$i</Footprint>\n};
    }

    if ($name =~ /^template_(structure|gaia)$/) {
        $out .= qq{$i<Obstruction/>\n};
    }

    if ($name =~ /^template_structure_resource_field$/) {
        $out .= qq{$i<Obstruction disable=""/>\n};
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

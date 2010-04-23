use strict;
use warnings;

use File::Find;
use XML::Simple;
use Data::Dumper;

my $vfsroot = '../../../binaries/data/mods';

my (%dot_actor, %dot_inherit);
my %xml = ('template_entity_full.xml' => 1, 'template_entity_quasi.xml' => 1);

convert_all("$vfsroot/public");
convert_all("$vfsroot/internal") if -e "$vfsroot/internal";

sub convert_all {
    my ($root) = @_;

    my $dir = "$root/entities";
    my $dir2 = "$root/simulation/templates";
    my @xml;
    find({ wanted => sub {
        push @xml, $_ if /\.xml$/ and !/template_entity(|_full|_quasi)\.xml$/;
    }, no_chdir => 1 }, $dir);
    s~\Q$dir/~~ for @xml;

    $xml{$_} = 1 for @xml;

    for my $xml (@xml) {
        print "  $xml\n";

        my $name = $xml;
        $name =~ s/\.xml$//;

        next if $name =~ /^(template_foundation|foundation_)/;

        my %opt = (KeyAttr => []);

        my $data = XMLin("$dir/$xml", %opt, ForceArray => 1);
        my $c = convert($name, $data);
        my $out = $c;
        open my $fo, "> $dir2/$xml" or die $!;
        binmode $fo, ':utf8';
        print $fo $out;
    }
}

sub convert {
    my ($name, $data) = @_;
    #print Dumper $data if $name eq 'template_unit_infantry';
    my $out = qq{<?xml version="1.0" encoding="utf-8"?>\n};

    my $i = "  ";

    $out .= qq{<Entity};
    if ($data->{Parent}) {
        my $p = $data->{Parent};
        $p = "units/$p" if $p =~ /^(celt|cart|hele|iber|pers|rome)_(cavalry|infantry|support)/;
        $p = "structures/$p" if $p =~ /^(celt|cart|hele|iber|pers|rome)_scout_tower/;
        warn "Unknown parent $p\n" unless $xml{"$p.xml"};
        $out .= qq{ parent="$p"};
        $dot_inherit{$name}{$p} = 1;
    }
    $out .= qq{>\n};

    my $civ;
    $civ = 'hele' if $name =~ /^other\/camp_(mace_hypaspist|sparta_phalangite)$/;
    $civ = 'gaia' if $name =~ /^(template_gaia|template_structure_gaia_settlement|fence_(long|short))$/ or ($name =~ /^gaia\/fauna_/ and $data->{Parent} !~ /^template_gaia/);
    $civ = $1 if $name =~ /^(?:units|structures)\/([a-z]{4})_/;
    $civ = $1 if $name =~ /^other\/camp_(pers|rome)_/;
    my $needs_explicit_civ = ($civ and $data->{Parent} !~ /^${civ}_/);
    if ($data->{Traits}[0]{Id} or $needs_explicit_civ) {
        if (not $civ and $name !~ /^template/ and $data->{Parent} =~ /^template/ and $data->{Traits}[0]{Id}[0]{Civ}) {
            $civ = ({ Hellenes => 'hele', Objects => 'gaia', Romans => 'rome'})->{ $data->{Traits}[0]{Id}[0]{Civ}[0] } or die;
            $needs_explicit_civ = 1;
        }
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

    if ($data->{Traits}[0]{MiniMap}) {
        $out .= qq{$i<Minimap>\n};
        $out .= qq{$i$i<Type>}.lc($data->{Traits}[0]{MiniMap}[0]{Type}[0]).qq{</Type>\n}
            if $data->{Traits}[0]{MiniMap}[0]{Type};
        $out .= qq{$i$i<Colour r="$data->{Traits}[0]{MiniMap}[0]{Red}[0]" g="$data->{Traits}[0]{MiniMap}[0]{Green}[0]" b="$data->{Traits}[0]{MiniMap}[0]{Blue}[0]"/>\n} #"
            if $data->{Traits}[0]{MiniMap}[0]{Red};
        $out .= qq{$i</Minimap>\n};
    }

    if ($name eq 'template_structure') {
        $out .= qq{$i<Minimap>\n};
        $out .= qq{$i$i<Type>structure</Type>\n};
        $out .= qq{$i</Minimap>\n};
    }

    if ($name eq 'template_unit') {
        $out .= qq{$i<UnitAI/>\n};
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>1</Population>\n};
        $out .= qq{$i$i<Resources>\n};
        $out .= qq{$i$i$i<food>0</food>\n};
        $out .= qq{$i$i$i<wood>0</wood>\n};
        $out .= qq{$i$i$i<stone>0</stone>\n};
        $out .= qq{$i$i$i<metal>0</metal>\n};
        $out .= qq{$i$i</Resources>\n};
        $out .= qq{$i</Cost>\n};
    }

    if ($data->{Traits}[0]{Population} or $data->{Traits}[0]{Creation}[0]{Resource} or $data->{Traits}[0]{Creation}[0]{Time}) {
        $out .= qq{$i<Cost>\n};
        $out .= qq{$i$i<Population>$data->{Traits}[0]{Population}[0]{Rem}[0]</Population>\n} if defined $data->{Traits}[0]{Population}[0]{Rem}
            and $data->{Traits}[0]{Population}[0]{Rem}[0] != 1;
        $out .= qq{$i$i<PopulationBonus>$data->{Traits}[0]{Population}[0]{Add}[0]</PopulationBonus>\n} if defined $data->{Traits}[0]{Population}[0]{Add};
        $out .= qq{$i$i<BuildTime>$data->{Traits}[0]{Creation}[0]{Time}[0]</BuildTime>\n} if defined $data->{Traits}[0]{Creation}[0]{Time};
        if ($data->{Traits}[0]{Creation}[0]{Resource}) {
            $out .= qq{$i$i<Resources>\n};
            for (qw(Food Wood Stone Metal)) {
                $out .= qq{$i$i$i<\l$_>$data->{Traits}[0]{Creation}[0]{Resource}[0]{$_}[0]</\l$_>\n} if defined $data->{Traits}[0]{Creation}[0]{Resource}[0]{$_}[0];
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
        my $t = $data->{Traits}[0]{Supply}[0]{Type}[0];
        $t .= '.'.$data->{Traits}[0]{Supply}[0]{SubType}[0] if $data->{Traits}[0]{Supply}[0]{SubType};
        $out .= qq{$i$i<Type>$t</Type>\n};
        $out .= qq{$i</ResourceSupply>\n};
    }

    if ($data->{Actions}[0]{Gather}) {
        $out .= qq{$i<ResourceGatherer>\n};
        my $speed = sprintf '%.1f', $data->{Actions}[0]{Gather}[0]{Speed}[0]/1000;
        $out .= qq{$i$i<BaseSpeed>$speed</BaseSpeed>\n};
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
        $out .= qq{$i$i<DeathType>corpse</DeathType>\n} if $name eq 'template_unit';
        $out .= qq{$i$i<DeathType>vanish</DeathType>\n} if $name =~ /^(template_structure|other\/fence_(long|short))$/;
        $out .= qq{$i$i<Max>$data->{Traits}[0]{Health}[0]{Max}[0]</Max>\n} if $data->{Traits}[0]{Health}[0]{Max};
        $out .= qq{$i$i<RegenRate>$data->{Traits}[0]{Health}[0]{RegenRate}[0]</RegenRate>\n} if $data->{Traits}[0]{Health}[0]{RegenRate};
        $out .= qq{$i</Health>\n};
    }

    if ($data->{Traits}[0]{Armour}) {
        $out .= qq{$i<Armour>\n};
        for my $n (qw(Hack Pierce Crush)) {
            $out .= qq{$i$i<$n>$data->{Traits}[0]{Armour}[0]{$n}[0]</$n>\n} if defined $data->{Traits}[0]{Armour}[0]{$n};
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
            my $e = $n;
            $e = 'MaxRange' if $e eq 'Range';
            $out .= qq{$i$i<$e>$attack->[0]{$n}[0]</$e>\n} if $attack->[0]{$n};
            $out .= qq{$i$i<$e>0.0</$e>\n} if $n eq 'MinRange' and
                $name =~ /^(template_unit_(cavalry_melee|hero|infantry_melee|mechanical_siege_(ballista|onager|ram)|super_(cavalry|infantry))|units\/celt_support_female_citizen)$/;
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

    if ($data->{Actor} or $data->{Traits}[0]{Creation}[0]{Foundation}) {
        $out .= qq{$i<VisualActor>\n};
        $out .= qq{$i$i<Actor>$data->{Actor}[0]</Actor>\n} if $data->{Actor};
        if ($data->{Traits}[0]{Creation}[0]{Foundation}) {
            $data->{Traits}[0]{Creation}[0]{Foundation}[0] =~ /^foundation_(\d+x\d+|theatron|field)$/ or die $data->{Traits}[0]{Creation}[0]{Foundation}[0];
            my $actor = ($1 eq 'field' ? 'structures/plot_field_found.xml' : "structures/fndn_$1.xml");
            $out .= qq{$i$i<FoundationActor>$actor</FoundationActor>\n};
        }
        $out .= qq{$i</VisualActor>\n};
    }

    if ($data->{Traits}[0]{Footprint}) {
        my $r = '';
        $r = ' replace=""' if
            ($data->{Traits}[0]{Footprint}[0]{Width} and $data->{Parent} =~ /^template_(unit|unit_mechanical|unit_mechanical_siege|unit_mechanical_siege_onager|unit_(super|hero)_ranged)$/) or
            ($data->{Traits}[0]{Footprint}[0]{Radius} and $data->{Parent} =~ /^template_structure_(special|military_fortress)$/);
        $out .= qq{$i<Footprint$r>\n};
        if ($data->{Traits}[0]{Footprint}[0]{Radius}) {
            $out .= qq{$i$i<Circle radius="$data->{Traits}[0]{Footprint}[0]{Radius}[0]"/>\n};
        }
        if ($data->{Traits}[0]{Footprint}[0]{Width}) {
            $out .= qq{$i$i<Square width="$data->{Traits}[0]{Footprint}[0]{Width}[0]" depth="$data->{Traits}[0]{Footprint}[0]{Depth}[0]"/>\n}; #"
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
        $out .= qq{$i$i<Rate>1.0</Rate>\n} if $data->{Actions}[0]{Build};
#        $out .= qq{$i$i<Entities datatype="tokens">\n};
        $out .= qq{$i$i<Entities>\n};
        for (sort (keys %{$data->{Actions}[0]{Create}[0]{List}[0]{StructCiv}[0]}, keys %{$data->{Actions}[0]{Create}[0]{List}[0]{StructMil}[0]})) {
            my $n = "structures/" . ($civ || "{civ}") . "_" . (lc $_);
            $out .= qq{$i$i$i$n\n};
        }
        $out .= qq{$i$i</Entities>\n};
        $out .= qq{$i</Builder>\n};
    }

    if ($data->{Actions}[0]{Create}[0]{List}[0]{Train}) {
        $out .= qq{$i<TrainingQueue>\n};
#        $out .= qq{$i$i<Entities datatype="tokens">\n};
        $out .= qq{$i$i<Entities>\n};
        for (sort (keys %{$data->{Actions}[0]{Create}[0]{List}[0]{Train}[0]})) {
            my $n = "units/" . ($civ || "{civ}") . "_" . (lc $_);
            $out .= qq{$i$i$i$n\n};
        }
        $out .= qq{$i$i</Entities>\n};
        $out .= qq{$i</TrainingQueue>\n};
    }

    if ($data->{SoundGroups}) {
        $out .= qq{$i<Sound>\n};
        $out .= qq{$i$i<SoundGroups>\n};
        for my $n (qw(Walk Run Melee Death Build Gather_Fruit Gather_Grain Gather_Wood Gather_Stone Gather_Metal)) {
            my $n2 = lc $n;
            if ($n2 eq 'melee') { $n2 = 'attack'; }
            if ($data->{SoundGroups}[0]{$n}) {
                my $f = $data->{SoundGroups}[0]{$n}[0];
                $f =~ s~^audio/~~ or die;
                $out .= qq{$i$i$i<$n2>$f</$n2>\n};
            }
        }
        $out .= qq{$i$i</SoundGroups>\n};
        $out .= qq{$i</Sound>\n};
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

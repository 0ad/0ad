#!/usr/bin/perl

# Generates a graph of the interdependencies between simulation components

use strict;
use warnings;

use File::Find;
use Data::Dumper;

my %implements;
my %implementedby;
my %subscribes;
my %queries;
my %posts;
my %native;

for (<../../simulation2/components/CCmp*>)
{
    next if /CCmpTest/;
    parse_ctype_cpp($_);
}

for (<../../../binaries/data/mods/public/simulation/components/*.js>)
{
    parse_ctype_js($_);
}

parse_helper_js("CommandQueue", "../../../binaries/data/mods/public/simulation/helpers/Commands.js");

#parse_ctype_design("components.txt");

# Add one that the parser misses
$posts{RangeManager}{RangeUpdate} = 1;

dump_stats();

use Data::Dumper; print Dumper \%queries;

dump_graph();
system("dot -Tpng -o components.png components.dot");

sub parse_ctype_cpp
{
    my ($fn) = @_;
    print "$fn ...\n";
    open my $f, $fn or die "can't open $fn: $!";
    my $cmp;
    if ($fn =~ /CCmpPathfinder/) { $cmp = 'Pathfinder'; } # because it's split into multiple .cpp files
    while (<$f>) {
        if (/class CCmp(\S+) : public ICmp(\S+)/) {
            $implements{$1} = $2;
            $implementedby{$2}{$1} = 1;
            $native{$1} = 1;
            $cmp = $1;
        } elsif (/Subscribe(Globally)?ToMessageType\(MT_(\S+)\)/) {
            $subscribes{$2}{$cmp} = 1;
        } elsif (/CmpPtr<ICmp(\S+)>/) {
            $queries{$cmp}{$1} = 1;
        } elsif (/^\s*CMessage(\S+)/) {
            $posts{$cmp}{$1} = 1;
        }
    }
}

sub parse_ctype_js
{
    my ($fn) = @_;
    print "$fn ...\n";
    open my $f, $fn or die "can't open $fn: $!";
    my $cmp;
    while (<$f>) {
        if (/^\s*function (\S+)\s*\(\)\s*\{\s*\}\s*$/) {
            $cmp = $1;
        } elsif ($cmp and /\s*$cmp\.prototype\.On(?:Global)?(\S+)\s*=/) {
            $subscribes{$1}{$cmp} = 1;
        } elsif (/Engine\.QueryInterface\(.*?,\s*IID_(\S+?)\)/) {
            $queries{$cmp}{$1} = 1;
        } elsif (/Engine\.RegisterComponentType\(IID_(\S+), "(\S+)", (\S+)\);/) {
            die unless $2 eq $cmp;
            die unless $3 eq $cmp;
            $implements{$cmp} = $1;
            $implementedby{$1}{$cmp} = 1;
        } elsif (/Engine\.(?:Post|Broadcast)Message\(\S+, MT_(\S+),/) {
            $posts{$cmp}{$1} = 1;
        }
    }
}

sub parse_helper_js
{
    my ($cmp, $fn) = @_;
    open my $f, $fn or die "can't open $fn: $!";
    while (<$f>) {
        if (/Engine\.QueryInterface\(.*?,\s*IID_(\S+)\)/) {
            $queries{$cmp}{$1} = 1;
        }
        # TODO: check for message sending
    }
}

sub parse_ctype_design
{
    my ($fn) = @_;
    open my $f, $fn or die "can't open $fn: $!";
    my $cmp;
    while (<$f>) {
        s/\s*#.*//;
        next unless /\S/;
        if (/^component (\S+)(?: : (\S+))?$/) {
            $implements{$1} = ($2 || $1);
            $implementedby{$2 || $1}{$1} = 1;
            $cmp = $1;
        } elsif (/^native$/) {
            $native{$cmp} = 1;
        } elsif (/^subscribe (\S+)$/) {
            $subscribes{$1}{$cmp} = 1;
        } elsif (/^query (\S+)$/) {
            $queries{$cmp}{$1} = 1;
        } elsif (/^post (\S+)$/) {
            $posts{$cmp}{$1} = 1;
        } else {
            die "Invalid input line: $_ in $fn.";
        }
    }
}

sub dump_graph
{
    open my $f, '>', 'components.dot' or die $!;
    print $f <<EOF;
digraph g {
    graph [ranksep=1 nodesep=0.1 fontsize=10 compound=true];
    node [fontsize=10];
    edge [fontsize=8];
EOF

#     for my $c (sort keys %implements) {
#         print $f "$c;\n";
#     }

    for my $i (sort keys %implementedby) {
        print $f "subgraph cluster_ifc_$i {\n";
        print $f "label=\"$i\";\n";
        for my $c (sort keys %{$implementedby{$i}}) {
            my $col = ($native{$c} ? "green" : "black");
            print $f "$c [color=$col];\n";
        }
        print $f "}\n";
    }

    print $f qq{node [color=gray fontcolor=gray];\n};

    print $f qq{edge [color=blue fontcolor=blue];\n};

    for my $c (sort keys %queries) {
        next if $c eq 'GuiInterface' or $c eq 'CommandQueue'; # these make the graph messy
        for my $t (sort keys %{$queries{$c}}) {
            my $tc = (sort keys %{$implementedby{$t}})[0];
            print $f qq{$c -> $tc [lhead=cluster_ifc_$t];\n};
        }
    }

    print $f qq{edge [color=red fontcolor=red weight=0.9];\n};

    for my $c (sort keys %posts) {
        for my $m (sort keys %{$posts{$c}}) {
            for my $t (sort keys %{$subscribes{$m}}) {
                print $f qq{$c -> $t [label="$m"];\n};
            }
        }
    }

    print $f <<EOF;
}
EOF
}

sub dump_stats
{
    my ($native, $scripted) = (0, 0);
    for my $c (keys %implements) {
        if ($native{$c}) { ++$native; } else { ++$scripted; }
    }
    printf "Native components: %d\nScripted components: %d\nTotal components: %d\n", $native, $scripted, $native+$scripted;
    printf "Interfaces: %d\n", (scalar keys %implementedby);
}

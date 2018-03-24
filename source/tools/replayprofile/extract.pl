#!/usr/bin/perl

use strict;
use warnings;

# Run the game normally to generate a replay.
# Run the non-visual replay like:
#  ./pyrogenesis -mod=public -replay=$HOME/.local/share/0ad/replays/0.0.23/2018-03-23_0010/commands.txt
# to generate profile.txt.
# Then run:
#  perl extract.pl > data.json
# then open graph.html.

open my $f, '../../../binaries/system/profile.txt' or die $!;
my $turn = 0;
my %s;
my %f;
while (<$f>) {
    if (/^PS profiler snapshot/) {
        if ($turn) {
            for (keys %f) {
                $s{$_}[$turn-1] = $f{$_};
            }
        }
        ++$turn;
        %f = ();
    } elsif (/Time in node: ([\d.]+)/) {
        $f{"total"} += $1;
    } elsif (/^[|'][- |']*([^|]+?)\s+\|[^|]+\|\s+(\S+)/) {
        $f{$1} += $2;
    } elsif (/^[|']-([^|]+?)\s+\|\s+(\d+)/) {
        $f{$1} += $2;
    }
}

print "[\n";
my $n = 0;
for my $k (sort keys %s) {
    print ",\n" if $n++;
    print "{label: '$k', data:[";
    for my $t (0..$#{$s{$k}}) {
        print "," if $t;
        print "[$t,".($s{$k}[$t] || 0)."]";
    }
    print "]}";
}
print "\n]\n";

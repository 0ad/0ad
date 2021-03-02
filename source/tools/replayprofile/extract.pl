#!/usr/bin/perl

use strict;
use warnings;

use Getopt::Long;
use File::Basename qw(dirname);
chdir(dirname(__FILE__));

# Run the game normally to generate a replay.
# Run the non-visual replay like:
#  ./pyrogenesis -mod=public -replay=$HOME/.local/share/0ad/replays/0.0.23/2018-03-23_0010/commands.txt
# to generate profile.txt.
# Then run:
#  perl extract.pl
#    OR
#  perl extract.pl --to-json > data.json ...  [manipulation with data.json] ...  perl extract.pl --from-json
# then open graph_onepage.html.

my ($to_json, $from_json); GetOptions ("to-json" => \$to_json, "from-json" => \$from_json);

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

my $json = "[\n";
my $n = 0;
for my $k (sort keys %s) {
    $json .= ",\n" if $n++;
    $json .= "{label: '$k', data:[";
    for my $t (0..$#{$s{$k}}) {
        $json .= "," if $t;
        $json .= "[$t,".($s{$k}[$t] || 0)."]";
    }
    $json .= "]}";
}
$json .= "\n]\n";

my $onepage = "graph_onepage.html";

sub recursive_file_slurp {
	my $filename = shift;
	my $output = '';
	open my $fd, $filename or die "Open $filename : $!";
	$output .= "<script>\n" if $filename =~ /\.js$/;
	while(<$fd>){
		if(/-- include js --/) {
			$output .= recursive_file_slurp("jquery.js");
			$output .= recursive_file_slurp("jquery.flot.js");
			$output .= recursive_file_slurp("jquery.flot.navigate.js");
		}
		elsif(/-- include graph js --/) {
			$output .= recursive_file_slurp("graph.js");
		}
		elsif(/-- include data json --/) {
			$output .= $json;
		}
		else {
			$output .= $_;
		}
	}
	$output .= "</script>\n" if $filename =~ /\.js$/;
	close($fd);
	return $output;
}

if($to_json) {
	print $json;
	exit;
}
elsif($from_json) {
	$json = recursive_file_slurp("data.json");
}

open my $opfd, '>', $onepage or die $!;
print $opfd recursive_file_slurp("graph.html");
close($opfd);

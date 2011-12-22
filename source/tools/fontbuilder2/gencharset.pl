use strict;
use warnings;

use Unicode::UCD;
use Data::Dumper;

binmode STDOUT, ':utf8';

my @cs;

for ( # (these are probably more than we really need)
    'Basic Latin',
    'Latin-1 Supplement',
    'Latin Extended-A',
    'Latin Extended-B',
    'Latin Extended Additional',
    'Spacing Modifier Letters',
    'General Punctuation',
    'Combining Diacritical Marks',
) {
    print "# $_\n";
    for my $r (@{Unicode::UCD::charblock($_)}) {
        for my $c ($r->[0]..$r->[1]) {
            next if $c < 0x20 or ($c >= 0x7f and $c < 0xa0);
            printf "%04X: %s  %s\n", $c, chr($c), Unicode::UCD::charinfo($c)->{name} if Unicode::UCD::charinfo($c);
        }
    }
    print "\n";
}

for my $c (0xFE33, 0xFFFD) {
    printf "%04X: %s  %s\n", $c, chr($c), Unicode::UCD::charinfo($c)->{name} if Unicode::UCD::charinfo($c);
}

use strict;
use warnings;

use File::Find;

my $vfsroot = '../../../binaries/data/mods';

sub find_entities
{
    my @files;
    my $find_process = sub {
        return $File::Find::prune = 1 if $_ eq '.svn';
        my $n = $File::Find::name;
        return if /~$/;
        return unless -f $_;
        push @files, $n;
    };
    find({ wanted => $find_process }, "$vfsroot/public/simulation/templates");
    find({ wanted => $find_process }, "$vfsroot/internal/simulation/templates") if -e "$vfsroot/internal";

    return @files;
}

open my $c, "charset.txt" or die $!;
binmode $c, ":utf8";
my %chars;
@chars{split //, do { local $/; <$c> }} = ();

$chars{"\r"} = undef;
$chars{"\n"} = undef;
$chars{"\t"} = undef;

for my $fn (sort(find_entities()))
{
    open my $f, "<", $fn or die $!;
    binmode $f, ":utf8";
    my %fchars;
    @fchars{split //, do { local $/; <$f> }} = ();
    for (sort keys %fchars)
    {
        if (not exists $chars{$_})
        {
            printf "%s\n# Missing char U+%04X\n\n", $fn, ord $_;
        }
    }
}

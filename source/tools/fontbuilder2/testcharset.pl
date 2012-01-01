# Tests the characters contained within entity template XML files (most usefully
# their <SpecificName>s) against the characters included in the bitmap font subset,
# and against the glyph data in the fonts themselves.
# When unsupported characters are reported, you should either edit the XML files
# to avoid them (e.g. use a codepoint that better corresponds to the character,
# or use a NFC or NFD version of the character) or add them to charset.txt.

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

my %fontchars;
{
    open my $f, 'python dumpfontchars.py|' or die $!;
    while (<$f>)
    {
        my ($name, @chars) = split;
        push @chars, 0x0009, 0x000A, 0x000D;
        @{$fontchars{$name}}{map chr($_), @chars} = ();
    }
}

delete $chars{chr(0x301)};

for my $fn (sort(find_entities()))
{
    open my $f, "<", $fn or die $!;
    binmode $f, ":utf8";
    my %fchars;
    @fchars{split //, do { local $/; <$f> }} = ();
    for (sort keys %fchars)
    {
        my @missing;
        if (not exists $chars{$_})
        {
            push @missing, 'charset';
        }
        for my $font (sort keys %fontchars)
        {
            if (not exists $fontchars{$font}{$_})
            {
                push @missing, $font;
            }
        }
        if (@missing)
        {
            printf "%s\n# Missing char U+%04X from %s\n\n", $fn, ord($_), (join ', ', @missing);
        }
    }
}

print "Done.\n";

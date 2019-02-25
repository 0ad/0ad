use strict;
use warnings;

use XML::LibXML;

use lib ".";
use Entity;

my $rngschema = XML::LibXML::RelaxNG->new(location => '../../../binaries/system/entity.rng');

sub escape_xml
{
    my ($t) = @_;
    $t =~ s/&/&amp;/g;
    $t =~ s/</&lt;/g;
    $t =~ s/>/&gt;/g;
    $t =~ s/"/&quot;/g;
    $t =~ s/\t/&#9;/g;
    $t =~ s/\n/&#10;/g;
    $t =~ s/\r/&#13;/g;
    $t;
}

sub to_xml
{
    my ($e) = @_;
    my $r = $e->{' content'};
    $r = '' if not defined $r;
    for my $k (sort grep !/^[\@ ]/, keys %$e) {
        $r .= "<$k";
        for my $a (sort grep /^\@/, keys %{$e->{$k}}) {
            $a =~ /^\@(.*)/;
            $r .= " $1=\"".escape_xml($e->{$k}{$a}{' content'})."\"";
        }
        $r .= ">";
        $r .= to_xml($e->{$k});
        $r .= "</$k>";
    }
    return $r;
}

sub validate
{
    my ($vfspath) = @_;
    my $xml = to_xml(Entity::load_inherited($vfspath));
    my $doc = XML::LibXML->new->parse_string($xml);
    $rngschema->validate($doc);
}


sub check_all
{
    my @files = Entity::find_entities("public");

    my $count = 0;
    my $failed = 0;
    for my $f (sort @files) {
        next if $f =~ /^template_/;
        print "# $f...\n";
        ++$count;
        eval {
            validate($f);
        };
        if ($@) {
            ++$failed;
            print $@;
            eval { print to_xml(load_inherited($f)), "\n"; }
        }
    }
    print "\nTotal: $count; failed: $failed\n";
}

check_all();

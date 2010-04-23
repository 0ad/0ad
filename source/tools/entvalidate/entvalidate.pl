use strict;
use warnings;

use XML::Parser;
use XML::LibXML;
use Data::Dumper;
use Storable qw(dclone);
use File::Find;

my $vfsroot = '../../../binaries/data/mods';
my $rngschema = XML::LibXML::RelaxNG->new(location => '../../../binaries/system/entity.rng');

sub get_file
{
    my ($vfspath) = @_;
    my $fn = "$vfsroot/public/simulation/templates/$vfspath.xml";
    if (not -e $fn) {
        $fn = "$vfsroot/internal/simulation/templates/$vfspath.xml";
    }
    open my $f, $fn or die "Error loading $fn: $!";
    local $/;
    return <$f>;
}

sub trim
{
    my ($t) = @_;
    return '' if not defined $t;
    $t =~ /^\s*(.*?)\s*$/s;
    return $1;
}

sub load_xml
{
    my ($file) = @_;
    my $root = {};
    my @stack = ($root);
    my $p = new XML::Parser(Handlers => {
        Start => sub {
            my ($e, $n, %a) = @_;
            my $t = {};
            die "Duplicate child node '$n'" if exists $stack[-1]{$n};
            $stack[-1]{$n} = $t;
            for (keys %a) {
                $t->{'@'.$_}{' content'} = trim($a{$_});
            }
            push @stack, $t;
        },
        End => sub {
            my ($e, $n) = @_;
            $stack[-1]{' content'} = trim($stack[-1]{' content'});
            pop @stack;
        },
        Char => sub {
            my ($e, $str) = @_;
            $stack[-1]{' content'} .= $str;
        },
    });
    $p->parse($file);

    return $root;
}

sub apply_layer
{
    my ($base, $new) = @_;
    $base->{' content'} = $new->{' content'};
    for my $k (grep $_ ne ' content', keys %$new) {
        if ($new->{$k}{'@disable'}) {
            delete $base->{$k};
        } else {
            if ($new->{$k}{'@replace'}) {
                delete $base->{$k};
            }
            $base->{$k} ||= {};
            apply_layer($base->{$k}, $new->{$k});
            delete $base->{$k}{'@replace'};
        }
    }
}

sub load_inherited
{
    my ($vfspath) = @_;
    my $layer = load_xml(get_file($vfspath));

    if ($layer->{Entity}{'@parent'}) {
        my $parent = load_inherited($layer->{Entity}{'@parent'}{' content'});
        apply_layer($parent->{Entity}, $layer->{Entity});
        return $parent;
    } else {
        return $layer;
    }
}

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
    my $xml = to_xml(load_inherited($vfspath));
    my $doc = XML::LibXML->new->parse_string($xml);
    $rngschema->validate($doc);
}


sub check_all
{
    my @files;
    my $find_process = sub {
        return $File::Find::prune = 1 if $_ eq '.svn';
        my $n = $File::Find::name;
        return if /~$/;
        return unless -f $_;
        $n =~ s~\Q$vfsroot\E/(public|internal)/simulation/templates/~~;
        $n =~ s/\.xml$//;
        push @files, $n;
    };
    find({ wanted => $find_process }, "$vfsroot/public/simulation/templates");
    find({ wanted => $find_process }, "$vfsroot/internal/simulation/templates") if -e "$vfsroot/internal";

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

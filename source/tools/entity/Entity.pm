package Entity;

use strict;
use warnings;

use XML::Parser;
use Data::Dumper;
use File::Find;

my $vfsroot = '../../../binaries/data/mods';

sub get_filename
{
    my ($vfspath) = @_;
    my $fn = "$vfsroot/public/simulation/templates/$vfspath.xml";
    return $fn;
}

sub get_file
{
    my ($vfspath) = @_;
    my $fn = get_filename($vfspath);
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
    my ($vfspath, $file) = @_;
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
    eval {
        $p->parse($file);
    };
    if ($@) {
        die "Error parsing $vfspath: $@";
    }
    return $root;
}

sub apply_layer
{
    my ($base, $new) = @_;
    if ($new->{'@datatype'} and $new->{'@datatype'}{' content'} eq 'tokens') {
        my @old = split /\s+/, ($base->{' content'} || '');
        my @new = split /\s+/, ($new->{' content'} || '');
        my @t = @old;
        for my $n (@new) {
            if ($n =~ /^-(.*)/) {
                @t = grep $_ ne $1, @t;
            } else {
                push @t, $n if not grep $_ eq $n, @t;
            }
        }
        $base->{' content'} = join ' ', @t;
    } elsif ($new->{'@op'}) {
        my $op = $new->{'@op'}{' content'};
        my $op1 = $base->{' content'};
        my $op2 = $new->{' content'};
        if ($op eq 'add') {
            $base->{' content'} = $op1 + $op2;
        }
        elsif ($op eq 'mul') {
            $base->{' content'} = $op1 * $op2;
        }
        else {
            die "Invalid operator '$op'";
        }
    } else {
        $base->{' content'} = $new->{' content'};
    }
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
    my $layer = load_xml($vfspath, get_file($vfspath));

    if ($layer->{Entity}{'@parent'}) {
        my $parent = load_inherited($layer->{Entity}{'@parent'}{' content'});
        apply_layer($parent->{Entity}, $layer->{Entity});
        return $parent;
    } else {
        return $layer;
    }
}

sub find_entities
{
    my @files;
    my $find_process = sub {
        return $File::Find::prune = 1 if $_ eq '.svn';
        my $n = $File::Find::name;
        return if /~$/;
        return unless -f $_;
        $n =~ s~\Q$vfsroot\E/public/simulation/templates/~~;
        $n =~ s/\.xml$//;
        push @files, $n;
    };
    find({ wanted => $find_process }, "$vfsroot/public/simulation/templates");

    return @files;
}

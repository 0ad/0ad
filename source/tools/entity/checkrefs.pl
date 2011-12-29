use strict;
use warnings;
use Data::Dumper;
use File::Find;
use XML::Simple;

use Entity;

use constant CHECK_SCENARIOS => 0;
use constant ROOT_ACTORS => 1;
use constant INCLUDE_INTERNAL => 1;

my @files;
my @roots;
my @deps;

my $vfsroot = '../../../binaries/data/mods';

sub vfs_to_physical
{
    my ($vfspath) = @_;
    my $fn = "$vfsroot/public/$vfspath";
    if (INCLUDE_INTERNAL and not -e $fn) {
        $fn = "$vfsroot/internal/$vfspath";
    }
    return $fn;
}

sub vfs_to_relative_to_mods
{
    my ($vfspath) = @_;
    my $fn = "public/$vfspath";
    if (INCLUDE_INTERNAL and not -e "$vfsroot/$fn") {
        $fn = "internal/$vfspath";
    }
    return $fn;
}

sub find_files
{
    my ($vfspath, $extn) = @_;
    my @files;
    my $find_process = sub {
        return $File::Find::prune = 1 if $_ eq '.svn';
        my $n = $File::Find::name;
        return if /~$/;
        return unless -f $_;
        return unless /\.($extn)$/;
        $n =~ s~\Q$vfsroot\E/(public|internal)/~~;
        push @files, $n;
    };
    find({ wanted => $find_process }, "$vfsroot/public/$vfspath");
    find({ wanted => $find_process }, "$vfsroot/internal/$vfspath") if INCLUDE_INTERNAL and -e "$vfsroot/internal/$vfspath";

    return @files;
}

sub add_entities
{
    print "Loading entities...\n";

    my @entfiles = find_files('simulation/templates', 'xml');
    s~^simulation/templates/(.*)\.xml$~$1~ for @entfiles;

    for my $f (sort @entfiles)
    {
        my $path = "simulation/templates/$f.xml";
        push @files, $path;
        my $ent = Entity::load_inherited($f);

        push @deps, [ $path, "simulation/templates/" . $ent->{Entity}{'@parent'}{' content'} . ".xml" ] if $ent->{Entity}{'@parent'};

        if ($f !~ /^template_/)
        {
            push @roots, $path;
            if ($ent->{Entity}{VisualActor})
            {
                push @deps, [ $path, "art/actors/" . $ent->{Entity}{VisualActor}{Actor}{' content'} ];
                push @deps, [ $path, "art/actors/" . $ent->{Entity}{VisualActor}{FoundationActor}{' content'} ] if $ent->{Entity}{VisualActor}{FoundationActor};
            }

            if ($ent->{Entity}{Sound})
            {
                push @deps, [ $path, "audio/" . $_->{' content'} ] for grep ref($_), values %{$ent->{Entity}{Sound}{SoundGroups}};
            }
        }
    }
}

sub add_actors
{
    print "Loading actors...\n";

    my @actorfiles = find_files('art/actors', 'xml');
    for my $f (sort @actorfiles)
    {
        push @files, $f;

        push @roots, $f if ROOT_ACTORS;

        my $actor = XMLin(vfs_to_physical($f), ForceArray => [qw(group variant prop animation)], KeyAttr => []) or die "Failed to parse '$f': $!";

        for my $group (@{$actor->{group}})
        {
            for my $variant (@{$group->{variant}})
            {
                push @deps, [ $f, "art/meshes/$variant->{mesh}" ] if $variant->{mesh};
                push @deps, [ $f, "art/textures/skins/$variant->{texture}" ] if $variant->{texture};
                for my $prop (@{$variant->{props}{prop}})
                {
                    push @deps, [ $f, "art/actors/$prop->{actor}" ] if $prop->{actor};
                }
                for my $anim (@{$variant->{animations}{animation}})
                {
                    push @deps, [ $f, "art/animation/$anim->{file}" ] if $anim->{file};
                }
            }
        }
    }
}

sub add_art
{
    print "Loading art files...\n";
    push @files, find_files('art/textures/skins', 'dds|png|jpg|tga');
    push @files, find_files('art/meshes', 'pmd|dae');
    push @files, find_files('art/animation', 'psa|dae');
}

sub add_scenarios
{
    print "Loading scenarios...\n";
    my @mapfiles = find_files('maps/scenarios', 'xml');
    for my $f (sort @mapfiles)
    {
        print "  $f\n";

        push @files, $f;

        push @roots, $f;

        my $map = XMLin(vfs_to_physical($f), ForceArray => [qw(Entity)], KeyAttr => []) or die "Failed to parse '$f': $!";

        my %used;
        for my $entity (@{$map->{Entities}{Entity}})
        {
            $used{$entity->{Template}} = 1;
        }

        for my $template (keys %used)
        {
            if ($template =~ /^actor\|(.*)$/)
            {
                push @deps, [ $f, "art/actors/$1" ];
            }
            else
            {
                push @deps, [ $f, "simulation/templates/$template.xml" ];
            }
        }
    }
}

sub add_soundgroups
{
    print "Loading sound groups...\n";
    my @soundfiles = find_files('audio', 'xml');
    for my $f (sort @soundfiles)
    {
        push @files, $f;

        my $sound = XMLin(vfs_to_physical($f), ForceArray => [qw(Sound)], KeyAttr => []) or die "Failed to parse '$f': $!";

        my $path = $sound->{Path};
        $path =~ s/\/$//; # strip optional trailing slash

        for (@{$sound->{Sound}})
        {
            push @deps, [$f, "$path/$_" ];
        }

        push @deps, [$f, "$path/$sound->{Replacement}" ] if $sound->{Replacement} and not ref $sound->{Replacement};
    }
}

sub add_audio
{
    print "Loading audio files...\n";
    push @files, find_files('audio', 'ogg');
}

sub check_deps
{
    my %files;
    @files{@files} = ();

    my %lcfiles;
    @lcfiles{map lc($_), @files} = @files;

    my %revdeps;
    for my $d (@deps)
    {
        push @{$revdeps{$d->[1]}}, $d->[0];
    }

    for my $f (sort keys %revdeps)
    {
        next if exists $files{$f};
        warn "Missing file '$f' referenced by: " . (join ', ', map "'$_'", map vfs_to_relative_to_mods($_), sort @{$revdeps{$f}}) . "\n";

        if (exists $lcfiles{lc $f})
        {
            warn "### Case-insensitive match (found '$lcfiles{lc $f}')\n";
        }
    }
}

sub check_unused
{
    my %reachable;
    @reachable{@roots} = ();

    my %deps;
    for my $d (@deps)
    {
        push @{$deps{$d->[0]}}, $d->[1];
    }

    while (1)
    {
        my @newreachable;
        for my $r (keys %reachable)
        {
            push @newreachable, grep { not exists $reachable{$_} } @{$deps{$r}};
        }
        last if @newreachable == 0;
        @reachable{@newreachable} = ();
    }

    for my $f (sort @files)
    {
        next if exists $reachable{$f};
        warn "Unused file '" . vfs_to_relative_to_mods($f) . "'\n";
    }
}

add_scenarios() if CHECK_SCENARIOS;

add_entities();

add_actors();

add_art();

add_soundgroups();

add_audio();

# TODO: add non-skin textures, and all the references to them
# TODO: add GUI files

print "\n";
check_deps();
print "\n";
check_unused();

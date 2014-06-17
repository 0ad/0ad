use strict;
use warnings;

use File::Find;
use XML::LibXML;

use constant VERBOSE_OUTPUT => 1;

my $vfsroot = '../../../binaries/data/mods';

sub vfs_to_physical
{
    my ($vfspath) = @_;
    my $fn = "$vfsroot/public/$vfspath";
    return $fn;
}

sub vfs_to_relative_to_mods
{
    my ($vfspath) = @_;
    my $fn = "public/$vfspath";
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
        $n =~ s~\Q$vfsroot\E/public/~~;
        push @files, $n;
    };
    find({ wanted => $find_process }, "$vfsroot/public/$vfspath");

    return @files;
}

sub validate
{
    my ($name, $arr, $schemapath) = @_;
    my @files = @{$arr};
    print "\nValidating ".$name."s...\n";

    my $rngschema = XML::LibXML::RelaxNG->new( location => vfs_to_physical($schemapath) );
    my $errorcount = 0;

    for my $f (sort @files)
    {
        my $doc = XML::LibXML->new->parse_file(vfs_to_physical($f));

        eval { $rngschema->validate( $doc ); };

        if ($@)
        {
            if (VERBOSE_OUTPUT)
            {
                # strip $vfsroot from messages
                $@ =~ s/$vfsroot//g;
                warn $@;
            }
            else
            {
                warn "$f validation failed\n";
            }

            $errorcount++;
        }
    }
    print "\n$errorcount $name validation errors\n";
}

sub validate_actors
{
    my @files = find_files('art/actors', 'xml');
    validate('actor', \@files, 'art/actors/actor.rng');
}

sub validate_guis
{
    # there are two different gui XML schemas depending on path
    my @files = find_files('gui', 'xml');
    my (@guipages, @guixmls);
    for my $f (@files)
    {
        if ($f =~ /^gui\/page_/)
        {
            push @guipages, $f;
        }
        else
        {
            push @guixmls, $f;
        }
    }

    validate('gui page', \@guipages, 'gui/gui_page.rng');
    validate('gui xml', \@guixmls, 'gui/gui.rng');
}

sub validate_maps
{
    my @files = find_files('maps/scenarios', 'xml');
    push @files, find_files('maps/skirmishes', 'xml');
    validate('map', \@files, 'maps/scenario.rng');
}

sub validate_materials
{
    my @files = find_files('art/materials', 'xml');
    validate('material', \@files, 'art/materials/material.rng');
}

sub validate_particles
{
    my @files = find_files('art/particles', 'xml');
    validate('particle', \@files, 'art/particles/particle.rng');
}

sub validate_simulation
{
    my @file = ('simulation/data/pathfinder.xml');
    validate('pathfinder', \@file, 'simulation/data/pathfinder.rng');
    @file = ('simulation/data/territorymanager.xml');
    validate('territory manager', \@file, 'simulation/data/territorymanager.rng');
}

sub validate_soundgroups
{
    my @files = find_files('audio', 'xml');
    validate('sound group', \@files, 'audio/sound_group.rng');
}

sub validate_terrains
{
    # there are two different terrain XML schemas depending on path
    my @files = find_files('art/terrains', 'xml');
    my (@terrains, @terraintextures);
    for my $f (@files)
    {
        if ($f =~ /terrains.xml/)
        {
            push @terrains, $f;
        }
        else
        {
            push @terraintextures, $f;
        }
    }

    validate('terrain', \@terrains, 'art/terrains/terrain.rng');
    validate('terrain texture', \@terraintextures, 'art/terrains/terrain_texture.rng');
}

sub validate_textures
{
    my @files;
    my @texturefiles = find_files('art/textures', 'xml');
    for my $f (@texturefiles)
    {
        push @files, $f if $f =~ /textures.xml/;
    }
    validate('texture', \@files, 'art/textures/texture.rng');
}

validate_actors();
validate_guis();
validate_maps();
validate_materials();
validate_particles();
validate_simulation();
validate_soundgroups();
validate_terrains();
validate_textures();

use strict;
use warnings;
use Data::Dumper;
use File::Find;
use XML::Simple;
use JSON;
use Getopt::Long qw(GetOptions);

use lib ".";
use Entity;

GetOptions (
    '--check-unused' => \(my $checkUnused = 0),
    '--check-map-xml' => \(my $checkMapXml = 0),
    '--validate-templates' => \(my $validateTemplates = 0),
    '--mod-to-check=s' => \(my $modToCheck = "public")
);

my @files;
my @roots;
my @deps;

# Force and checkMapXml if checkUnused is enabled to avoid false positives.
$checkMapXml |= $checkUnused;
my $vfsroot = '../../../binaries/data/mods';
my $supportedTextureFormats = 'dds|png';
my $mods = get_mod_dependencies_string($modToCheck);
my $mod_list_string = $modToCheck;
if ($mods ne "")
{
   $mod_list_string =  $mod_list_string."|$mods";
}
$mod_list_string =  $mod_list_string."|mod";
print("Checking $modToCheck\'s integrity. \n");
print("The following mod(s) will be loaded: $mod_list_string. \n");
my @mods_list = split(/\|/, "$mod_list_string");

sub get_mod_dependencies
{
    my ($mod) = @_;
    my $modjson = parse_json_file_full_path("$vfsroot/$mod/mod.json");
    my $modjsondeps = $modjson->{'dependencies'};
    for my $dep (@{$modjsondeps})
    {
        # 0ad's folder isn't named like the mod.
        if(index($dep, "0ad") != -1)
        {
            $dep = "public";
        }
    }

    return $modjsondeps;
}

sub get_mod_dependencies_string
{
    my ($mod) = @_;
    return join( '|',@{get_mod_dependencies($mod)});
}

sub vfs_to_physical
{
    my ($vfsPath) = @_;
    my $fn = vfs_to_relative_to_mods($vfsPath);
    return "$vfsroot/$fn";
}

sub vfs_to_relative_to_mods
{
    my ($vfsPath) = @_;

    for my $dep (@mods_list)
    {
        my $fn = "$dep/$vfsPath";

        if (-e "$vfsroot/$fn")
        {
            return $fn;
        }
    }
}

sub find_files
{
    my ($vfsPath, $extn) = @_;
    my @files;
    my $find_process = sub {
        return $File::Find::prune = 1 if $_ eq '.svn';
        my $n = $File::Find::name;
        return if /~$/;
        return unless -f $_;
        return unless /\.($extn)$/;
        $n =~ s~\Q$vfsroot\E/($mod_list_string)/~~;
        push @files, $n;
    };

    for my $dep (@mods_list)
    {
        find({ wanted => $find_process },"$vfsroot/$dep/$vfsPath") if -d "$vfsroot/$dep/$vfsPath";
    }

    return @files;
}

sub parse_json_file_full_path
{
    my ($vfspath) = @_;
    open my $fh, $vfspath or die "Failed to open '$vfspath': $!";
    # decode_json expects a UTF-8 string and doesn't handle BOMs, so we strip those
    # (see http://trac.wildfiregames.com/ticket/1556)
    return decode_json(do { local $/; my $file = <$fh>; $file =~ s/^\xEF\xBB\xBF//; $file });
}

sub parse_json_file
{
    my ($vfspath) = @_;
    return parse_json_file_full_path(vfs_to_physical($vfspath))
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
        my $ent = Entity::load_inherited($f, "$mod_list_string");

        push @deps, [ $path, "simulation/templates/" . $ent->{Entity}{'@parent'}{' content'} . ".xml" ] if $ent->{Entity}{'@parent'};

        if ($f !~ /^template_/)
        {
            push @roots, $path;
            if ($ent->{Entity}{VisualActor} and $ent->{Entity}{VisualActor}{Actor})
            {
                my $phenotypes = $ent->{Entity}{Identity}{Phenotype}{' content'} || "default";
                my @phenotypes = split /\s/,$phenotypes;

                for my $phenotype (@phenotypes)
                {
                    # See simulation2/components/CCmpVisualActor.cpp and Identity.js for explanation.
                    my $actorPath = $ent->{Entity}{VisualActor}{Actor}{' content'};
                    $actorPath =~ s/{phenotype}/$phenotype/g;
                    push @deps, [ $path, "art/actors/" . $actorPath ];
                }

                push @deps, [ $path, "art/actors/" . $ent->{Entity}{VisualActor}{FoundationActor}{' content'} ] if $ent->{Entity}{VisualActor}{FoundationActor};
            }

            if ($ent->{Entity}{Sound})
            {
                my $phenotypes = $ent->{Entity}{Identity}{Phenotype}{' content'} || "default";
                my $lang = $ent->{Entity}{Identity}{Lang}{' content'} || "greek";

                my @phenotypes = split /\s/,$phenotypes;

                for my $phenotype (@phenotypes)
                {
                    for (grep ref($_), values %{$ent->{Entity}{Sound}{SoundGroups}})
                    {
                        # see simulation/components/Sound.js and Identity.js for explanation
                        my $soundPath = $_->{' content'};
                        $soundPath =~ s/{phenotype}/$phenotype/g;
                        $soundPath =~ s/{lang}/$lang/g;
                        push @deps, [ $path, "audio/" . $soundPath ];
                    }
                }
            }

            if ($ent->{Entity}{Identity})
            {
                push @deps, [ $path, "art/textures/ui/session/portraits/" . $ent->{Entity}{Identity}{Icon}{' content'} ] if $ent->{Entity}{Identity}{Icon} and $ent->{Entity}{Identity}{Icon}{' content'} ne '';
            }

            if ($ent->{Entity}{Heal} and $ent->{Entity}{Heal}{RangeOverlay})
            {
                push @deps, [ $path, "art/textures/selection/" . $ent->{Entity}{Heal}{RangeOverlay}{LineTexture}{' content'} ] if $ent->{Entity}{Heal}{RangeOverlay}{LineTexture} and $ent->{Entity}{Heal}{RangeOverlay}{LineTexture}{' content'} ne '';
                push @deps, [ $path, "art/textures/selection/" . $ent->{Entity}{Heal}{RangeOverlay}{LineTextureMask}{' content'} ] if $ent->{Entity}{Heal}{RangeOverlay}{LineTextureMask} and $ent->{Entity}{Heal}{RangeOverlay}{LineTextureMask}{' content'} ne '';
            }

            if ($ent->{Entity}{Selectable} and $ent->{Entity}{Selectable}{Overlay} and $ent->{Entity}{Selectable}{Overlay}{Texture})
            {
                push @deps, [ $path, "art/textures/selection/" . $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTexture}{' content'} ] if $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTexture} and $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTexture}{' content'} ne '';
                push @deps, [ $path, "art/textures/selection/" . $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTextureMask}{' content'} ] if $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTextureMask} and $ent->{Entity}{Selectable}{Overlay}{Texture}{MainTextureMask}{' content'} ne '';
            }

            if ($ent->{Entity}{Formation})
            {
                push @deps, [ $path, "art/textures/ui/session/icons/" . $ent->{Entity}{Formation}{Icon}{' content'} ] if $ent->{Entity}{Formation}{Icon} and $ent->{Entity}{Formation}{Icon}{' content'} ne '';
            }
        }
    }
}

sub push_variant_dependencies
{
    my ($variant, $f) = @_;
    push @deps, [ $f, "art/variants/$variant->{file}" ] if $variant->{file};
    push @deps, [ $f, "art/meshes/$variant->{mesh}" ] if $variant->{mesh};
    push @deps, [ $f, "art/particles/$variant->{particles}{file}" ] if $variant->{particles}{file};
    for my $tex (@{$variant->{textures}{texture}})
    {
        push @deps, [ $f, "art/textures/skins/$tex->{file}" ] if $tex->{file};
    }
    for my $prop (@{$variant->{props}{prop}})
    {
        push @deps, [ $f, "art/actors/$prop->{actor}" ] if $prop->{actor};
    }
    for my $anim (@{$variant->{animations}{animation}})
    {
        push @deps, [ $f, "art/animation/$anim->{file}" ] if $anim->{file};
    }
}

sub add_actors
{
    print "Loading actors...\n";

    my @actorfiles = find_files('art/actors', 'xml');
    for my $f (sort @actorfiles)
    {
        push @files, $f;
        push @roots, $f;

        my $actor = XMLin(vfs_to_physical($f), ForceArray => [qw(group variant texture prop animation)], KeyAttr => []) or die "Failed to parse '$f': $!";

        for my $group (@{$actor->{group}})
        {
            for my $variant (@{$group->{variant}})
            {
                push_variant_dependencies($variant, $f);
            }
        }

        push @deps, [ $f, "art/materials/$actor->{material}" ] if $actor->{material};
    }
}


sub add_variants
{
    print "Loading variants...\n";
    my @variantfiles = find_files('art/variants', 'xml');

    for my $f (sort @variantfiles)
    {
        push @files, $f;
        push @roots, $f;
        my $variant = XMLin(vfs_to_physical($f), ForceArray => [qw(texture prop animation)], KeyAttr => []) or die "Failed to parse '$f': $!";
        push_variant_dependencies($variant, $f);
    }
}

sub add_art
{
    print "Loading art files...\n";
    push @files, find_files('art/textures/particles', $supportedTextureFormats);
    push @files, find_files('art/textures/terrain', $supportedTextureFormats);
    push @files, find_files('art/textures/skins', $supportedTextureFormats);
    push @files, find_files('art/meshes', 'pmd|dae');
    push @files, find_files('art/animation', 'psa|dae');
}

sub add_materials
{
    print "Loading materials...\n";
    my @materialfiles = find_files('art/materials', 'xml');
    for my $f (sort @materialfiles)
    {
        push @files, $f;

        my $material = XMLin(vfs_to_physical($f), ForceArray => [qw(alternative)], KeyAttr => []);
        for my $alternative (@{$material->{alternative}})
        {
            push @deps, [ $f, "art/materials/$alternative->{material}" ] if $alternative->{material};
        }
    }
}

sub add_particles
{
    print "Loading particles...\n";
    my @particlefiles = find_files('art/particles', 'xml');
    for my $f (sort @particlefiles)
    {
        push @files, $f;

        my $particle = XMLin(vfs_to_physical($f));
        push @deps, [ $f, "$particle->{texture}" ] if $particle->{texture};
    }
}

sub add_maps_xml
{
    print "Loading maps XML...\n";
    my @mapfiles = find_files('maps/scenarios', 'xml');
    push @mapfiles, find_files('maps/skirmishes', 'xml');
    push @mapfiles, find_files('maps/tutorials', 'xml');
    for my $f (sort @mapfiles)
    {
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
                # Handle special 'actor|' case
                push @deps, [ $f, "art/actors/$1" ];
            }
            else
            {
                if ($template =~ /^resource\|(.*)$/)
                {
                    # Handle special 'resource|' case
                    $template = $1;
                }
                push @deps, [ $f, "simulation/templates/$template.xml" ];
            }
        }

        # Map previews
        my $settings = decode_json($map->{ScriptSettings});
        push @deps, [ $f, "art/textures/ui/session/icons/mappreview/" . $settings->{Preview} ] if $settings->{Preview};
    }
}

sub add_maps_pmp
{
    print "Loading maps PMP...\n";

    # Need to generate terrain texture filename=>path lookup first
    my %terrains;
    for my $f (find_files('art/terrains', 'xml'))
    {
        $f =~ /([^\/]+)\.xml/ or die;

        # ignore terrains.xml
        if ($f !~ /terrains.xml$/)
        {
            warn "Duplicate terrain name '$1' (from '$terrains{$1}' and '$f')\n" if $terrains{$1};
            $terrains{$1} = $f;
        }
    }

    my @mapfiles = find_files('maps/scenarios', 'pmp');
    push @mapfiles, find_files('maps/skirmishes', 'pmp');
    for my $f (sort @mapfiles)
    {
        push @files, $f;

        push @roots, $f;

        open my $fh, vfs_to_physical($f) or die "Failed to open '$f': $!";
        binmode $fh;

        my $buf;

        read $fh, $buf, 4;
        die "Invalid PMP header ($buf) in '$f'" unless $buf eq "PSMP";

        read $fh, $buf, 4;
        my $version = unpack 'V', $buf;
        die "Invalid PMP version ($version) in '$f'" unless $version == 6;

        read $fh, $buf, 4;
        my $datasize = unpack 'V', $buf;

        read $fh, $buf, 4;
        my $mapsize = unpack 'V', $buf;

        seek $fh, 2 * ($mapsize*16+1)*($mapsize*16+1), 1; # heightmap

        read $fh, $buf, 4;
        my $numtexs = unpack 'V', $buf;

        for (0..$numtexs-1)
        {
            read $fh, $buf, 4;
            my $len = unpack 'V', $buf;
            my $str;
            read $fh, $str, $len;

            push @deps, [ $f, $terrains{$str} || "art/terrains/(unknown)/$str" ];
        }

        # ignore patches data
    }
}

sub add_soundgroups
{
    print "Loading sound groups...\n";
    my @soundfiles = find_files('audio', 'xml');
    for my $f (sort @soundfiles)
    {
        push @files, $f;
        push @roots, $f;

        my $sound = XMLin(vfs_to_physical($f), ForceArray => [qw(Sound)], KeyAttr => []) or die "Failed to parse '$f': $!";

        my $path = $sound->{Path};
        $path =~ s/\/$//; # strip optional trailing slash

        for (@{$sound->{Sound}})
        {
            push @deps, [$f, "$path/$_" ];
        }
    }
}

sub add_audio
{
    print "Loading audio files...\n";
    push @files, find_files('audio', 'ogg');
}

sub add_gui_xml
{
    print "Loading GUI XML...\n";
    my @guifiles = find_files('gui', 'xml');
    for my $f (sort @guifiles)
    {
        push @files, $f;

        if ($f =~ /^gui\/page_/)
        {
            push @roots, $f;
            my $xml = XMLin(vfs_to_physical($f), ForceArray => [qw(include)], KeyAttr => []) or die "Failed to parse '$f': $!";

            for my $include (@{$xml->{include}})
            {
                # If including an entire directory, find all the *.xml files
                if ($include =~ /\/$/)
                {
                    push @deps, [ $f, $_ ] for find_files("gui/$include", 'xml');
                }
                else
                {
                    push @deps, [ $f, "gui/$include" ];
                }
            }
        }
        else
        {
            my $xml = XMLin(vfs_to_physical($f), ForceArray => [qw(object script action sprite image)], KeyAttr => [], KeepRoot => 1) or die "Failed to parse '$f': $!";
            my $name = (keys %$xml)[0];
            if ($name eq 'objects' or $name eq 'object')
            {
                for (grep ref $_ , @{$xml->{objects}{script}})
                {
                    push @deps, [ $f, $_->{file} ] if $_->{file};
                    if ($_->{directory})
                    {
                        # If including an entire directory, find all the *.js files
                        push @deps, [ $f, $_ ] for find_files($_->{directory}, 'js')
                    }
                }
                my $add_objects;
                $add_objects = sub
                {
                    my ($parent) = @_;
                    for my $obj (@{$parent->{object}})
                    {
                        # TODO: look at sprites, styles, etc
                        $add_objects->($obj);
                    }
                };
                $add_objects->($xml->{objects});
            }
            elsif ($name eq 'setup')
            {
                # TODO: look at sprites, styles, etc
            }
            elsif ($name eq 'styles')
            {
                # TODO: look at sprites, styles, etc
            }
            elsif ($name eq 'sprites')
            {
                for my $sprite (@{$xml->{sprites}{sprite}})
                {
                    for my $image (@{$sprite->{image}})
                    {
                        push @deps, [ $f, "art/textures/ui/$image->{texture}" ] if $image->{texture};
                    }
                }
            }
            else
            {
                print "Unexpected GUI XML root element '$name':\n" . Dumper $xml;
                exit;
            }
        }
    }
}

sub add_gui_data
{
    print "Loading GUI data...\n";
    push @files, find_files('gui', 'js');
    push @files, find_files('art/textures/ui', $supportedTextureFormats);
    push @files, find_files('art/textures/selection', $supportedTextureFormats);
}

sub add_civs
{
    print "Loading civs...\n";

    my @civfiles = find_files('simulation/data/civs', 'json');
    for my $f (sort @civfiles)
    {
        push @files, $f;

        push @roots, $f;

        my $civ = parse_json_file($f);

        push @deps, [ $f, "art/textures/ui/" . $civ->{Emblem} ] if $civ->{Emblem};

        push @deps, [ $f, "audio/music/" . $_->{File} ] for @{$civ->{Music}};
    }
}

sub add_rms
{
    print "Loading random maps...\n";

    push @files, find_files('maps/random', 'js');
    my @rmsdefs = find_files('maps/random', 'json');

    for my $f (sort @rmsdefs)
    {
        next if $f =~ /^maps\/random\/rmbiome/;

        push @files, $f;

        push @roots, $f;

        my $rms = parse_json_file($f);
        push @deps, [ $f, "maps/random/" . $rms->{settings}{Script} ]   if $rms->{settings}{Script};

        # Map previews
        push @deps, [ $f, "art/textures/ui/session/icons/mappreview/" . $rms->{settings}{Preview} ] if $rms->{settings}{Preview};
    }
}

sub add_techs
{
    print "Loading techs...\n";

    my @techfiles = find_files('simulation/data/technologies', 'json');
    for my $f (sort @techfiles)
    {
        push @files, $f;
        push @roots, $f;

        my $tech = parse_json_file($f);

        push @deps, [ $f, "art/textures/ui/session/portraits/technologies/" . $tech->{icon} ] if $tech->{icon};
        push @deps, [ $f, "simulation/data/technologies/" . $tech->{supersedes} . ".json" ] if $tech->{supersedes};
    }
}

sub add_auras
{
    print "Loading auras...\n";

    my @aurafiles = find_files('simulation/data/auras', 'json');
    for my $f (sort @aurafiles)
    {
        push @files, $f;
        push @roots, $f;

        my $aura = parse_json_file($f);

        push @deps, [ $f, $aura->{overlayIcon} ] if $aura->{overlayIcon};

        if($aura->{rangeOverlay})
        {
            push @deps, [ $f, "art/textures/selection/" . $aura->{rangeOverlay}{lineTexture} ] if $aura->{rangeOverlay}{lineTexture};
            push @deps, [ $f, "art/textures/selection/" . $aura->{rangeOverlay}{lineTextureMask} ] if $aura->{rangeOverlay}{lineTextureMask};
        }
    }
}

sub add_terrains
{
    print "Loading terrains...\n";

    my @terrains = find_files('art/terrains', 'xml');
    for my $f (sort @terrains)
    {
        # ignore terrains.xml
        if ($f !~ /terrains.xml$/)
        {
            push @files, $f;
            push @roots, $f;

            my $terrain = XMLin(vfs_to_physical($f), ForceArray => [qw(texture)], KeyAttr => []) or die "Failed to parse '$f': $!";

            for my $texture (@{$terrain->{textures}{texture}})
            {
                push @deps, [ $f, "art/textures/terrain/$texture->{file}" ] if $texture->{file};
            }
            push @deps, [ $f, "art/materials/$terrain->{material}" ] if $terrain->{material};
        }
    }
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
        next if exists $reachable{$f}
        || index($f, "art/terrains/") != -1
        || index($f, "maps/random/") != -1
        || index($f, "art/materials/") != -1;
        warn "Unused file '" . vfs_to_relative_to_mods($f) . "'\n";
    }
}


add_maps_xml() if $checkMapXml;
add_maps_pmp();
add_entities();
add_actors();
add_variants();
add_art();
add_materials();
add_particles();
add_soundgroups();
add_audio();
add_gui_xml();
add_gui_data();
add_civs();
add_rms();
add_techs();
add_terrains();
add_auras();

check_deps();
check_unused() if $checkUnused;
print "\n" if $checkUnused;
system("perl ../xmlvalidator/validate.pl") if $validateTemplates;

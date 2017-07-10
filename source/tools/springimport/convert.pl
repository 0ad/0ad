# Copyright (C) 2010 Wildfire Games.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# This is a fairly hacky converter from the .3do format used by Spring
# to the Collada + actor XML format used by 0 A.D.

use strict;
use warnings;
use Data::Dumper;

use TextureAtlas;
use SpringPalette;

use constant SCALE => 3.0 / 1048576;

my $ROOT = "../../../binaries/data/mods/public";
my $SOURCE = "../../../../../misc/spring/BA712";

sub parse_3do
{
    my ($fh) = @_;

    my $b;

    read $fh, $b, 13*4 or die;
    my ($sig, $num_verts, $num_prims, $sel_prim, $x, $y, $z,
        $off_name, undef, $off_verts, $off_prims, $off_sib, $off_child) =
        unpack 'l*', $b;

    die unless $sig == 1;

    seek $fh, $off_name, 0;
    my $name = '';
    while (1) {
        read $fh, my $c, 1 or die;
        last if $c eq "\0";
        $name .= $c;
    }

    seek $fh, $off_verts, 0;
    read $fh, $b, 12*$num_verts or die;
    my @verts = unpack 'l*', $b;

    my @prims;
    for my $p (0..$num_prims-1) {
        seek $fh, $off_prims + 32*$p, 0;
        read $fh, $b, 32 or die;
        my ($palette, $num_idxs, undef, $off_idxs, $off_tex) = unpack 'l*', $b;

        seek $fh, $off_idxs, 0;
        read $fh, $b, 2*$num_idxs or die;
        my @idxs = unpack 's*', $b;

        my $texture;
        if ($off_tex) {
            $texture = '';
            seek $fh, $off_tex, 0;
            while (1) {
                read $fh, my $c, 1 or die;
                last if $c eq "\0";
                $texture .= $c;
            }
        }

        push @prims, {
            palette => $palette,
            idxs => \@idxs,
            texture => $texture,
        };
    }

    my $ret = {
        name => $name,
        verts => \@verts,
        prims => \@prims,
        offset => [$x, $y, $z],
    };

    if ($off_child) {
        seek $fh, $off_child, 0;
        $ret->{children} = [ parse_3do($fh) ];
    }

    if ($off_sib) {
        seek $fh, $off_sib, 0;
        return ($ret, parse_3do($fh));
    } else {
        return ($ret);
    }
}

sub write_geometries
{
    my ($obj, $atlas, $name, $fh) = @_;

    my $num_verts = @{ $obj->{verts} } / 3;
    my $num_prims = @{ $obj->{prims} };

    my @vertex_normals;
    for (0..$num_verts-1) {
        push @vertex_normals, [0,0,0];
    }

    my @prim_normals;
    for my $prim (@{ $obj->{prims} }) {
        my @v0 = ($obj->{verts}[$prim->{idxs}[0]*3], $obj->{verts}[$prim->{idxs}[0]*3+1], $obj->{verts}[$prim->{idxs}[0]*3+2]);
        my @v1 = ($obj->{verts}[$prim->{idxs}[1]*3], $obj->{verts}[$prim->{idxs}[1]*3+1], $obj->{verts}[$prim->{idxs}[1]*3+2]);
        my @v2 = ($obj->{verts}[$prim->{idxs}[2]*3], $obj->{verts}[$prim->{idxs}[2]*3+1], $obj->{verts}[$prim->{idxs}[2]*3+2]);
        my @d10 = ($v1[0] - $v0[0], $v1[1] - $v0[1], $v1[2] - $v0[2]);
        my @d20 = ($v2[0] - $v0[0], $v2[1] - $v0[1], $v2[2] - $v0[2]);
        my @x = ($d10[1]*$d20[2] - $d10[2]*$d20[1], $d10[2]*$d20[0] - $d10[0]*$d20[2], $d10[0]*$d20[1] - $d10[1]*$d20[0]);
        my $d = sqrt($x[0]*$x[0] + $x[1]*$x[1] + $x[2]*$x[2]);
        my @n = $d ? ($x[0]/$d, $x[1]/$d, $x[2]/$d) : (0,1,0);
        push @prim_normals, \@n;
        for my $v (@{ $prim->{idxs} }) {
            $vertex_normals[$v][0] += $n[0];
            $vertex_normals[$v][1] += $n[1];
            $vertex_normals[$v][2] += $n[2];
        }
    }

    for (0..$num_verts-1) {
        my @n = @{ $vertex_normals[$_] };
        my $d = sqrt($n[0]*$n[0] + $n[1]*$n[1] + $n[2]*$n[2]);
        $vertex_normals[$_] = $d ? [$n[0]/$d, $n[1]/$d, $n[2]/$d] : [0,0,0];
    }

    print $fh <<EOF;
  <library_geometries>
    <geometry id="$name">
      <mesh>
        <source id="$name-Position">
          <float_array id="$name-Position-array" count="@{[ $num_verts*3 ]}">
EOF

    for my $n (0..$num_verts-1) {
        printf $fh "%.6f %.6f %.6f\n", $obj->{verts}[$n*3]*SCALE, $obj->{verts}[$n*3+1]*SCALE, $obj->{verts}[$n*3+2]*SCALE;
    }

    print $fh <<EOF;
</float_array>
          <technique_common>
            <accessor source="#$name-Position-array" count="$num_verts" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="$name-Normal0">
          <float_array id="$name-Normal0-array" count="@{[ $num_verts*3 ]}">
EOF

    for my $n (0..$num_verts-1) {
        printf $fh "%.6f %.6f %.6f\n", @{$vertex_normals[$n]};
    }

    my @uvs;
    for my $prim (@{ $obj->{prims} }) {
        my ($u0,$v0, $u1,$v1) = $atlas->get_texcoords(texture_filename($prim));
        if (@{ $prim->{idxs} } == 3) {
            push @uvs, $u0,$v0, $u0,$v1, $u1,$v1;
        } elsif (@{ $prim->{idxs} } == 4) {
            push @uvs, $u0,$v0, $u0,$v1, $u1,$v1, $u1,$v0;
        } else {
            push @uvs, ($u1,$v0) x @{ $prim->{idxs} };
        }
    }
    my $num_uvs = @uvs/2;

    print $fh <<EOF;
</float_array>
          <technique_common>
            <accessor source="#$name-Normal0-array" count="$num_verts" stride="3">
              <param name="X" type="float"/>
              <param name="Y" type="float"/>
              <param name="Z" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <source id="$name-UV0">
          <float_array id="$name-UV0-array" count="@{[ $num_uvs*2 ]}">
@uvs
</float_array>
          <technique_common>
            <accessor source="#$name-UV0-array" count="$num_uvs" stride="2">
              <param name="S" type="float"/>
              <param name="T" type="float"/>
            </accessor>
          </technique_common>
        </source>
        <vertices id="$name-Vertex">
          <input semantic="POSITION" source="#$name-Position"/>
        </vertices>
        <polygons count="$num_prims">
          <input semantic="VERTEX" offset="0" source="#$name-Vertex"/>
          <input semantic="NORMAL" offset="1" source="#$name-Normal0"/>
          <input semantic="TEXCOORD" offset="2" set="0" source="#$name-UV0"/>
EOF

    my $i = 0;
    my $j = 0;
    for my $prim (@{ $obj->{prims} }) {
        print $fh "<p>";
        for (@{ $prim->{idxs} }) {
            print $fh "$_ $_ $j ";
            ++$j;
        }
        ++$i;
        print $fh "</p>\n";
    }

    print $fh <<EOF;
        </polygons>
      </mesh>
    </geometry>
  </library_geometries>
EOF
}

sub write_mesh
{
    my ($obj, $atlas, $name, $fh) = @_;

    print $fh <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.0">
  <asset>
    <unit meter="0.025400"/>
    <up_axis>Y_UP</up_axis>
  </asset>
EOF

    write_geometries($obj, $atlas, $name, $fh);

    print $fh <<EOF;
  <library_visual_scenes>
    <visual_scene id="RootNode" name="RootNode">
      <node name="$name\_node">
        <instance_geometry url="#$name"/>
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene>
    <instance_visual_scene url="#RootNode"/>
  </scene>
</COLLADA>
EOF
}

sub write_joint
{
    my ($joint, $fh, $indent) = @_;

    print $fh qq[$indent<node id="prop-$joint->{name}" name="prop-$joint->{name}" type="JOINT">\n];
    print $fh qq[$indent  <translate>] . (sprintf "%.6f %.6f %.6f", map $_*SCALE, $joint->{offset}[0], $joint->{offset}[1], -$joint->{offset}[2]) . qq[</translate>\n];
    for my $c (@{ $joint->{children} }) {
        write_joint($c, $fh, "$indent  ");
    }
    print $fh qq[$indent</node>\n];
}

sub write_skeleton
{
    my ($skeleton, $rootobj, $atlas, $name, $fh) = @_;

    print $fh <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.0">
  <asset>
    <unit meter="0.025400"/>
    <up_axis>Y_UP</up_axis>
  </asset>
EOF

    write_geometries($rootobj, $atlas, $name, $fh);

    print $fh <<EOF;
  <library_visual_scenes>
    <visual_scene id="RootNode">
      <node type="NODE" name="$name\_node">
        <rotate>0 1.0 0 180</rotate>
        <instance_geometry url="#$name"/>
EOF

    write_joint($_, $fh, "        ") for @$skeleton;

    print $fh <<EOF;
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene>
    <instance_visual_scene url="#RootNode"/>
  </scene>
</COLLADA>
EOF
}

sub extract_joint_names
{
    my ($joint) = @_;
    return ($joint->{name}, map { extract_joint_names($_) } @{ $joint->{children} });
}

sub texture_filename
{
    my ($prim) = @_;
    if ($prim->{texture}) {
        my $base = "$prim->{texture}";
        my @files = ("${base}.tga", "${base}00.tga", "${base}00.bmp", "${base}00.BMP");
        push @files, map { lc $_ } @files;
        @files = grep { -e "$SOURCE/unittextures/tatex/$_" } @files;
        die "can't find $prim->{texture}" unless @files;
        return "$SOURCE/unittextures/tatex/$files[0]";
    } else {
        return SpringPalette::get_image($prim->{palette});
    }
}

sub write_skeleton_actor
{
    my ($skeleton, $name, $fh) = @_;
    print $fh <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<actor version="1">

  <castshadow/>

  <group>
    <variant frequency="100" name="Base">
      <mesh>spring/$name.dae</mesh>
      <texture>spring/$name.tga</texture>
      <props>
        <prop actor="props/units/weapons/spear_gold.xml" attachpoint="projectile"/>
EOF

    my @joints = map { extract_joint_names($_) } @$skeleton;
    for my $n (@joints[1..$#joints]) {
        print $fh qq[        <prop actor="spring/$name-$n.xml" attachpoint="$n"/>\n];
    }

    print $fh <<EOF;
      </props>
    </variant>
  </group>

  <material>player_trans.xml</material>

</actor>
EOF
}

sub write_mesh_actor
{
    my ($mesh, $prefix, $name, $fh) = @_;
    #warn Dumper $mesh;
    print $fh <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<actor version="1">

  <castshadow/>

  <group>
    <variant frequency="100" name="Base">
      <mesh>spring/$name.dae</mesh>
      <texture>spring/$prefix.tga</texture>
    </variant>
  </group>

  <material>player_trans.xml</material>

</actor>
EOF
}

sub write_entity
{
    my ($prefix, $fh) = @_;
    print $fh <<EOF;
<?xml version="1.0" encoding="utf-8"?>
<Entity parent="template_unit_infantry_ranged">
  <Identity>
    <Civ>hele</Civ>
    <SpecificName>$prefix</SpecificName>
  </Identity>
  <Promotion>
    <Entity>???</Entity>
  </Promotion>
  <UnitMotion>
    <WalkSpeed>9.0</WalkSpeed>
  </UnitMotion>
  <VisualActor>
    <Actor>spring/$prefix.xml</Actor>
  </VisualActor>
EOF

    if ($prefix =~ /^(armaas|armbats|corcrus)$/) {
        print $fh <<EOF;
  <Position>
    <Floating>true</Floating>
  </Position>
EOF
    }

    print $fh <<EOF;
</Entity>
EOF
}

sub extract_textures
{
    my ($obj, $atlas) = @_;

    my @meshes;
    my @prims = grep { @{ $_->{idxs} } >= 3 } @{ $obj->{prims} };

    for (@prims) {
        $atlas->add(texture_filename($_));
    }

    extract_textures($_, $atlas) for @{ $obj->{children} };
}

sub extract_meshes
{
    my ($obj) = @_;

    my @prims = grep { @{ $_->{idxs} } >= 3 } @{ $obj->{prims} };

    my @meshes;
    push @meshes, { name => $obj->{name}, verts => $obj->{verts}, prims => \@prims };
    push @meshes, map { extract_meshes($_) } @{ $obj->{children} };
    return @meshes;
}

sub extract_skeleton
{
    my ($obj) = @_;
    return { name => $obj->{name}, offset => $obj->{offset}, children => [ map { extract_skeleton($_) } @{ $obj->{children} } ] };
}

sub write_model
{
    my ($prefix, $model) = @_;

    my $atlas = new TextureAtlas(256);
    extract_textures($model, $atlas);
    $atlas->finish("$ROOT/art/textures/skins/spring/$prefix.tga");

    my @meshes = extract_meshes($model);
    #print Dumper \@meshes;

    #exit;

    my @skeleton = extract_skeleton($model);
    #print Dumper \@skeleton;

    {
        open my $fhm, '>', "$ROOT/art/meshes/spring/$prefix.dae" or die $!;
        write_skeleton(\@skeleton, $meshes[0], $atlas, $prefix, $fhm);

        open my $fha, '>', "$ROOT/art/actors/spring/$prefix.xml" or die $!;
        write_skeleton_actor(\@skeleton, $prefix, $fha);
    }

    for my $mesh (@meshes[1..$#meshes]) {
        my $name = $prefix.'-'.$mesh->{name};
        open my $fhm, '>', "$ROOT/art/meshes/spring/$name.dae" or die $!;
        write_mesh($mesh, $atlas, $name, $fhm);

        open my $fha, '>', "$ROOT/art/actors/spring/$name.xml" or die $!;
        write_mesh_actor($mesh, $prefix, $name, $fha);
    }

    {
        open my $fhe, '>', "$ROOT/simulation/templates/spring/$prefix.xml" or die $!;
        write_entity($prefix, $fhe);
    }
}

#for my $n (qw(armpw armcom KrogTaar armlab corak corlab)) {
for my $n (map { (/.*\/(.*)\.3do/)[0] } grep /\/(cor|arm)[a-z]+\./, <$SOURCE/Objects3d/*.3do>) {
    print "$n\n";
    open my $t, "$SOURCE/Objects3d/$n.3do" or die "$n: $!";
    binmode $t;
    my $model = parse_3do($t);
    #print Dumper $model;
    write_model($n, $model);
}

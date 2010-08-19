package TextureAtlas;

# Incredibly rubbish texture packer

use strict;
use warnings;

use Image::Magick;

sub new
{
    my ($class, $width) = @_;
    my $self = {
        width => $width,
        x => 0,
        y => 0,
        rowheight => 0,
        images => {},
    };
    bless $self, $class;
}

sub add
{
    my ($self, $filename) = @_;
    return if $self->{images}{$filename};

    my $img = new Image::Magick;
    $img->ReadImage($filename);
    my $w = $img->Get("width");
    my $h = $img->Get("height");

    if ($filename =~ /\.tga$/i) {
        for my $y (0..$h-1) {
            for my $x (0..$w-1) {
                my @p = $img->GetPixel(x => $x, y => $y, channel => "RGBA");
                if ($p[0] == $p[2] and $p[1] == 0) {
                    my $a = $p[0] * 1.5;
                    $a = 0.95 if $a > 0.95; # prevent premul ugliness
                    my $c = $a;
                    $img->SetPixel(x => $x, y => $y, color => [$c,$c,$c,$a], channel => "RGBA");
                } else {
                    $img->SetPixel(x => $x, y => $y, color => [$p[0],$p[1],$p[2],0], channel => "RGBA");
                }
            }
        }
    }

    die if $w > $self->{width};
    if ($self->{x} + $w > $self->{width}) {
        $self->{x} = 0;
        $self->{y} += $self->{rowheight};
        $self->{rowheight} = 0;
    }

    $self->{images}{$filename} = { img => $img, w => $w, h => $h, x => $self->{x}, y => $self->{y} };

    $self->{rowheight} = $h if $h > $self->{rowheight};
    $self->{x} += $w;
}

sub finish
{
    my ($self, $filename) = @_;

    my $h = $self->{y} + $self->{rowheight};
    my $hlog = log($h)/log(2);
    $hlog = int($hlog+1) if $hlog != int($hlog);
    $h = 2**$hlog;
    $self->{height} = $h;

    my $image = new Image::Magick;
    $image->Set(size => $self->{width}."x".$h, depth => 8);
    $image->ReadImage("xc:transparent");
    for my $t (values %{ $self->{images} }) {
        $image->Composite(image => $t->{img}, x => $t->{x}, y => $t->{y});
    }
    $image->Write($filename);
}

sub get_texcoords
{
    my ($self, $filename) = @_;
    my $t = $self->{images}{$filename} or die;
    return (
        $t->{x} / $self->{width},
        1 - ($t->{y} / $self->{height}),
        ($t->{x} + $t->{w}) / $self->{width},
        1 - (($t->{y} + $t->{h}) / $self->{height}),
    );
}

1;

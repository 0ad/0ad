use strict;
use warnings;

package DDS;

sub new
{
	my $proto = shift;
	my $class = ref($proto) || $proto;
	
	my $fh;
	if (ref $_[0])
	{
		$fh = $_[0];
	}
	else
	{
		open $fh, '<', $_[0] or die "Cannot open $_[0]";
		binmode $fh;
	}
	
	my %dds;
	$dds{dwMagic} = pack L => read_DWORD($fh);
	for (qw(Size Flags Height Width PitchOrLinearSize Depth MipMapCount))
	{
		$dds{"dw$_"} = read_DWORD($fh);
	}
	read_DWORD($fh) for 1..11; # dwReserved1

	$dds{dwFlags} = expand_flags($dds{dwFlags}, 'sd');

	$dds{ddpfPixelFormat} = read_DDPIXELFORMAT($fh);
	$dds{ddsCaps} = read_DDCAPS2($fh);

	read_DWORD($fh);# dwReserved2

	bless \%dds, $class;
}

sub getType
{
	my ($self) = @_;
	if (grep $_ eq 'DDPF_RGB', @{$self->{ddpfPixelFormat}{dwFlags}})
	{
		return pack N => (
			((unpack L => 'RRRR') & $self->{ddpfPixelFormat}{dwRBitMask}) |
			((unpack L => 'GGGG') & $self->{ddpfPixelFormat}{dwGBitMask}) |
			((unpack L => 'BBBB') & $self->{ddpfPixelFormat}{dwBBitMask}) |
			((unpack L => 'AAAA') & $self->{ddpfPixelFormat}{dwRGBAlphaBitMask})
		);
	}
	elsif (grep $_ eq 'DDPF_FOURCC', @{$self->{ddpfPixelFormat}{dwFlags}})
	{
		return $self->{ddpfPixelFormat}{dwFourCC}
	}
	else
	{
		die "Unknown type";
	}
}


sub read_DWORD
{
	die "Failed to read DWORD" unless (read $_[0], my $b, 4) == 4;
	return unpack L => $b;
}

sub read_DDPIXELFORMAT
{
	my $r = { map +("dw$_" => read_DWORD($_[0])),
		qw(Size Flags FourCC RGBBitCount RBitMask GBitMask BBitMask RGBAlphaBitMask) };

	$r->{dwFourCC} = pack L => $r->{dwFourCC};
	$r->{dwFlags} = expand_flags($r->{dwFlags}, 'pf');

	return $r;
}

sub read_DDCAPS2
{
	my $r = { map +("dw$_" => read_DWORD($_[0])),
		qw(Caps1 Caps2) };

	$r->{dwCaps1} = expand_flags($r->{dwCaps1}, 'cap1');
	$r->{dwCaps2} = expand_flags($r->{dwCaps2}, 'cap2');

	return $r;
}



my %flag_names = (
	sd => {
		DDSD_CAPS => 0x00000001,
		DDSD_HEIGHT => 0x00000002,
		DDSD_WIDTH => 0x00000004,
		DDSD_PITCH => 0x00000008,
		DDSD_PIXELFORMAT => 0x00001000,
		DDSD_MIPMAPCOUNT => 0x00020000,
		DDSD_LINEARSIZE => 0x00080000,
		DDSD_DEPTH => 0x00800000,
	},
	pf => {
		DDPF_ALPHAPIXELS => 0x00000001,
		DDPF_FOURCC => 0x00000004,
		DDPF_RGB => 0x00000040,
	},
	cap1 => {
		DDSCAPS_COMPLEX => 0x00000008,
		DDSCAPS_TEXTURE => 0x00001000,
		DDSCAPS_MIPMAP => 0x00400000,
	},
	cap2 => {
		DDSCAPS2_CUBEMAP => 0x00000200,
		DDSCAPS2_CUBEMAP_POSITIVEX => 0x00000400,
		DDSCAPS2_CUBEMAP_NEGATIVEX => 0x00000800,
		DDSCAPS2_CUBEMAP_POSITIVEY => 0x00001000,
		DDSCAPS2_CUBEMAP_NEGATIVEY => 0x00002000,
		DDSCAPS2_CUBEMAP_POSITIVEZ => 0x00004000,
		DDSCAPS2_CUBEMAP_NEGATIVEZ => 0x00008000,
		DDSCAPS2_VOLUME => 0x00200000,
	},
);

sub expand_flags
{
	my ($n, $name) = @_;
	return [ grep $flag_names{$name}{$_} & $n, keys %{$flag_names{$name}} ];
}

1;

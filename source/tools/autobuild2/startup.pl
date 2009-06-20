=pod

This script is executed on startup (via a service) on the build server.
It does as little as possible, since it is necessarily frozen
into the static machine image and is hard to update.

The interesting code is passed in a zip file via the EC2 user-data API.

=cut

use strict;
use warnings;

use LWP::Simple();
use Archive::Zip;
use IO::String;

open STDOUT, '>c:/0ad/autobuild/stdout.txt';
open STDERR, '>c:/0ad/autobuild/stderr.txt';
STDOUT->autoflush;
STDERR->autoflush;

# This bit sometimes fails, for reasons I don't understand, so just keep trying it lots of times
for my $i (0..60) {
    mkdir 'd:/0ad' and last;
    warn "($i) $!";
    sleep 1;
}

my $extract_root = 'd:/0ad/autobuild';

extract_user_data();
do "$extract_root/run.pl";

sub extract_user_data {
    my $data = LWP::Simple::get('http://169.254.169.254/2008-09-01/user-data');

    my $zip = Archive::Zip->new();
    $zip->readFromFileHandle(new IO::String($data));
    for ($zip->members) {
        $_->extractToFileNamed("$extract_root/" . $_->fileName);
    }
}

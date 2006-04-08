#!/usr/bin/perl -w

BEGIN { $ENV{CATALYST_ENGINE} ||= 'CGI' }

use strict;
use FindBin;
use lib "$FindBin::Bin/../lib";
use SVNLog;

SVNLog->run;

1;

=head1 NAME

svnlog_cgi.pl - Catalyst CGI

=head1 SYNOPSIS

See L<Catalyst::Manual>

=head1 DESCRIPTION

Run a Catalyst application as cgi.

=head1 AUTHOR

Sebastian Riedel, C<sri@oook.de>

=head1 COPYRIGHT

Copyright 2004 Sebastian Riedel. All rights reserved.

This library is free software, you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

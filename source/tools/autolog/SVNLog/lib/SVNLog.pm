package SVNLog;

use strict;
use warnings;

use YAML ();

use Catalyst qw/FormValidator CDBI::Transaction/;

our $VERSION = '0.01';

__PACKAGE__->config(YAML::LoadFile(__PACKAGE__->path_to('config.yml')));

__PACKAGE__->setup;

sub end : Private {
    my ($self, $c) = @_;

    $c->forward('SVNLog::View::TT') unless $c->response->body;
}

1;

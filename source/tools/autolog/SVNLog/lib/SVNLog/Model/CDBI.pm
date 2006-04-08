package SVNLog::Model::CDBI;

use strict;
use base 'Catalyst::Model::CDBI';

__PACKAGE__->config(
	dsn           => SVNLog->config->{cdbi}{dsn},
	user          => '',
	password      => '',
	options       => {},
	relationships => 1,
	additional_base_classes => [qw/Class::DBI::FromForm Class::DBI::AsForm/],
);

1;

package SVNLog::Model::CDBI::Logentry;

use strict;

__PACKAGE__->add_constructor(recent => "datetime(substr(date, 0, 26)) >= datetime('now', -? || ' days') ORDER BY revision DESC");

__PACKAGE__->might_have(public_msg => 'SVNLog::Model::CDBI::PublicMessage');

1;

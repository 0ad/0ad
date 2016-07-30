package SVNLog::Model::CDBI::PublicMessage;

use strict;

__PACKAGE__->set_sql(log_range => qq{
	SELECT __TABLE__.id
	FROM __TABLE__
	JOIN logentry ON __TABLE__.logentry = logentry.id
	ORDER BY revision DESC
	LIMIT ? OFFSET ?
});

1;

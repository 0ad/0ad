package SVNLog::Controller::LogUpdate;

use strict;
use base 'Catalyst::Controller';

use XML::Simple;
use Text::ASCIITable::Wrap;
use File::Remote;

sub doupdate : Local
{
	my ($self, $c) = @_;
	
	my $latest = SVNLog::Model::CDBI::Logentry->maximum_value_of('revision') || 0;
	my $min = $latest;
	my $max = 'HEAD';
	
	my $svn_cmd_data = '--username '.$c->config->{svn}{username}.' --password '.$c->config->{svn}{password}.' '.$c->config->{svn}{url};
	my $log_xml = `svn log --xml --verbose -r $min:$max $svn_cmd_data`;
	die "SVN request failed" if not (defined $log_xml and $log_xml =~ m~</log>~);
	
	my $log = XMLin($log_xml, ContentKey => 'path');

	my $max_seen = -1;
	
	$c->transaction(sub {
		for my $logentry (ref $log->{logentry} eq 'ARRAY' ? @{$log->{logentry}} : ($log->{logentry}))
		{
			$max_seen = $logentry->{revision} if $logentry->{revision} > $max_seen;
			next if $logentry->{revision} <= $latest;

			my $paths = $logentry->{paths}->{path};
			delete $logentry->{paths};
			
			# Convert <msg></msg> into just a string (because XML::Simple doesn't know that's what it should be)
			$logentry->{msg} = '' if ref $logentry->{msg};
			
			my $entry = SVNLog::Model::CDBI::Logentry->insert($logentry);

			for my $path (ref $paths eq 'ARRAY' ? @$paths : ($paths))
			{
				SVNLog::Model::CDBI::Paths->insert({logentry => $entry->id, filter_copyfrom(%$path)});
			}
		}
	}) or $c->log->error("Transaction failed: " . $c->error->[-1]);
	
	add_default_public_msgs($c);

	my $scp = new File::Remote(rcp => $c->config->{scp}{command});
	$scp->writefile($c->config->{scp}{filename}, generate_text()) or die $!;

	$c->res->body("Updated log to $max_seen.");
}

sub defaultise : Local
{
	my ($self, $c) = @_;
	add_default_public_msgs($c);
	$c->res->body("Done");
}

# (To reset the database:
#  delete from public_message; delete from sqlite_sequence where name = "public_message";
# )

sub add_default_public_msgs
{
	my ($c) = @_;
	
	$c->transaction(sub {
		my @unmessaged = SVNLog::Model::CDBI::Logentry->retrieve_from_sql(qq{
			NOT (SELECT COUNT(*) FROM public_message WHERE public_message.logentry = logentry.id)
			ORDER BY logentry.id
		});
		for my $logentry (@unmessaged)
		{
			my @lines;
			for (split /\n/, $logentry->msg)
			{
				push @lines, $_ if s/^#\s*//;
			}
			my $msg = join "\n", @lines;

			SVNLog::Model::CDBI::PublicMessage->insert({
				logentry => $logentry->id,
				msg => $msg,
			});
		}
	});
}

sub createtext : Local
{
	my ($self, $c) = @_;
	my $out = generate_text();
	$c->res->body("<tt>$out</tt>");
}


sub generate_text
{
	my @logentries = SVNLog::Model::CDBI::Logentry->recent(7);
	
	my $out = '';
	for (@logentries)
	{
		my ($revision, $author, $date, $msg) = ($_->revision, $_->author, $_->date, $_->public_msg);
		next unless defined $msg and $msg->msg;
		
		$date =~ s/T.*Z//;
		$out .= <<EOF;
<b>revision:</b> $revision<br>
<b>author:</b> $author<br>
<b>date:</b> $date<br>
EOF
		my $text = $msg->msg;
		$text =~ s/&/&amp;/g;
		$text =~ s/</&lt;/g;
		$text =~ s/>/&gt;/g;
		$text =~ s/\n/<br>/g;
		$out .= $text . '<hr>';
	}
	
	$out ||= 'Sorry, no data is available right now.';
	return $out;
}

sub filter_copyfrom
{
	my @r = @_;
	s/^(copyfrom)-(rev|path)$/$1_$2/ for @r;
	@r;
}

1;

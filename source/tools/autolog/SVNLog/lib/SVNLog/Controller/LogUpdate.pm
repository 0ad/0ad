package SVNLog::Controller::LogUpdate;

use strict;
use base 'Catalyst::Controller';

use XML::Simple;
use XML::Atom::SimpleFeed;
use Data::UUID;

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

	open my $text_fh, '>', $c->config->{output}{filename} or die $!;
	print $text_fh $self->generate_text($c);
	open my $feed_fh, '>', $c->config->{output}{filename_feed} or die $!;
	print $feed_fh $self->generate_feed($c);

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
	my $out = $self->generate_text($c);
	$c->res->body($out);
}

sub createfeed : Local
{
	my ($self, $c) = @_;
	my $out = $self->generate_feed($c);
	$c->res->body($out);
}

sub get_log_entries
{
	my ($days, $max) = @_;
	my @logentries = ();
	for (SVNLog::Model::CDBI::Logentry->recent($days))
	{
		my $msg = $_->public_msg;
		next unless defined $msg and $msg->msg; # skip ones with empty messages

		push @logentries, $_;

		last if $max and @logentries >= $max;
	}
	return @logentries;
}

sub generate_text
{
	my ($self, $c) = @_;

	my $feed_url = $c->config->{output}{feed_url};
	my $out = <<EOF;
<!DOCTYPE html>
<title>0 A.D. Revision Log</title>
<style>
body {
  color: #fff;
  margin: 3px;
  background-color: #000;
  font-family: Geneva, Arial, Helvetica, sans-serif;
  font-size: 10px;
}
img {
  border: 0;
}
</style>
<a href="$feed_url" target="_top"><img alt="Atom feed" title="Subscribe to feed of revision log" src="/feed-icon-16x16.png" style="float: right"></a>
EOF

	my @logentries = get_log_entries(28, 10);

	for (@logentries)
	{
		my ($revision, $author, $date, $msg) = ($_->revision, $_->author, $_->date, $_->public_msg);

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
		$out .= $text . "\n<hr>\n";
	}
	
	if (not @logentries)
	{
		$out .= 'Sorry, no data is available right now.';
	}

	return $out;
}

sub generate_feed
{
	my ($self, $c) = @_;

	my $uid_gen = new Data::UUID;

	my $feed_url = $c->config->{output}{feed_url};

	my $feed = new XML::Atom::SimpleFeed(
		title => "0 A.D. Revision Log",
		link => "http://www.wildfiregames.com/0ad/",
		link => { rel => 'self', href => $feed_url },
		id => "urn:uuid:" . $uid_gen->create_from_name_str('WFG SVN feed', 'feed'),
	);

	my @logentries = get_log_entries(7);

	for (@logentries)
	{
		my ($revision, $author, $date, $msg) = ($_->revision, $_->author, $_->date, $_->public_msg);

		my $uid = $uid_gen->create_from_name_str('WFG SVN feed', $revision);
		
		$feed->add_entry(
			title => "Revision $revision - ".$msg->msg,
			id => "urn:uuid:$uid",
			author => $author,
			content => $msg->msg,
			published => $date,
			updated => $date,
			link => "http://www.wildfiregames.com/0ad/",
		);
	}
	
	return $feed->as_string;
}

sub filter_copyfrom
{
	my @r = @_;
	s/^(copyfrom)-(rev|path)$/$1_$2/ for @r;
	@r;
}

1;

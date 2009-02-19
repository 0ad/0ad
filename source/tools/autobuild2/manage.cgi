#!/usr/bin/perl -wT

use strict;
use warnings;

use CGI::Simple;
use CGI::Carp qw(fatalsToBrowser);
use DBI;
use Net::Amazon::EC2;
use DateTime::Format::ISO8601;
use Archive::Zip;
use MIME::Base64;
use IO::String;
use Data::Dumper;

my $root = '/var/svn/autobuild';

my %config = load_conf("$root/manage.conf");

my $cgi = new CGI::Simple;

my $user = $cgi->remote_user;
die unless $user;

my $dbh = DBI->connect("dbi:SQLite:dbname=$root/$config{database}", '', '', { RaiseError => 1 });

my $ec2 = new Net::Amazon::EC2(
    AWSAccessKeyId => $config{aws_access_key_id},
    SecretAccessKey => $config{aws_secret_access_key},
);

my $action = $cgi->url_param('action');

if (not defined $action or $action eq 'index') {
    log_action('index');
    print_index('');

} elsif ($action eq 'start') {
    die "Must be POST" unless $cgi->request_method eq 'POST';
    log_action('start');

=pod

Only one instance may run at once, and we need to prevent race conditions.
So:
* Use SQLite as a mutex, so only one CGI script can be trying to start at once
* If the last attempted start was only a few seconds ago, reject this one since
  it's probably a double-click or something
* Check the list of active machines. If it's non-empty, reject this request.
* Otherwise, start the new machine.

=cut

    $dbh->begin_work;
    die unless 1 == $dbh->do('UPDATE state SET value = ? WHERE key = ?', undef, $$, 'process_mutex');
    my ($last_start) = $dbh->selectrow_array('SELECT value FROM state WHERE key = ?', undef, 'last_start');
    my $last_start_age = time() - $last_start;
    die "Last start was only $last_start_age seconds ago - please try again later."
        if $last_start_age < 10;
    die unless 1 == $dbh->do('UPDATE state SET value = ? WHERE key = ?', undef, time(), 'last_start');

    my $instances_status = get_ec2_status_table();
    for (@$instances_status) {
        if ($_->{instance_state} ne 'terminated') {
            die "Already got an active instance ($_->{instance_id}) - can't start another one.";
        }
    }

    # No instances are currently active, and nobody else is in this
    # instance-starting script, so it's safe to start a new one

    my $instances = start_ec2_instance();

    $dbh->commit;

    for (@$instances) {
        $dbh->do('INSERT INTO instances VALUES (?, ?)', undef, $_->{instance_id}, DateTime->now->iso8601);
    }

    print_index(generate_status_table($instances, 'Newly started instance') . '<hr>');

} elsif ($action eq 'stop') {
    die "Must be POST" unless $cgi->request_method eq 'POST';
    my $id = $cgi->url_param('instance_id');
    $id =~ /\Ai-[0-9a-f]+\z/ or die "Invalid instance_id";
    log_action('stop', $id);
    stop_ec2_instance($id);
    print_index("<strong>Stopping instance $id</strong><hr>");

} elsif ($action eq 'console') {
    my $id = $cgi->url_param('instance_id');
    $id =~ /\Ai-[0-9a-f]+\z/ or die "Invalid instance_id";
    log_action('console', $id);
    my $output = get_console_output($id);
    $output =~ s/</&lt;/g;
    print_index("<strong>Console output from $id:</strong><pre>\n$output</pre><hr>");

} elsif ($action eq 'activity') {
    my $days = int $cgi->url_param('days') || 7;
    print_activity($days);

} else {
    log_action('invalid', $action);
    die "Invalid action '$action'";
}

$dbh->disconnect;

sub print_index {
    my ($info) = @_;

    my $instances_status = get_ec2_status_table();
    my $got_active_instance;
    for (@$instances_status) {
        if ($_->{instance_state} ne 'terminated') {
            $got_active_instance = $_->{instance_id} || '?';
        }
    }

    my $status = generate_status_table($instances_status, 'Current EC2 machine status');
    print <<EOF;
Pragma: no-cache
Content-Type: text/html

<!DOCTYPE html>
<title>0 A.D. autobuild manager</title>
<link rel="stylesheet" href="manage.css">
<p>Hello <i>$user</i>.</p>
<p>
 <a href="?action=index">Refresh status</a> |
 <a href="http://wfg-autobuild-logs.s3.amazonaws.com/logindex.html">View build logs</a>
</p>
<hr>
$info
$status
<p>
EOF

    if ($got_active_instance) {
        print qq{<button disabled title="Already running an instance ($got_active_instance)">Start new build</button>\n};
    } else {
        print qq{<form action="?action=start" method="post" onsubmit="return confirm('Are you sure you want to start a new build?')"><button type="submit">Start new build</button></form>\n};
    }
}

sub log_action {
    my ($action, $params) = @_;
    $dbh->do('INSERT INTO activity (user, ip, ua, action, params) VALUES (?, ?, ?, ?, ?)',
        undef, $user, $cgi->remote_addr, $cgi->user_agent, $action, $params);
}

sub print_activity {
    my ($days) = @_;
    print <<EOF;
Content-Type: text/html

<!DOCTYPE html>
<title>0 A.D. autobuild activity log</title>
<link rel="stylesheet" href="manage.css">
<table>
<tr><th>Date<th>User<th>IP<th>Action<th>Params<th>UA
EOF

    my $sth = $dbh->prepare("SELECT * FROM activity WHERE timestamp > datetime('now', ?) ORDER BY id DESC");
    $sth->execute("-$days day");
    while (my $row = $sth->fetchrow_hashref) {
        print '<tr>';
        print '<td>'.$cgi->escapeHTML($row->{$_}) for qw(timestamp user ip action params ua);
        print "\n";
    }

print <<EOF;
</table>
EOF

}

sub generate_status_table {
    my ($instances, $caption) = @_;
    my @columns = (
        [ reservation_id => 'Reservation ID' ],
        [ instance_id => 'Instance ID' ],
        [ instance_state => 'State' ],
        [ image_id => 'Image ID '],
        [ dns_name => 'DNS name' ],
        [ launch_time => 'Launch time' ],
        [ reason => 'Last change' ],
    );
    my $count = @$instances;
    my $status = qq{<table id="status">\n<caption>$caption &mdash; $count instances</caption>\n<tr>};
    for (@columns) { $status .= qq{<th>$_->[1]}; }
    $status .= qq{\n};

    for my $item (@$instances) {
        $status .= qq{<tr>};
        for (@columns) {
            if ($_->[0] eq 'launch_time') {
                my $t = DateTime::Format::ISO8601->parse_datetime($item->{$_->[0]});
                my $now = DateTime->now();
                my $diff = $now - $t;
                my ($hours, $minutes) = $diff->in_units('hours', 'minutes');
                my $age = "$minutes minutes ago";
                $age = "$hours hours, $age" if $hours;
                $status .= qq{<td>$item->{$_->[0]} ($age)};
            } else {
                $status .= qq{<td>$item->{$_->[0]}};
            }
        }
        $status .= qq{<td><a href="?action=console;instance_id=$item->{instance_id}">Console output</a>\n};
        $status .= qq{<td><form action="?action=stop;instance_id=$item->{instance_id}" method="post" onsubmit="return confirm('Are you sure you want to terminate this instance?')"><button type="submit">Terminate</button></form>\n};
    }

    $status .= qq{</table>};
    return $status;
}

sub flatten_instance {
    my ($reservation, $instance) = @_;
    return {
        reservation_id => $reservation->reservation_id,
        instance_id => $instance->instance_id,
        instance_state => $instance->instance_state->name,
        image_id => $instance->image_id,
        dns_name => $instance->dns_name,
        launch_time => $instance->launch_time,
        reason => $instance->reason,
    };
}

sub get_ec2_status_table {
#     return [ ];
#     return [ {
#         reservation_id => 'r-12345678',
#         instance_id => 'i-12345678',
#         instance_state => 'pending',
#         image_id => 'ami-12345678',
#         dns_name => '',
#         launch_time => '2008-12-30T17:14:22.000Z',
#         reason => '',
#     } ];

    my $reservations = $ec2->describe_instances();
    my @ret = ();
    for my $reservation (@$reservations) {
        push @ret, map flatten_instance($reservation, $_), @{$reservation->instances_set};
    }
    return \@ret;
}

sub get_console_output {
    my ($instance_id) = @_;
    my $output = $ec2->get_console_output(InstanceId => $instance_id);
    return "(Last updated: ".$output->timestamp.")\n".$output->output;
}

sub start_ec2_instance {
#     return [ {
#         reservation_id => 'r-12345678',
#         instance_id => 'i-12345678',
#         instance_state => 'pending',
#         image_id => 'ami-12345678',
#         dns_name => '',
#         launch_time => '2008-12-30T17:14:22.000Z',
#         reason => '',
#     } ];

    my $user_data = create_user_data();

    my $reservation = $ec2->run_instances(
        ImageId => $config{image_id},
        MinCount => 1,
        MaxCount => 1,
        KeyName => $config{key_name},
        SecurityGroup => $config{security_group},
        UserData => encode_base64($user_data),
        InstanceType => $config{instance_type},
        'Placement.AvailabilityZone' => $config{availability_zone},
    );

    if (ref $reservation eq 'Net::Amazon::EC2::Errors') {
        die "run_instances failed:\n".(Dumper $reservation);
    }

    return [ map flatten_instance($reservation, $_), @{$reservation->instances_set} ];
}

sub create_user_data {
    my @files = qw(run.pl run.conf);

    my $zip = new Archive::Zip;
    for (@files) {
        $zip->addFile("$root/$_", "$_") or die "Failed to add $root/$_ to zip";
    }
    my $fh = new IO::String;
    if ($zip->writeToFileHandle($fh) != Archive::Zip::AZ_OK) {
        die "writeToFileHandle failed";
    }
    return ${$fh->string_ref};
}

sub stop_ec2_instance {
    my ($instance_id) = @_;

#     return;

    $ec2->terminate_instances(
        InstanceId => $instance_id,
    );
}

sub load_conf {
    my ($filename) = @_;
    open my $f, '<', $filename or die "Failed to open $filename: $!";
    my %c;
    while (<$f>) {
        if (/^(.+?): (.+)/) {
            $c{$1} = $2;
        }
    }
    return %c;
}

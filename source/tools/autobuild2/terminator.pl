#!/usr/bin/perl

=pod

To prevent runaway server instances eating up lots of money, this script
is run frequently by cron and will kill any that have been running for too long.

(The instances attempt to terminate themselves once they're finished building,
so typically this script won't be required.)

To cope with dodgy clock synchronisation, two launch times are used per instance:
the launch_time reported by EC2's describe_instances, and the local_launch_time
stored in SQLite by manage.cgi. The process is killed if either time exceeds
the cutoff limit.

=cut

use strict;
use warnings;

use DBI;
use Net::Amazon::EC2;
use DateTime::Format::ISO8601;

my $root = '/var/svn/autobuild';

my %config = load_conf("$root/manage.conf");

my $dbh = DBI->connect("dbi:SQLite:dbname=$root/$config{database}", '', '', { RaiseError => 1 });

my $now = DateTime->now;

print "\n", $now->iso8601, "\n";

my $cutoff = DateTime::Duration->new(minutes => 55);

my $ec2 = new Net::Amazon::EC2(
    AWSAccessKeyId => $config{aws_access_key_id},
    SecretAccessKey => $config{aws_secret_access_key},
);

my @instances;

my $reservations = $ec2->describe_instances();
for my $reservation (@$reservations) {
    for my $instance (@{$reservation->instances_set}) {
        my ($local_launch_time) = $dbh->selectrow_array('SELECT local_launch_time FROM instances WHERE instance_id = ?', undef, $instance->instance_id);
        push @instances, {
            id => $instance->instance_id,
            state => $instance->instance_state->name,
            launch_time => $instance->launch_time,
            local_launch_time => $local_launch_time,
        };
    }
}

use Data::Dumper; print Dumper \@instances;
# @instances = ( {
#     id => 'i-12345678',
#     state => 'pending',
#     launch_time => '2008-12-30T17:14:22.000Z',
#     local_launch_time => '2008-12-30T17:14:22.000Z',
# } );

for my $instance (@instances) {
    next if $instance->{state} eq 'terminated';

    my $too_old = 0;
    my $age = $now - DateTime::Format::ISO8601->parse_datetime($instance->{launch_time});
    $too_old = 1 if DateTime::Duration->compare($age, $cutoff) > 0;
    if (defined $instance->{local_launch_time}) {
        my $local_age = $now - DateTime::Format::ISO8601->parse_datetime($instance->{local_launch_time});
        $too_old = 1 if DateTime::Duration->compare($local_age, $cutoff) > 0;
    }
    next unless $too_old;

    print "Terminating $instance->{id}, launched at $instance->{launch_time} / ", ($instance->{local_launch_time} || ''), "\n";

    log_action('terminate', $instance->{id});

    $ec2->terminate_instances(
        InstanceId => $instance->{id},
    );
}


$dbh->disconnect;

sub log_action {
    my ($action, $params) = @_;
    $dbh->do('INSERT INTO activity (user, ip, ua, action, params) VALUES (?, ?, ?, ?, ?)',
        undef, 'local', '', '', $action, $params);
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

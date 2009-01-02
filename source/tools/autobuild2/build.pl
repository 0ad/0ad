# Build script - does the actual building of the project

use strict;
use warnings;

use constant EXIT_BUILDCOMPLETE => 0;
use constant EXIT_FAILED => 1;

my %config = load_conf("c:\\0ad\\autobuild\\aws.conf");

my $svn_trunk = "e:\\svn";
my $temp_trunk = "d:\\0ad";
my $log_dir = "d:\\0ad\\buildlogs";
my $vcbuild = "$svn_trunk\\source\\tools\\autobuild2\\vcbuild_env.bat";

my $time_start = time();

eval { # catch deaths

# Clean the output directory, just in case it's got old junk left over somehow
`rmdir /q /s $temp_trunk 2>&1`; # (ignore failures, they don't matter)
# This bit sometimes fails, for reasons I don't understand, so just keep trying it lots of times
for my $i (0..60) {
    mkdir $temp_trunk and last;
    warn "($i) $!";
    sleep 1;
}
mkdir $log_dir or die $!;

# Capture all output
open STDOUT, '>', "$log_dir\\build_stdout.txt";
open STDERR, '>', "$log_dir\\build_stderr.txt";
open BUILDLOG, '>', "$log_dir\\buildlog.txt" or die $!;

add_to_buildlog("Starting build");

chdir $svn_trunk or die $!;

# Copy all the necessary files into the temporary working area

# For some directories, do a real copy
for (qw(build source libraries))
{
    add_to_buildlog("xcopying $_");
    `xcopy /e $svn_trunk\\$_ $temp_trunk\\$_\\ 2>&1`;
    die "xcopy $_: $?" if $?;
}

# # For other directories (which we're only going to read), do a 'junction' (like a symbolic link) because it's faster
# #
# # Actually don't, because it's easier to not have to install the junction tool
# for (qw(source libraries))
# {
#     `$junction $temp_trunk\\$_ $svn_trunk\\$_`;
#     die "junction $_: $?" if $?;
# }

# Create the workspace files

add_to_buildlog("Running update-workspaces");
chdir "$temp_trunk\\build\\workspaces" or die $!;
my $updateworkspaces_output = `update-workspaces.bat 2>&1`;
add_to_buildlog($updateworkspaces_output);
die $? if $?;

# Create target directories for built files

mkdir "$temp_trunk\\binaries" or die $!;
mkdir "$temp_trunk\\binaries\\system" or die $!;

# Do the Release build

add_to_buildlog("Running vcbuild");
my $build_output = `$vcbuild /time /M2 /logfile:$log_dir\\build_vcbuild.txt vc2008\\pyrogenesis.sln "Release|Win32" 2>&1`;
add_to_buildlog($build_output);
die $? if ($? and $? != 32768);
# { open my $f, '>', "$temp_trunk/binaries/system/pyrogenesis.exe"; print $f "exe\n" }
# { open my $f, '>', "$temp_trunk/binaries/system/pyrogenesis.pdb"; print $f "pdb\n" }

# Copy the output

add_to_buildlog("Copying generated binaries");
`copy $temp_trunk\\binaries\\system\\pyrogenesis.exe $svn_trunk\\binaries\\system\\`;
die $? if $?;
`copy $temp_trunk\\binaries\\system\\pyrogenesis.pdb $svn_trunk\\binaries\\system\\`;
die $? if $?;

# Commit to SVN

my $svn_output = `svn commit --username $config{svn_username} --password $config{svn_password} $svn_trunk\\binaries\\system\\pyrogenesis.exe $svn_trunk\\binaries\\system\\pyrogenesis.pdb --message "Automated build." 2>&1`;
add_to_buildlog($svn_output);
die $? if $?;

}; # end of eval

if ($@)
{
    warn $@;
    quit(EXIT_FAILED);
}
else
{
    quit(EXIT_BUILDCOMPLETE);
}


# Exit, after copying the current log files over the previous ones
sub quit
{
    my $time_end = time();
    my $time_taken = $time_end - $time_start;
    add_to_buildlog("Build completed with code $_[0] - took $time_taken seconds.");

    exit($_[0]);
}


sub add_to_buildlog
{
    print BUILDLOG +(gmtime time)."\n$_[0]\n--------------------------------------------------------------------------------\n";
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

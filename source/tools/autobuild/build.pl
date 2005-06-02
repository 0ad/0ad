# Build script - does the actual building of the project

use strict;
use warnings;

use constant EXIT_BUILDCOMPLETE => 0;
use constant EXIT_NOTCOMPILED => 1;
use constant EXIT_FAILED => 2;
use constant EXIT_ABORTED => 3;

my $svn_trunk = 'c:\0ad\trunk';
my $temp_trunk = 'r:\trunk';
my $output_dir = 'c:\0ad\builds';
my $log_dir = 'c:\0ad\autobuild';

my $sevenz = '"C:\Program Files\7-Zip\7z.exe"';

#my $vcbuild = '"C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\vcbuild"';
# except I need to call vcvars32.bat then "vcbuild.exe /useenv", since the VS registry settings don't always exist
my $vcbuild = '"C:\0ad\autobuild\vcbuild_env.bat"';

my $time_start = time();

eval { # catch deaths

# Capture all output
open STDOUT, '>', "$log_dir\\build_stdout_temp.txt";
open STDERR, '>', "$log_dir\\build_stderr_temp.txt";
open BUILDLOG, '>', "$log_dir\\buildlog_temp.txt" or die $!;

our ($username, $password);
do 'login_details.pl' or die "Cannot find login details: $! / $@";
# login_details.pl contains:
#    $::username = "philip";
#    $::password = "something";
# (but with a valid password, which I'm not going to tell you)


add_to_buildlog("Starting build at ".(gmtime($time_start))." GMT.\n");

chdir $svn_trunk or die $!;

if (grep { $_ eq '--commitlatest' } @ARGV)
{
	add_to_buildlog("Committing latest code");

	### Find the latest revision number ###
	opendir my $dir, $output_dir or die $!;
	my @revs = grep /^\d+$/, readdir $dir;
	die unless @revs;
	my $rev = (sort { $b <=> $a } @revs)[0];
	
	add_to_buildlog("Committing ps.exe for revision $rev");

	### Copy ps.exe and ps.pdb over the SVN copy ###	
	`copy $output_dir\\$rev\\ps.exe $svn_trunk\\binaries\\system\\`;
	die $? if $?;
	`copy $output_dir\\$rev\\ps.pdb $svn_trunk\\binaries\\data\\`;
	die $? if $?;
	
	### Commit ps.exe and ps.pdb ###
	my $svn_output = `svn commit binaries\\system\\ps.exe binaries\\data\\ps.pdb --username $username --password $password --message "Automated build." 2>&1`;
	add_to_buildlog($svn_output);
	die $? if $?;
	
	# Just exit, and don't overwrite the earlier logs
	exit(EXIT_NOTCOMPILED);
}


### Update from SVN ###

my $svn_output = `svn update --username $username --password $password 2>&1`;
add_to_buildlog($svn_output);
die $? if $?;

allow_abort();

$svn_output =~ /^(?:Updated to|At) revision (\d+)\.$/m or die;
my $svn_revision = $1;

if ($svn_output =~ m~^.  (source(?![/\\]tools)|build|libraries)~m)
{
	# The source has been updated.
	# ('source' means something in the source, build, or libraries directories, excluding source/tools)
}
else
{
	# Nothing's changed. Build anyway?
	if (grep { $_ eq '--force' } @ARGV)
	{
		# Yes
	}
	else
	{
		add_to_buildlog("*** Build $svn_revision not needed - no source changes ***");
		# Just exit, and don't overwrite the earlier logs
		exit(EXIT_NOTCOMPILED);
	}
}

### Check whether we've already built this ###

if (-e "$output_dir\\$svn_revision")
{
	add_to_buildlog("*** Build $svn_revision already exists ***");
	# Just exit, and don't overwrite the earlier logs
	exit(EXIT_NOTCOMPILED);
}

### Clean the RAM disk ###

`rmdir /q /s $temp_trunk 2>&1`;
# ignore failures - the RAM disk might have been recently reset

### Copy all the necessary files onto it ###

for (qw(source libraries build))
{
	`xcopy /e $svn_trunk\\$_ $temp_trunk\\$_\\ 2>&1`;
	die "xcopy $_: $?" if $?;
	allow_abort();
}

### Create the workspace files ###

chdir "$temp_trunk\\build\\workspaces" or die $!;
my $updateworkspaces_output = `update-workspaces.bat 2>&1`;
add_to_buildlog($updateworkspaces_output);
die $? if $?;

### Create target directories for built files ###

mkdir "$temp_trunk\\binaries" or die $!;
mkdir "$temp_trunk\\binaries\\system" or die $!;
mkdir "$temp_trunk\\binaries\\data" or die $!;

allow_abort();

### Do the Testing build ###

my $build_output1 = `$vcbuild /time vc2003\\pyrogenesis.sln Testing 2>&1`;
add_to_buildlog($build_output1);
die $? if ($? and $? != 32768); # 32768 seems to be returned when it succeeds

allow_abort();

### Copy the output ###

`mkdir $output_dir\\temp`;
# ignore failures - this might already exist if the last build was aborted

`copy $temp_trunk\\binaries\\system\\ps_test.exe $output_dir\\temp\\`;
die $? if $?;
`copy $temp_trunk\\binaries\\data\\ps_test.pdb $output_dir\\temp\\`;
die $? if $?;

### Clean up unnecessary files to save space ###

`rmdir /q /s $temp_trunk\\build\\workspaces\\vc2003\\obj\\Testing 2>&1`;
die $? if $?;
`rmdir /q /s $temp_trunk\\binaries 2>&1`;
die $? if $?;

allow_abort();

### Recreate targets for built files ###

mkdir "$temp_trunk\\binaries" or die $!;
mkdir "$temp_trunk\\binaries\\system" or die $!;
mkdir "$temp_trunk\\binaries\\data" or die $!;

### Do the Release build ###

my $build_output2 = `$vcbuild /time vc2003\\pyrogenesis.sln Release 2>&1`;
add_to_buildlog($build_output2);
die $? if ($? and $? != 32768);

### Copy the output ###

`copy $temp_trunk\\binaries\\system\\ps.exe $output_dir\\temp\\`;
die $? if $?;
`copy $temp_trunk\\binaries\\data\\ps.pdb $output_dir\\temp\\`;
die $? if $?;

### Store the output permanently ###

rename "$output_dir\\temp", "$output_dir\\$svn_revision" or die $!;

### and make a compressed archive ###

chdir "$output_dir\\$svn_revision" or die $!;
`$sevenz a -mx9 -bd -sfx7zC.sfx ..\\$svn_revision.exe`;
die $? if $?;

# (TODO: delete the non-archived data when it's not needed, to save disk space)

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
	add_to_buildlog("\nBuild completed at ".(gmtime($time_end))." GMT - took $time_taken seconds.");
	
	close BUILDLOG;
	rename "$log_dir\\buildlog_temp.txt", "$log_dir\\buildlog.txt" or die $!;
	close STDOUT;
	rename "$log_dir\\build_stdout_temp.txt", "$log_dir\\build_stdout.txt" or die $!;
	close STDERR;
	rename "$log_dir\\build_stderr_temp.txt", "$log_dir\\build_stderr.txt" or die $!;
	exit($_[0]);
}


sub add_to_buildlog
{
	print BUILDLOG "$_[0]\n--------------------------------------------------------------------------------\n";
}

# Call this at strategic moments, to allow builds to be aborted (when e.g. there's another revision just come in that needs to be built instead)
sub allow_abort
{
	if (-e "$log_dir\\build_abort")
	{
		add_to_buildlog("*** Build aborted ***\n");
		unlink "$log_dir\\build_abort";
		quit(EXIT_ABORTED);
	}
}
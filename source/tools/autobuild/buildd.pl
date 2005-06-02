# Build daemon - receives notifications of commits, runs the build process (ensuring there's only one copy at once), commits the built files, and provides access to all earlier builds and the build log of the latest.

use warnings;
use strict;

# Exit codes used by build.pl:
use constant EXIT_BUILDCOMPLETE => 0;
use constant EXIT_NOTCOMPILED => 1;
use constant EXIT_FAILED => 2;
use constant EXIT_ABORTED => 3;

use POE qw(Component::Server::TCP Filter::HTTPD Filter::Line);
use HTTP::Response;

use Win32::Process;

my $build_process;       # stores Win32::Process handle
my $build_required = 0;  # set by commits, cleared by builds
my $commit_required = 0; # stores the time when it should happen, or 0 if never
my $build_start_time;    # time that the most recent build started
my $last_exit_code;      # exit code of the most recent completed build
my $build_active = 0;    # 0 => build process not running; 1 => build process running within the past second

use constant COMMIT_DELAY => 60*60; # seconds to wait after a code commit before committing ps.exe

open my $logfile, '>>', 'access_log' or die "Error opening access_log: $!";
$logfile->autoflush();
sub LOG {
	my ($pkg, $file, $line) = caller;
	my $msg = localtime()." - $file:$line - @_\n";
	print $logfile $msg;
	print $msg;
}

POE::Component::Server::TCP->new(
	Alias        => "web_server",
	Port         => 57470,
	ClientFilter => 'POE::Filter::HTTPD',

	ClientInput => sub {
		my ($kernel, $heap, $request) = @_[KERNEL, HEAP, ARG0];

		# Respond to errors in the client's request
		if ($request->isa("HTTP::Response")) {
			$heap->{client}->put($request);
			$kernel->yield("shutdown");
			return;
		}

		LOG $heap->{remote_ip}." - ".$request->uri->as_string;

		my $response = HTTP::Response->new(200);

		my $url = $request->uri->path;
		
		if ($url eq '/commit_notify.html')
		{
			$build_required = 1;
			abort_build();
			$response->push_header('Content-type', 'text/plain');
			$response->content("Build initiated.");
		}
		elsif ($url eq '/commit_latest.html')
		{
			$commit_required = time();
			$response->push_header('Content-type', 'text/plain');
			$response->content("Commit initiated.");
		}
		elsif ($url eq '/abort_build.html')
		{
			abort_build();
			$response->push_header('Content-type', 'text/plain');
			$response->content("Build aborted.");
		}
		elsif ($url eq '/status.html')
		{
			$response->push_header('Content-type', 'text/html');
			my $text = <<EOF;
<html><body>
@{[ $build_active ? "Build in progress - ".(time()-$build_start_time)." seconds elapsed." : "Build not in progress." ]}
<br><br>
Last build status: @{[do{
	if ($last_exit_code == EXIT_BUILDCOMPLETE or $last_exit_code == EXIT_NOTCOMPILED) {
		"Succeeded";
	} elsif ($last_exit_code == EXIT_ABORTED) {
		"Aborted"
	} else {
		"FAILED ($last_exit_code)"
	}
}]}.
<br><br>
Build log: (<a href="logs.html">complete logs</a>)
<br><br>
EOF
			my $buildlog = do { local $/; my $f; open $f, 'buildlog.txt' and <$f> };
			if ($buildlog)
			{
				$buildlog =~ s/&/&amp;/g;
				$buildlog =~ s/</&lt;/g;
				$buildlog =~ s/>/&gt;/g;
			}
			else
			{
				$buildlog = "(Error opening build log)";
			}
			
			$text .= "<pre>$buildlog</pre>";

			$text .= qq{</body></html>};
			$response->content($text);
		}
		elsif ($url eq '/logs.html')
		{
			$response->push_header('Content-type', 'text/plain');
			my $text;
			for my $n (qw(buildlog build_stdout build_stderr))
			{
				my $filedata = do { local $/; my $f; open $f, "$n.txt" and <$f> };
				$text .= "--------------------------------------------------------------------------------\n";
				$text .= "$n\n";
				$text .= "--------------------------------------------------------------------------------\n";
				$text .= $filedata;
				$text .= "\n\n\n";
			}
			$response->content($text);
		}
		elsif ($url eq '/filelist.html')
		{
			my $output = '';
			eval {
				opendir my $d, "..\\builds" or die $!;
				my @revs;
				for (grep /^\d+\.exe$/, readdir $d)
				{
					/^(\d+)/;
					push @revs, $1;
				}
				$output .= qq{<a href="download/$_.exe">$_</a>  (}.int( (stat "..\\builds\\$_.exe")[7]/1024 ).qq{k)\n} for sort { $b <=> $a } @revs;
			};
			if ($@)
			{
				LOG "filelist failed: $@";
				$response->push_header('Content-type', 'text/plain');
				$response->content("Internal error.");
			}
			else
			{
				$response->push_header('Content-type', 'text/html');
				$response->content("<html><body><pre>$output</pre></body></html>");
			}
		}
		elsif ($url =~ m~/download/(\d+).exe~)
		{
			my $rev = $1;
			if (-e "..\\builds\\$rev.exe" and open my $f, "..\\builds\\$rev.exe")
			{
				binmode $f;
				$response->push_header('Content-type', 'application/octet-stream');
				$response->content(do{local $/; <$f>});
			}
			else
			{
				LOG "Error serving file $url";
				$response->push_header('Content-type', 'text/plain');
				$response->content("File not found.");
			}
		}
		elsif ($url eq '/favicon.ico')
		{
			$response = HTTP::Response->new(404);
			$response->push_header('Content-type', 'text/html');
			$response->content($response->error_as_HTML);
		}
		else
		{
			$response->push_header('Content-type', 'text/plain');
			$response->content("Unrecognised request.");
		}

		$heap->{client}->put($response);
		$kernel->yield("shutdown");
	}
);

POE::Session->create(
	inline_states => {
		_start => sub {
			$_[KERNEL]->delay(tick => 1);
		},
		tick => sub {
			$_[KERNEL]->delay(tick => 1);
			
			if ($build_active and not build_is_running())
			{
				# Build has just completed.
				
				$last_exit_code = get_exit_code();
				$build_active = 0;
				undef $build_process;

				LOG "Build complete ($last_exit_code)";
				
				if ($last_exit_code == EXIT_BUILDCOMPLETE)
				{
					$commit_required = $build_start_time + COMMIT_DELAY;
				}
				else
				{
					$commit_required = 0;	
				}
			}

			if ($build_required and not $build_active)
			{
				start_build();
			}
			elsif ($commit_required and time() >= $commit_required)
			{
				start_build(commit => 1);
				$commit_required = 0;
			}
		}
	}
);

LOG "Starting kernel";
$poe_kernel->run();
LOG "Kernel exited";
exit 0;


sub build_is_running
{
	my $exit_code = get_exit_code();
	return (defined $exit_code and $exit_code == 259);
}

sub get_exit_code
{
	return undef if not $build_process;
	my $exit_code;
	$build_process->GetExitCode($exit_code);
	return $exit_code;
}

sub abort_build
{
	LOG "Aborting build";

	open my $f, '>', 'build_abort';	
}

sub start_build
{
	my %params = @_;
	
	LOG "Starting build";
	
	unlink 'build_abort';

	$build_start_time = time;
	
	Win32::Process::Create(
		$build_process,
		"c:\\perl\\bin\\perl.exe",
		"perl build.pl" . ($params{commit} ? ' --commitlatest' : ''),
		0,
		CREATE_NO_WINDOW,
		"c:\\0ad\\autobuild") or die "Error spawning build script: ".Win32::FormatMessage(Win32::GetLastError());

	$build_required = 0;
	$commit_required = 0;
	$build_active = 1;
}
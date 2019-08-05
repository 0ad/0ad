def AtlasOption = ""
def GlooxOption = ""
def output = ""
def jobs = "2"
pipeline {
	agent { label 'WindowsSlave' }

	parameters {
		booleanParam(name: 'pyrogenesis', defaultValue: true, description: 'Build and commit the main executable.')
		booleanParam(name: 'atlas', defaultValue: false, description: 'Build and commit the AtlasUI library.')
		booleanParam(name: 'collada', defaultValue: false, description: 'Build and commit the Collada library.')
		booleanParam(name: 'gloox', defaultValue: false, description: 'Build and commit the gloox wrapper.')
	}

	stages {
		stage("Checkout") {
			options {
				// Account for network errors
				retry(3)
			}
			steps {
				script {
					try {
						checkout([$class: 'SubversionSCM', locations: [[credentialsId: 'redacted', local: '.', remote: 'https://svn.wildfiregames.com/svn/ps/trunk']]])
					} catch(e) {
						bat "svn cleanup"
						sleep 300
						throw e
					}
				}
				bat "svn cleanup"
			}
		}

		stage('Setup workspace') {
			steps {
			   	bat "del binaries\\system\\pyrogenesis.pdb binaries\\system\\pyrogenesis.exe"
				script {
					if (env.atlas == 'true') {
						echo "atlas is enabled"
						AtlasOption = "--atlas"
						bat 'robocopy /MIR C:\\wxwidgets3.0.4\\lib libraries\\win32\\wxwidgets\\lib ^& IF %ERRORLEVEL% LEQ 1 exit 0'
						bat 'robocopy /MIR C:\\wxwidgets3.0.4\\include libraries\\win32\\wxwidgets\\include ^& IF %ERRORLEVEL% LEQ 1 exit 0'
						bat "del binaries\\system\\AtlasUI.dll"
					}
					if (env.gloox == 'true') {
						echo "gloox is enabled"
						GlooxOption = "--build-shared-glooxwrapper"
						bat "del binaries\\system\\glooxwrapper.pdb binaries\\system\\glooxwrapper.lib binaries\\system\\glooxwrapper.dll"
						bat "del binaries\\system\\glooxwrapper_dbg.pdb binaries\\system\\glooxwrapper_dbg.lib binaries\\system\\glooxwrapper_dbg.dll"
					}
					if (env.collada == 'true') {
						echo "collada is enabled"
						bat "del binaries\\system\\Collada.dll"
					}
					output = bat(returnStdout: true, script: 'svnversion source -n').trim()
					output = output.readLines().drop(1).join("")
				}
				bat "cd build\\workspaces && update-workspaces.bat ${AtlasOption} ${GlooxOption} --large-address-aware --jenkins-tests"
				bat "echo L\"${output}\" > build\\svn_revision\\svn_revision.txt"
			}
		}

		stage ('Build') {
			steps {
				bat("cd build\\workspaces\\vc2015 && MSBuild.exe pyrogenesis.sln /m:${jobs} /p:PlatformToolset=v140_xp /t:pyrogenesis /t:test /p:Configuration=Release")
			}
		}

		stage ('Build debug glooxwrapper') {
			when { environment name: 'gloox', value: 'true'}
			steps {
					bat("cd build\\workspaces\\vc2015 && MSBuild.exe pyrogenesis.sln /m:${jobs} /p:PlatformToolset=v140_xp /t:glooxwrapper /p:Configuration=Debug")
			}
		}

		stage ('Tests') {
			steps {
				bat 'binaries\\system\\test.exe'
			}
		}

		stage ('Commit') {
		    options {
				// Account for network errors
				retry(3)
			}
			steps {
				bat "svn changelist --remove --recursive --cl commit ."
				script {
	  				if (env.pyrogenesis == 'true') {
						bat "svn changelist commit binaries\\system\\pyrogenesis.pdb binaries\\system\\pyrogenesis.exe"
					}
					if (env.atlas == 'true') {
						bat "svn changelist commit binaries\\system\\AtlasUI.dll"
					}
					if (env.collada == 'true') {
						bat "svn changelist commit binaries\\system\\Collada.dll"
					}
					if (env.gloox == 'true') {
						bat "svn changelist commit binaries\\system\\glooxwrapper.dll binaries\\system\\glooxwrapper.lib binaries\\system\\glooxwrapper.pdb binaries\\system\\glooxwrapper_dbg.dll binaries\\system\\glooxwrapper_dbg.lib binaries\\system\\glooxwrapper_dbg.pdb"
					}
				}
				withCredentials([usernamePassword(credentialsId: 'redacted', passwordVariable: 'SVNPASS', usernameVariable: 'SVNUSER')]) {
					bat 'svn commit --username %SVNUSER% --password %SVNPASS% --no-auth-cache --non-interactive --changelist commit -m "[Windows] Automated build."'
				}
			}
		}
	}

	post {
		always {
			bat "svn revert -R ."
		}
	}
}

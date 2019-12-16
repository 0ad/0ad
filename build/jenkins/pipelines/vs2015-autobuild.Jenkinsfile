/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// This pipeline builds Windows binaries. It is run at most daily, every
// time a commit touches the source code.

def AtlasOption = ""
def GlooxOption = ""
def AtlasPrj = ""
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
						AtlasPrj = "/t:AtlasUI"
						bat "(robocopy /MIR C:\\wxwidgets3.0.4\\lib libraries\\win32\\wxwidgets\\lib) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
						bat "(robocopy /MIR C:\\wxwidgets3.0.4\\include libraries\\win32\\wxwidgets\\include) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
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
				bat("cd build\\workspaces\\vc2015 && MSBuild.exe pyrogenesis.sln /m:${jobs} /p:PlatformToolset=v140_xp /t:pyrogenesis ${AtlasPrj} /t:test /p:Configuration=Release")
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

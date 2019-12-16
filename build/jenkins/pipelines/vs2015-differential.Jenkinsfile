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

// This pipeline is used to build patches on MSVC 14.0 (Visual Studio 2015).

def jobs = "2"
pipeline {
	agent { label 'WindowsSlave' }
	options {
		skipDefaultCheckout()
	}

	parameters {
		string(name: 'DIFF_ID', defaultValue: '', description: 'ID of the Phabricator Differential.')
		string(name: 'PHID', defaultValue: '', description: 'Phabricator ID')
	}

	stages {
		stage ('Checkout') {
			options {
				retry(3)
			}
			steps {
				script {
					try {
						svn "https://svn.wildfiregames.com/public/ps/trunk"
					} catch(e) {
						bat "svn cleanup"
						sleep 300
						throw e
					}
				}
				bat "svn cleanup"
			}
		}
		stage ('Patch') {
			steps {
				script {
					try {
					 bat "arc patch --diff ${params.DIFF_ID} --force"
					} catch (e) {
						bat 'svn revert -R .'
						bat 'powershell.exe "svn st --no-ignore | %% {$_.substring(8)} | del -r" '
						bat "arc patch --diff ${params.DIFF_ID} --force"
					}
				}
			}
		}
		stage ('Update-Workspace') {
			steps {
				bat "(robocopy /MIR C:\\wxwidgets3.0.4\\lib libraries\\win32\\wxwidgets\\lib) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
				bat "(robocopy /MIR C:\\wxwidgets3.0.4\\include libraries\\win32\\wxwidgets\\include) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
				bat "cd build\\workspaces && update-workspaces.bat --atlas --build-shared-glooxwrapper --jenkins-tests"
			}
		}
		stage ('Debug: Build') {
			steps {
				bat("cd build\\workspaces\\vc2015 && MSBuild.exe pyrogenesis.sln /m:${jobs} /p:PlatformToolset=v140_xp /t:pyrogenesis /t:AtlasUI /t:test /p:Configuration=Debug -clp:Warningsonly -clp:ErrorsOnly ")
			}
		}
		// Debug tests do not work on Windows, see #3753.
		stage ('Release: Build') {
			steps {
				bat("cd build\\workspaces\\vc2015 && MSBuild.exe pyrogenesis.sln /m:${jobs} /p:PlatformToolset=v140_xp /t:pyrogenesis /t:AtlasUI /t:test /p:Configuration=Release -clp:Warningsonly -clp:ErrorsOnly ")
			}
		}
		stage ('Release: Test') {
			options {
				timeout(time: 30)
			}
			steps {
				bat 'binaries\\system\\test.exe > cxxtest_release.xml'
			}
			post {
				failure {
					echo (message: readFile (file: "cxxtest_release.xml"))
				}
				always {
					junit "cxxtest_release.xml"
				}
			}
		}
	}
	post {
		always {
			step([$class: 'PhabricatorNotifier', commentOnSuccess: true, commentWithConsoleLinkOnFailure: true])
			bat 'del cxxtest_*.xml'
			bat 'svn revert -R .'
			bat 'powershell.exe "svn st binaries/data | %% {$_.substring(8)} | del -r " '
			bat 'powershell.exe "svn st source/ | %% {$_.substring(8)} | del -r " '
		}
	}
}

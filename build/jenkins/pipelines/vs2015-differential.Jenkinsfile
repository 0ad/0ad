/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// This pipeline is used to build patches on MSVC 15.0 (Visual Studio 2017).

def jobs = "2"
def visualStudioPath = "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MSBuild.exe\""
def cleanFiles() {
	bat 'powershell.exe "svn st -q | ForEach-Object { Remove-Item -ErrorAction Ignore -Recurse -Force -Verbose -LiteralPath $_.substring(8) } " '
	bat 'powershell.exe "svn st binaries/data | ForEach-Object { Remove-Item -ErrorAction Ignore -Recurse -Verbose -Force -LiteralPath $_.substring(8) } " '
	bat 'powershell.exe "svn st source/ | ForEach-Object { Remove-Item -ErrorAction Ignore -Recurse -Force -Verbose -LiteralPath $_.substring(8) } " '
	bat 'svn revert -R .'
}
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
						cleanFiles()
						bat "arc patch --diff ${params.DIFF_ID} --force"
					}
				}
			}
		}
		stage ('Update-Workspace') {
			steps {
				bat "(robocopy /MIR /NDL /NJH /NJS /NP /NS /NC C:\\wxwidgets3.0.4\\lib libraries\\win32\\wxwidgets\\lib) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
				bat "(robocopy /MIR /NDL /NJH /NJS /NP /NS /NC C:\\wxwidgets3.0.4\\include libraries\\win32\\wxwidgets\\include) ^& IF %ERRORLEVEL% LEQ 1 exit 0"
				bat "cd build\\workspaces && update-workspaces.bat --atlas --build-shared-glooxwrapper --jenkins-tests"
			}
		}
		stage ('Debug: Build') {
			steps {
				dir('build\\workspaces\\vs2017'){
					bat("${visualStudioPath} pyrogenesis.sln /nologo /p:XPDeprecationWarning=false /m:${jobs} /p:PlatformToolset=v141_xp /t:pyrogenesis /t:AtlasUI /t:test /p:Configuration=Debug -clp:Warningsonly -clp:ErrorsOnly > ..\\..\\..\\build-errors-debug.txt 2>&1")
				}
			}
			post {
				always {
					bat "if exist build-errors-debug.txt type build-errors-debug.txt"
				}
			}
		}
		stage ('Debug: Test') {
			options {
				timeout(time: 30)
			}
			steps {
				catchError {
					script {
						try {
							bat 'binaries\\system\\test_dbg.exe > cxxtest_debug.xml'
						} catch(err) {
							echo (message: readFile (file: "cxxtest_debug.xml"))
						}
					}
				}
			}
			post {
				failure {
					echo (message: readFile (file: "cxxtest_debug.xml"))
					archiveArtifacts artifacts: "cxxtest_debug.xml", fingerprint: true
				}
				always {
					junit "cxxtest_debug.xml"
					archiveArtifacts artifacts: "cxxtest_debug.xml", fingerprint: true
				}
			}
		}
		stage ('Release: Build') {
			steps {
				dir('build\\workspaces\\vs2017'){
					bat("${visualStudioPath} pyrogenesis.sln /nologo /p:XPDeprecationWarning=false /m:${jobs} /p:PlatformToolset=v141_xp /t:pyrogenesis /t:AtlasUI /t:test /p:Configuration=Release -clp:Warningsonly -clp:ErrorsOnly > ..\\..\\..\\build-errors-release.txt 2>&1")
				}
			}
			post {
				always {
					bat "if exist build-errors-release.txt type build-errors-release.txt"
				}
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
			script {
					catchError {
						bat 'powershell.exe "if (![String]::IsNullOrWhiteSpace((Get-Content -ErrorAction Ignore -Path build-errors-debug.txt))) { Add-Content .phabricator-comment Debug: }" '
						bat "if exist build-errors-debug.txt type build-errors-debug.txt >> .phabricator-comment"
						bat 'powershell.exe "if (![String]::IsNullOrWhiteSpace((Get-Content -ErrorAction Ignore -Path build-errors-release.txt))) { Add-Content .phabricator-comment Release: }" '
						bat "if exist build-errors-release.txt type build-errors-release.txt >> .phabricator-comment"
					}
					try {
						if (fileExists(".phabricator-comment")) {
							bat "if exist .phabricator-comment type .phabricator-comment"
							step([$class: 'PhabricatorNotifier', commentOnSuccess: true, commentWithConsoleLinkOnFailure: true, customComment: true, commentFile: ".phabricator-comment"])
						}
					} catch(e) {
						throw e
					} finally {
						bat 'del .phabricator-comment'
						bat 'del build-errors-debug.txt'
						bat 'del build-errors-release.txt'
						bat 'del cxxtest_*.xml'
						cleanFiles()
					}
			}
		}
	}
}

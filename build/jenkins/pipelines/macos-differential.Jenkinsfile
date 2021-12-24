/* Copyright (C) 2021 Wildfire Games.
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

pipeline {
	agent { label 'MacSlave' }
	options {
		skipDefaultCheckout()
	}

	parameters {
		string(name: 'DIFF_ID', defaultValue: '', description: 'ID of the Phabricator Differential.')
		string(name: 'PHID', defaultValue: '', description: 'Phabricator ID')
		booleanParam(name: 'CLEAN_WORKSPACE', defaultValue: false, description: 'Delete the workspace before compiling (NB: does not delete the compiled libraries)')
	}

	stages {
		stage ("Checkout") {
			options {
				retry(3)
			}
			steps {
				script {
					try {
						sh "svn update"
					} catch(e) {
						sh "svn cleanup"
						sleep 300
						throw e
					}
				}
				sh "svn cleanup"
			}
		}
		stage ("Patch") {
			steps {
				script {
					try {
						sh "arc patch --diff ${params.DIFF_ID} --force"
					} catch (e) {
						sh "svn st binaries/data/ | cut -c 9- | xargs rm -rfv"
						sh "svn st source/ | cut -c 9- | xargs rm -rfv"
						sh "svn st -q | cut -c 9- | xargs rm -rfv"
						sh "svn revert -R ."
						sh "arc patch --diff ${params.DIFF_ID} --force"
					}
				}
			}
		}
		stage("macOS libraries build") {
			steps {
				sh "cd libraries/osx/ && ./build-osx-libs.sh -j4"
			}
		}
		stage("Update workspaces") {
			steps {
				script {
					if (params.CLEAN_WORKSPACE) {
						sh "rm -rf build/workspaces/gcc"
					}
					sh "cd build/workspaces/ && ./update-workspaces.sh -j4 --jenkins-tests"
				}

			}
		}
		stage("Debug Build & Tests") {
			steps {
				sh "cd build/workspaces/gcc/ && make -j4 config=debug 2> ../../../builderr-debug-macos.txt"
				script {
					try {
						sh "binaries/system/test_dbg > cxxtest-debug.xml"
					} catch (e) {
						echo (message: readFile (file: "cxxtest-debug.xml"))
						throw e
					} finally {
						junit "cxxtest-debug.xml"
					}
				}
			}
		}
		stage("Release Build & Tests") {
			steps {
				sh "cd build/workspaces/gcc/ && make -j4 config=release 2> ../../../builderr-release-macos.txt"
				script {
					try {
						sh "binaries/system/test > cxxtest-release.xml"
					} catch (e) {
						echo (message: readFile (file: "cxxtest-release.xml"))
						throw e
					} finally {
						junit "cxxtest-release.xml"
					}
				}
			}
		}
	}

	post {
		always {
			script {
				catchError {
					sh '''
					for file in builderr-*.txt ; do
						if [ -s "$file" ]; then
							echo "$file" >> .phabricator-comment
							cat "$file" >> .phabricator-comment
						fi
					done
					'''
				}

				try {
					if (fileExists(".phabricator-comment")) {
						step([$class: 'PhabricatorNotifier', commentOnSuccess: true, commentWithConsoleLinkOnFailure: true, customComment: true, commentFile: ".phabricator-comment"])
					} else {
						step([$class: 'PhabricatorNotifier', commentWithConsoleLinkOnFailure: true])
					}
				} catch(e) {
					throw e
				} finally {
					sh "rm -f .phabricator-comment builderr-*.txt cxxtest-*.xml"
					sh "svn st binaries/data/ | cut -c 9- | xargs rm -rfv"
					sh "svn st source/ | cut -c 9- | xargs rm -rfv"
					sh "svn st -q | cut -c 9- | xargs rm -rfv"
					sh "svn revert -R ."
				}
			}
		}
	}
}

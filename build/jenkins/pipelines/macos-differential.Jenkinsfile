/* Copyright (C) 2020 Wildfire Games.
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
	}

	stages {
		stage ("Checkout") {
			options {
				retry(3)
			}
			steps {
				script {
					try {
						svn "https://svn.wildfiregames.com/public/ps/trunk"
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
						sh "svn revert -R ."
						sh "svn st | cut -c 9- | xargs rm -rf"
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
				sh "cd build/workspaces/ && ./update-workspaces.sh -j4 --atlas --jenkins-tests"
			}
		}
		stage("Debug Build & Tests") {
			steps {
				sh "cd build/workspaces/gcc/ && make -j4 config=debug"
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
				sh "cd build/workspaces/gcc/ && make -j4 config=release"
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
			step([$class: 'PhabricatorNotifier', commentOnSuccess: true, commentWithConsoleLinkOnFailure: true])
			sh "rm -f cxxtest_*.xml"
			sh "svn revert -R ."
			sh "svn st binaries/data/ | cut -c 9- | xargs rm -rf"
			sh "svn st source/ | cut -c 9- | xargs rm -rf"
		}
	}
}

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

	parameters {
		string(name: 'SVN_REV', defaultValue: '', description: 'For instance 21000')
		string(name: 'PHID', defaultValue: '', description: 'Phabricator ID')
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
						svn "https://svn.wildfiregames.com/public/ps/trunk@${params.SVN_REV}"
					} catch(e) {
						sh "svn cleanup"
						sleep 300
						throw e
					}
				}
				sh "svn cleanup"
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
			step([$class: 'PhabricatorNotifier'])
		}
	}
}

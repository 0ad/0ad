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

// This is the post-commit pipeline.
// In case of success, it provides a clean base for incremental builds
// with the `differential` pipeline.

def compilers = ["gcc6"]

def volumeUpdatesMap = compilers.collectEntries {
	["${it}": volumeUpdate(it)]
}
def volumeUpdate(compiler) {
	return {
		stage("Update: ${compiler}") {
			ws("/zpool0/${compiler}") {
				sh "svn up -r ${params.SVN_REV}"
			}
		}
	}
}

def buildsMap = compilers.collectEntries {
	["${it}": build(it)]
}
def build(compiler) {
	return {
		stage("Build: ${compiler}") {
			try {
				ws("/zpool0/${compiler}") {
					docker.image("0ad-${compiler}:latest").inside {
						sh "build/workspaces/update-workspaces.sh -j1 --jenkins-tests"

						sh "cd build/workspaces/gcc/ && make -j1 config=debug"
						try {
							sh "binaries/system/test_dbg > cxxtest-debug-${compiler}.xml"
						} catch (e) {
							echo (message: readFile (file: "cxxtest-debug-${compiler}.xml"))
							throw e
						} finally {
							stash includes: "cxxtest-debug-${compiler}.xml", name: "tests-debug-${compiler}"
						}

						sh "cd build/workspaces/gcc/ && make -j1 config=release"
						try {
							sh "binaries/system/test > cxxtest-release-${compiler}.xml"
						} catch (e) {
							echo (message: readFile (file: "cxxtest-release-${compiler}.xml"))
							throw e
						} finally {
							stash includes: "cxxtest-release-${compiler}.xml", name: "tests-release-${compiler}"
						}
					}
				}

				sh "sudo zfs destroy zpool0/${compiler}@latest"
				sh "sudo zfs snapshot zpool0/${compiler}@latest"
			} catch (e) {
				sh "sudo zfs rollback zpool0/${compiler}@latest"
				throw e
			}
		}
	}
}

pipeline {
	agent {
		node {
			label 'LinuxSlave'
			customWorkspace '/zpool0/trunk'
		}
	}
	parameters {
		string(name: 'SVN_REV', defaultValue: '', description: 'For instance 21000')
		string(name: 'PHID', defaultValue: '', description: 'Phabricator ID')
	}

	stages {
		stage("Update") {
			steps {
				sh "svn cleanup 2>/dev/null || true"
				svn "https://svn.wildfiregames.com/public/ps/trunk@${params.SVN_REV}"
				sh "svn st --no-ignore | cut -c 9- | xargs rm -rf"
				sh "svn revert -R ."
			}
		}
		stage("Volume updates") {
			steps {
				script { parallel volumeUpdatesMap }
			}
		}
		stage("Build") {
			steps {
				script { parallel buildsMap }
			}
			post {
				always {
					script {
						for(compiler in compilers) {
							catchError { unstash "tests-debug-${compiler}" }
							catchError { unstash "tests-release-${compiler}" }
						}
					}
					catchError { junit 'cxxtest*.xml' }
				}
				failure {
					sh "sudo zfs rollback zpool0/trunk@latest"
				}
				success {
					sh "sudo zfs destroy -R zpool0/trunk@latest"
					sh "sudo zfs snapshot zpool0/trunk@latest"
				}
			}
		}
		stage("Data checks") {
			steps {
				sh "cd source/tools/entity/ && perl checkrefs.pl --check-map-xml --validate-templates"
			}
		}
	}

	post {
		always {
			step([$class: 'PhabricatorNotifier'])
		}
	}
}

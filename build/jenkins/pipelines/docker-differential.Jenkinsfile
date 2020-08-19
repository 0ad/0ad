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

// This pipeline is used to build patches on various compilers.

def compilers = ["gcc6"]

def patchesMap = compilers.collectEntries {
	["${it}": patch(it)]
}
def patch(compiler) {
	return {
		stage("Patch: ${compiler}") {
			try {
				ws("/zpool0/${compiler}") {
					sh "arc patch --diff ${params.DIFF_ID} --force"
				}
			} catch(e) {
				sh "sudo zfs rollback zpool0/${compiler}@latest"
				throw e
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

						try {
							retry(3) {
								sh "cd build/workspaces/gcc/ && make -j1 config=debug 2> ../../../builderr-debug-${compiler}.txt"
							}
						} catch(e) {
							throw e
						} finally {
							stash includes: "builderr-debug-${compiler}.txt", name: "build-debug-${compiler}"
						}

						try {
							sh "binaries/system/test_dbg > cxxtest-debug-${compiler}.xml"
						} catch (e) {
							echo (message: readFile (file: "cxxtest-debug-${compiler}.xml"))
							throw e
						} finally {
							stash includes: "cxxtest-debug-${compiler}.xml", name: "tests-debug-${compiler}"
						}

						try {
							retry(3) {
								sh "cd build/workspaces/gcc/ && make -j1 config=release 2> ../../../builderr-release-${compiler}.txt"
							}
						} catch(e) {
							throw e
						} finally {
							stash includes: "builderr-release-${compiler}.txt", name: "build-release-${compiler}"
						}

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
			} catch (e) {
				throw e
			} finally {
				sh "sudo zfs rollback zpool0/${compiler}@latest"
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
		string(name: 'DIFF_ID', defaultValue: '', description: 'ID of the Phabricator Differential.')
		string(name: 'PHID', defaultValue: '', description: 'Phabricator ID')
	}

	stages {
		stage("Patch") {
			steps {
				sh "arc patch --diff ${params.DIFF_ID} --force"
				script { parallel patchesMap }
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
							catchError { unstash "build-debug-${compiler}" }
							catchError { unstash "tests-debug-${compiler}" }
							catchError { unstash "build-release-${compiler}" }
							catchError { unstash "tests-release-${compiler}" }
						}
					}
					catchError {
						sh '''
						for file in builderr-*.txt ; do
						  if [ -s "$file" ]; then
						    echo "$file" >> build-errors.txt
						    cat "$file" >> build-errors.txt
						  fi
						done
						'''
					}
					catchError { junit 'cxxtest*.xml' }
				}
			}
		}
		stage("Lint") {
			steps {
				script {
					try {
						withDockerContainer("0ad-coala:latest") {
							sh '''
							svn st | grep '^[AM]' | cut -c 9- | xargs coala -d build/coala --ci --disable-caching \
							  --format '{{ "name": "{origin}", "code": "{origin}", "severity": "{severity_str}", "path": "{file}", "line": {line}, "description": "`{message}`" }}' \
							  --limit-files > coala-report
							'''
						}
					} catch (e) {
						sh 'sed -i "s|$(pwd)/||g" coala-report'
						sh 'sed -e "s/INFO/advice/g" -e "s/NORMAL/warning/g" -e "s/MAJOR/error/g" coala-report > .phabricator-lint'
					}
				}
			}
		}
		stage("Data checks") {
			steps {
				sh "cd source/tools/entity/ && perl checkrefs.pl --check-map-xml --validate-templates 2> data-errors.txt"
			}
		}
	}

	post {
		always {
			script {
				catchError {
					sh "if [ -s build-errors.txt ]; then cat build-errors.txt >> .phabricator-comment ; fi"
					sh '''
					if [ -s data-errors.txt ]; then
					  echo "Data checks errors:" >> .phabricator-comment
					  cat data-errors.txt >> .phabricator-comment
					fi
					'''
				}

				try {
					if (fileExists(".phabricator-comment")) {
						step([$class: 'PhabricatorNotifier', commentOnSuccess: true, commentWithConsoleLinkOnFailure: true, customComment: true, commentFile: ".phabricator-comment", processLint: true, lintFile: ".phabricator-lint"])
					} else {
						step([$class: 'PhabricatorNotifier', commentWithConsoleLinkOnFailure: true, processLint: true, lintFile: ".phabricator-lint"])
					}
				} catch(e) {
					throw e
				} finally {
					sh "sudo zfs rollback zpool0/trunk@latest"
				}
			}
		}
	}
}

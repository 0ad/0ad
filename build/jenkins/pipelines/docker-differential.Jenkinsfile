/* Copyright (C) 2024 Wildfire Games.
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

def compilers = ["gcc7", "clang8"]

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
				reset("${compiler}").call()
				throw e
			}
		}
	}
}

def resetMap = compilers.collectEntries {
	["${it}": reset(it)]
}

def reset(compiler)
{
	return {
		stage("Reset: ${compiler}") {
			sleep 30
			sh "sudo zfs rollback zpool0/${compiler}@latest"
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
								try {
									sh "cd build/workspaces/gcc/ && make -j1 config=debug 2> ../../../builderr-debug-${compiler}.txt"
								} catch(e) {
									sh "rm -rf build/workspaces/gcc/obj/test_Debug"
									throw e
								}
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
								try {
									sh "cd build/workspaces/gcc/ && make -j1 config=release 2> ../../../builderr-release-${compiler}.txt"
								} catch(e) {
									sh "rm -rf build/workspaces/gcc/obj/test_Release"
									throw e
								}
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
				reset("${compiler}").call()
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
			when { expression { return !!params.DIFF_ID } }
			steps {
				script {
					try {
						sh "arc patch --diff ${params.DIFF_ID} --force"
						script { parallel patchesMap }
					} catch(e) {
						// In case of failure, reset both, since they were patched together.
						parallel resetMap
						throw e
					}
				}
			}
		}
		stage("Build") {
			steps {
				script {
					try {
						buildsMap.each { key, value ->
							value.call()
						}
					} catch(e) {
						// In case of failure, reset both, since they were patched together.
						parallel resetMap
						throw e
					}
				}
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
						docker.image("0ad-lint:latest").inside {
							try {
								// arc lint outputs an empty file on success - unless there is nothing to lint.
								// On failure, it'll output the file and a failure error code.
								// Explicitly checking for the file presence is thus best to detect the linter did run
								sh '~/arcanist/bin/arc lint --never-apply-patches --output jenkins --outfile .phabricator-lint && touch .phabricator-lint'
							}
							catch (e) {
								if (!fileExists(".phabricator-lint")) {
									sh '''echo '{"General":[{"line": 0, "char": 0, "code": "Jenkins", "severity": "error", "name": "ci-error", "description": "Error running lint", "original": null, "replacement": null, "granularity": 1, "locations": [], "bypassChangedLineFiltering": true, "context": null}]}' > .phabricator-lint '''
								}
								else {
									sh 'echo "error(s) were found running lint"'
								}
							}
							finally {
								stash includes: ".phabricator-lint", name: "Lint File"
							}
						} 
					}
					finally {
							unstash("Lint File")
							if (fileExists(".phabricator-lint")) {
								sh '''cat .phabricator-lint '''
							}
					}
				}
			}
		}
		stage("Data checks") {
			steps {
				warnError('CheckRefs.py script failed!') {
					sh "cd source/tools/entity/ && python3 checkrefs.py -tax 2> data-errors.txt"
				}
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

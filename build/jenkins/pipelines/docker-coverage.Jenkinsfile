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

// This pipeline is used to build the documentation.

pipeline {
	agent {
		node {
			label 'LinuxSlave'
		}
	}
	environment {
		INFO_FILE = 'coverage.info'
		REPORT_PATH	= '../../../coverage-results/'
	}
	stages {
		stage("Setup") {
			steps {
				sh "sudo zfs clone zpool0/gcc7@latest zpool0/docker-coverage"
			}
		}
		stage("Build") {
			steps {
				ws("/zpool0/docker-coverage"){
				  dir('build/workspaces/'){
					sh "./update-workspaces.sh -j1 --jenkins-tests --coverage"
					dir('gcc/'){
						// Reset everything in case there were ever leftovers
						sh "lcov --directory . --zerocounters"
						sh "make -B -j1 test"
					}
				  }
				}
			}
		}
		stage("Generation") {
			steps {
				ws("/zpool0/docker-coverage"){
				  dir('build/workspaces/gcc'){
					// The executable must be launched from the same path
					// as where it was built else nothing is generated.
					sh "../../../binaries/system/test"
					// Capture the symbols.
					sh "lcov --directory . --capture --output-file ${INFO_FILE}"
					// Remove things we don't need to analyze like external
					// libraries or system ones.
					sh "lcov --remove ${INFO_FILE} \"*/*/*/tests/*\" \"*/*/*/libraries/*\" \"/usr/*\"  --output-file ${INFO_FILE}"
					sh "mkdir -p ${REPORT_PATH}"
					sh "genhtml --o ${REPORT_PATH} -t \"0 A.D. test coverage report\" --num-spaces 4 --demangle-cpp ${INFO_FILE}"
				  }
				}
			}
		}
		stage("Upload") {
			steps {
				ws("/zpool0/docker-coverage"){
					sh "rsync -airt --progress coverage-results/* docs.wildfiregames.com:~/www/coverage/"
				}
			}
		}
	}
	post {
		always {
			ws("/zpool0/trunk") {
				sleep 10
				sh "sudo zfs destroy zpool0/docker-coverage"
			}
		}
	}
}

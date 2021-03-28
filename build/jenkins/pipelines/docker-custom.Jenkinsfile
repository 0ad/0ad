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
	agent {
		node {
			label 'LinuxSlave'
			customWorkspace '/zpool0/trunk'
		}
	}
	parameters {
		string(name: 'DIFF_ID', defaultValue: '', description: '(optional) ID of the Phabricator Differential.')
		booleanParam(name: 'NO_PCH', defaultValue: false, description: 'Run this build without PCH.')
		booleanParam(name: 'DEBUG', defaultValue: false, description: 'Compile in debug mode.')
	}

	stages {
		stage("Setup") {
			steps {
				sh "sudo zfs clone zpool0/gcc7@latest zpool0/custom"
			}
		}
		stage("Apply Differential") {
			when {
				expression { return params.DIFF_ID != "" }
			}
			steps {
				ws("/zpool0/custom") {
					sh "arc patch --diff ${params.DIFF_ID} --force"
				}
			}
		}
		stage("PCH clean up") {
			when {
				expression { return params.NO_PCH }
			}
			steps {
				ws("/zpool0/custom") {
					sh "rm -rf build/workspaces/gcc/"
				}
			}
		}
		stage("Build") {
			steps {
				script {
					ws("/zpool0/custom") {
						// Destroy test *.cpp files as they use (invalid) absolute paths.
						sh 'python3 -c \"import glob; print(\\\" \\\".join(glob.glob(\\\"source/**/tests/**.cpp\\\", recursive=True)));\" | xargs rm -v'
						// Hack: ignore NVTT
						sh "echo '' > libraries/source/nvtt/build.sh"
						docker.image("0ad-gcc7:latest").inside {
							stage("Update Workspaces") {
								if (params.NO_PCH) {
									sh "build/workspaces/update-workspaces.sh -j1 --without-pch"
								} else {
									sh "build/workspaces/update-workspaces.sh -j1"
								}
							}
							stage("Build") {
								if (params.DEBUG) {
									sh "cd build/workspaces/gcc/ && make -j1 config=debug"
								}
								sh "cd build/workspaces/gcc/ && make -j1 config=release"
							}
							stage("Run tests") {
								if (params.DEBUG) {
									sh "binaries/system/test_dbg"
								}
								sh "binaries/system/test"
							}
						}
					}
				}
			}
		}
	}
	post {
		always {
			sh "sudo zfs destroy zpool0/custom"
		}
	}
}

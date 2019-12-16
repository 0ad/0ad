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

// This pipeline is used to build a clean base from scratch in order to
// use ZFS snapshots efficiently.

def compilers = ["gcc6"]

def volumeUpdatesMap = compilers.collectEntries {
	["${it}": volumeUpdate(it)]
}
def volumeUpdate(compiler) {
	return {
		stage("Recreate: ${compiler}") {
			sh "sudo zfs clone zpool0/trunk@base zpool0/${compiler}"
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
						sh "binaries/system/test_dbg"

						sh "cd build/workspaces/gcc/ && make -j1 config=release"
						sh "binaries/system/test"
					}
				}
			} catch (e) {
				throw e
			} finally {
				sh "sudo zfs snapshot zpool0/${compiler}@latest"
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

	stages {
		stage("Volume updates") {
			steps {
				// Note: latest must always be the last snapshot in order to be able to rollback to it
				sh "sudo zfs rollback zpool0/trunk@latest"
				sh "sudo zfs destroy -R zpool0/trunk@latest"
				sh "sudo zfs destroy -R zpool0/trunk@base"
				sh "sudo zfs snapshot zpool0/trunk@base"
				sh "sudo zfs snapshot zpool0/trunk@latest"
				script { parallel volumeUpdatesMap }
			}
		}
		stage("Build") {
			steps {
				script { parallel buildsMap }
			}
		}
	}
}

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

// This is a helper pipeline to build Docker images and setup ZFS volumes.

pipeline {
	agent {
		node {
			label 'LinuxSlave'
			customWorkspace '/zpool0/trunk'
		}
	}
	parameters {
		booleanParam(name: 'no-cache', defaultValue: false, description: 'Rebuild containers from scratch')
		booleanParam(name: 'reset-volumes', defaultValue: false, description: 'Reset ZFS volumes')
	}

	stages {
		stage("Cleanup") {
			steps {
				sh 'docker system prune -f'
			}
		}
		stage("Full Rebuild") {
			when {
				environment name: 'no-cache', value: 'true'
			}
			steps {
				sh 'docker build --no-cache -t 0ad-gcc6 ~/dockerfiles/gcc6'
				// WIP: clang and recent gccs
				sh 'docker build --no-cache -t 0ad-coala ~/dockerfiles/coala'
				sh 'docker build --no-cache -t 0ad-translations ~/dockerfiles/translations'
			}
		}
		stage("Build") {
			steps {
				sh 'docker build -t 0ad-gcc6 ~/dockerfiles/gcc6'
				// WIP: clang and recent gccs
				sh 'docker build -t 0ad-coala ~/dockerfiles/coala'
				sh 'docker build -t 0ad-translations ~/dockerfiles/translations'
			}
		}
		stage("Update") {
			steps {
				sh "svn cleanup 2>/dev/null || true"
				svn "https://svn.wildfiregames.com/public/ps/trunk"
				sh "svn st --no-ignore | cut -c 9- | xargs rm -rf"
				sh "svn revert -R ."
			}
		}
		stage("Volumes") {
			when {
				environment name: 'reset-volumes', value: 'true'
			}
			steps {
				sh "sudo zfs destroy -R zpool0/trunk@base || true"
				sh "sudo zfs destroy -R zpool0/trunk@latest || true"

				sh "sudo zfs snapshot zpool0/trunk@base"
				sh "sudo zfs clone zpool0/trunk@base zpool0/gcc6"
				sh "sudo zfs clone zpool0/trunk@base zpool0/gcc7"

				sh "sudo zfs snapshot zpool0/trunk@latest"
			}
		}
	}
}

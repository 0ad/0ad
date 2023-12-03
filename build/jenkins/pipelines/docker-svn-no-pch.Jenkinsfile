/* Copyright (C) 2022 Wildfire Games.
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
	stages {
		stage("Setup") {
			steps {
				sh "sudo zfs clone zpool0/gcc7@latest zpool0/docker-nopch"
			}
		}
		stage("Build") {
			steps {
				ws("/zpool0/docker-nopch"){
				  dir('build/workspaces/'){
					sh "./update-workspaces.sh -j1 --without-pch"
					dir('gcc/'){
						sh "make clean"
						sh "make -j1"
					}
				  }
				}
			}
		}
		stage("Test") {
			steps {
				ws("/zpool0/docker-nopch"){
				  dir('build/workspaces/gcc'){
					// as where it was built else nothing is generated.
					sh "../../../binaries/system/test"
				  }
				}
			}
		}
	}
	post {
		always {
			ws("/zpool0/trunk") {
				sleep 10
				sh "sudo zfs destroy zpool0/docker-nopch"
			}
		}
	}
}

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

// This pipeline is used to update translations to and from Transifex.

pipeline {
	agent {
		node {
			label 'LinuxSlave'
			customWorkspace '/zpool0/trunk'
		}
	}

	stages {
		stage("Prepare volume") {
			steps {
				sh "sudo zfs clone zpool0/trunk@latest zpool0/translations"
			}
		}

		stage("Update translations") {
			steps {
				ws('/zpool0/translations') {
					withDockerContainer("0ad-translations:latest") {
						sh "sh source/tools/i18n/maintenanceTasks.sh"
					}
				}
			}
		}

		stage("Commit") {
			steps {
				ws('/zpool0/translations') {
					withCredentials([usernamePassword(credentialsId: 'redacted', passwordVariable: 'SVNPASS', usernameVariable: 'SVNUSER')]) {
						sh "svn relocate --username ${SVNUSER} --password ${SVNPASS} --no-auth-cache https://svn.wildfiregames.com/svn/ps/trunk"
						sh "svn add --force binaries/"
						sh "svn commit --username ${SVNUSER} --password ${SVNPASS} --no-auth-cache --non-interactive -m '[i18n] Updated POT and PO files.'"
					}
				}
			}
		}
	}

	post {
		always {
			sleep 10
			sh "sudo zfs destroy zpool0/translations"
		}
	}
}

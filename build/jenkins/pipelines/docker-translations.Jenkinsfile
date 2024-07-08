/* Copyright (C) 2023 Wildfire Games.
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
				ws('/zpool0/translations') {
					sh "svn revert . -R"
					sh "svn st | cut -c 9- | xargs rm -rf"
					sh "svn up"
				}
			}
		}

		stage("Update translations") {
			steps {
				ws('/zpool0/translations') {
					withDockerContainer("0ad-translations:latest") {
						sh "python3 source/tools/i18n/updateTemplates.py"
						withCredentials([string(credentialsId: 'redacted', variable: 'token')]) {
							sh 'TX_TOKEN="$token" python3 source/tools/i18n/pullTranslations.py'
						}
						dir("source/tools/i18n/") {
							sh "python3 generateDebugTranslation.py --long"
						}
						sh "python3 source/tools/i18n/cleanTranslationFiles.py"
						sh "python3 source/tools/i18n/checkDiff.py --verbose"
						sh "python3 source/tools/i18n/creditTranslators.py"
					}
				}
			}
		}

		stage("Commit") {
			steps {
				ws('/zpool0/translations') {
					withCredentials([usernamePassword(credentialsId: 'redacted', passwordVariable: 'SVNPASS', usernameVariable: 'SVNUSER')]) {
						sh 'svn relocate --username $SVNUSER --password $SVNPASS --no-auth-cache https://svn.wildfiregames.com/svn/ps/trunk'
						sh 'svn commit --username $SVNUSER --password $SVNPASS --no-auth-cache --non-interactive -m "[i18n] Updated POT and PO files."'
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

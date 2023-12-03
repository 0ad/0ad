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

// This pipeline is used to build the design document.

pipeline {
	agent {
		node {
			label 'LinuxSlave'
		}
	}
	stages {
		stage("Checkout") {
			steps {
				ws("/zpool0/design-docs"){
					git "https://code.wildfiregames.com/source/design.git"
				}
			}
		}
		stage("Generate") {
			steps {
				ws("/zpool0/design-docs"){
					sh "mkdocs build"
				}
			}
		}
		stage("Upload") {
			steps {
				ws("/zpool0/design-docs"){
					sh "rsync -rti --delete-after --progress site/ docs.wildfiregames.com:~/www/design/"
				}
			}
		}
	}
	post {
		always {
			step([$class: 'PhabricatorNotifier'])
		}
	}
}

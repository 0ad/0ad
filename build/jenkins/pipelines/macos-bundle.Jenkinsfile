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

pipeline {
	agent { label 'MacSlave' }

	// Archive the installer for public download; keep only the latest one.
	options {
		buildDiscarder(logRotator(artifactNumToKeepStr: '1'))
	}

	parameters {
		booleanParam(name: 'release', defaultValue: false)
	}

	stages {
		stage("Checkout") {
			steps {
				svn "https://svn.wildfiregames.com/public/ps/trunk"
				sh "svn cleanup"
				sh "svn st --no-ignore | cut -c 9- | xargs rm -rf"
			}
		}
		stage("Bundle") {
			steps {
				sh "cd source/tools/dist && ./build-osx-bundle.sh"
			}
		}
	}

	post {
		success {
			archiveArtifacts 'build/workspaces/*.dmg'
		}
	}
}

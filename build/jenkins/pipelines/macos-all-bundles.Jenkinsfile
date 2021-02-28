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
		string(name: 'BUNDLE_VERSION', defaultValue: '0.0.24dev', description: 'Bundle Version')
		string(name: 'SVN_REV', defaultValue: 'HEAD', description: 'For instance 21000')
		booleanParam(name: 'ONLY_MOD', defaultValue: true, description: 'Only archive the mod mod.')
	}

	stages {
		stage ("Checkout") {
			options {
				retry(3)
			}
			steps {
				script {
					try {
						svn "https://svn.wildfiregames.com/public/ps/trunk@${params.SVN_REV}"
					} catch(e) {
						sh "svn cleanup"
						sleep 300
						throw e
					}
				}
				sh "svn cleanup"
				sh "svn revert . -R"
				sh "svn st --no-ignore | cut -c 9- | xargs rm -rfv"
			}
		}
		stage("Compile Mac Executable") {
			steps {
				sh "source/tools/dist/build-osx-executable.sh"
			}
		}
		stage("Create archive data") {
			steps {
				sh "ONLY_MOD=${ONLY_MOD} source/tools/dist/build-archives.sh"
			}
		}
		stage("Create Mac Bundle") {
			steps {
				sh "python3 source/tools/dist/build-osx-bundle.py ${BUNDLE_VERSION}"
			}
		}
		stage("Create Windows installer & *nix files") {
			steps {
				// The files created by the mac compilation need to be deleted
				sh "svn st binaries/ --no-ignore | cut -c 9- | xargs rm -rf"
				sh "svn st build/ --no-ignore | cut -c 9- | xargs rm -rf"
				sh "svn st libraries/ --no-ignore | cut -c 9- | xargs rm -rf"
				// The generated tests use hardcoded paths so they must be deleted as well.
				sh 'python3 -c \"import glob; print(\\\" \\\".join(glob.glob(\\\"source/**/tests/**.cpp\\\", recursive=True)));\" | xargs rm -v'
				sh "svn revert build/ -R"

				// Then run the core object.
				sh "BUNDLE_VERSION=${BUNDLE_VERSION} source/tools/dist/build-unix-win32.sh"
			}
		}
	}

	post {
		success {
			archiveArtifacts '*.dmg,*.exe,*.tar.gz,*.tar.xz'
		}
	}
}

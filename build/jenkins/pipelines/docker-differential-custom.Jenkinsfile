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

// This pipeline is used to build patches on various compilers.

def compilers = []
if (params["With Clang"])
{
	compilers = ["clang7"]
}
else
{
	compilers = ["gcc7"]
}

def patchesMap = compilers.collectEntries {
	["${it}": patch(it)]
}
def patch(compiler) {
	return {
		stage("Patch: ${compiler}") {
			try {
				ws("/zpool0/${compiler}") {
					sh "arc patch ${params.DIFF_ID} --force"
				}
			} catch(e) {
				sh "sudo zfs rollback zpool0/${compiler}@latest"
				throw e
			}
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
						script {
							def NoAudioOption = ""
							def NoPchOption = ""
							def GlesOption = ""
							def NoLobbyOption = ""

							if (params["No PCH"])
							{
								NoPchOption="--without-pch "
							}
							if (params["OpenGL ES"])
							{
								GlesOption="--gles "
							}
							if (params["No Audio"])
							{
								NoAudioOption="--without-audio "
							}
							if (params["No Lobby"])
							{
								NoLobbyOption="--without-lobby "
							}

							sh "build/workspaces/update-workspaces.sh ${NoAudioOption}${GlesOption}${NoPchOption}${NoLobbyOption} -j1"
						}
						test_env="this is test env"
						withEnv(["CXXFLAGS=${params.CXXFLAGS}", "CFLAGS=${params.CFLAGS}", "LDFLAGS=${params.LDFLAGS}"]){
							if (params["CXXFLAGS"])
							{
							   echo "Custom CXXFLAGS: ${env.CXXFLAGS}"							 
							}
							if (params["CFLAGS"])
							{
							   echo "Custom CFLAGS: ${env.CFLAGS}"							 
							}
							if (params["Debug Build"])
							{
								def debugBuildErrorFile = "builderr-debug-${compiler}.log"
								try {
									try {
										sh '''
											CC_VERSION="$($CC --version | sed -e \'s/version//g\' -e \'s/(.*)//g' -e \'s/\\s//\' | sed 1q)"
											CXX_VERSION="$($CXX --version | sed -e \'s/version//g\' -e \'s/(.*)//g' -e \'s/\\s//\' | sed 1q)"
											echo "Building pyrogenesis in debug with $CC_VERSION/$CXX_VERSION using 2 jobs..."
										'''
										sh "cd build/workspaces/gcc/ && make config=debug clean && make -j2 config=debug 2> ../../../${debugBuildErrorFile}"
									} catch(e) {
										sh "rm -rf build/workspaces/gcc/obj/test_Debug"
										throw e
									}
								} catch(e) {
									throw e
								} finally {
									archiveArtifacts artifacts: "${debugBuildErrorFile}", fingerprint: true
								}
		
								def debugBuildTestFile = "cxxtest-debug-${compiler}.log"
								try {
									sh "binaries/system/test_dbg > ${debugBuildTestFile}"
								} catch (e) {
									echo (message: readFile (file: "${debugBuildTestFile}"))
									throw e
								} finally {
									archiveArtifacts artifacts: "${debugBuildTestFile}", fingerprint: true
								}
							}
							def releaseBuildErrorFile = "builderr-release-${compiler}.log"
							try {
								try {
									sh '''
										CC_VERSION="$($CC --version | sed -e \'s/version//g\' -e \'s/(.*)//g' -e \'s/\\s//\' | sed 1q)"
										CXX_VERSION="$($CXX --version | sed -e \'s/version//g\' -e \'s/(.*)//g' -e \'s/\\s//\' | sed 1q)"
										echo "Building pyrogenesis in release with $CC_VERSION/$CXX_VERSION using 2 jobs..."
									'''
									sh "cd build/workspaces/gcc/ && make clean && make -j2 config=release 2> ../../../${releaseBuildErrorFile}"
								} catch(e) {
									sh "rm -rf build/workspaces/gcc/obj/test_Release"
									throw e
								}
							} catch(e) {
								throw e
							} finally {
								archiveArtifacts artifacts: "${releaseBuildErrorFile}", fingerprint: true
							}
	
							def releaseBuildTestFile = "cxxtest-release-${compiler}.log"
							try {
								sh "binaries/system/test > ${releaseBuildTestFile}"
							} catch (e) {
								echo (message: readFile (file: "${releaseBuildTestFile}"))
								throw e
							} finally {
								archiveArtifacts artifacts: "${releaseBuildTestFile}", fingerprint: true
							}
						}
					}
				}
			} catch (e) {
				throw e
			} finally {
				sh "sudo zfs rollback zpool0/${compiler}@latest"
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
	parameters {
		string(name: 'DIFF_ID', defaultValue: '', description: 'ID of the Phabricator Differential.')
		booleanParam(name: 'OpenGL ES', defaultValue: false, description: 'Build with --gles.')
		booleanParam(name: 'No PCH', defaultValue: false, description: 'Build with --without-pch.')
		booleanParam(name: 'No Audio', defaultValue: false, description: 'Build with --without-audio.')
		booleanParam(name: 'No Lobby', defaultValue: false, description: 'Build with --without-lobby.')
	}

	stages {
		stage("Patch") {
			when { expression { return !!params.DIFF_ID } }
			steps {
				script { parallel patchesMap }
			}
		}
		stage("Build") {
			options {
				timeout(time: 2, unit: 'HOURS')
			}
			steps {
				script { parallel buildsMap }
			}
		}
	}
}

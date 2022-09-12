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
				sh "sudo zfs clone zpool0/gcc7@latest zpool0/entity-docs"
			}
		}
		stage("Engine docs") {
			steps {
				ws("/zpool0/entity-docs"){
					sh "cd docs/doxygen/ && doxygen config"
				}
			}
		}
		stage("Build") {
			steps {
				ws("/zpool0/entity-docs"){
					sh "build/workspaces/update-workspaces.sh --disable-atlas --without-tests -j1"
					sh "cd build/workspaces/gcc/ && make pyrogenesis -j1"
				}
			}
		}
		stage("Entity docs") {
			steps {
				ws("/zpool0/entity-docs"){
					sh "cd binaries/system/ && ./pyrogenesis -mod=public -dumpSchema"
					sh "cd source/tools/entdocs/ && ./build.sh"
				}
			}
		}
		stage("Template Analyzer") {
			steps {
				ws("/zpool0/entity-docs"){
					sh "cd source/tools/templatesanalyzer/ && python3 unitTables.py"
				}
			}
		}
		stage("Upload") {
			steps {
				ws("/zpool0/entity-docs"){
					sh "rsync -rti --delete-after --progress docs/doxygen/html/ docs.wildfiregames.com:~/www/pyrogenesis/"
					sh "rsync -ti --progress source/tools/entdocs/entity-docs.html docs.wildfiregames.com:~/www/entity-docs/trunk.html"
					sh "rsync -ti --progress source/tools/templatesanalyzer/unit_summary_table.html docs.wildfiregames.com:~/www/templatesanalyzer/index.html"
				}
			}
		}
	}
	post {
		always {
			ws("/zpool0/trunk") {
				sleep 10
				sh "sudo zfs destroy zpool0/entity-docs"
			}
		}
	}
}

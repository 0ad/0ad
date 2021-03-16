#!/usr/bin/env php
<?php
/**
 * Copyright (C) 2021 Wildfire Games.
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


/**
 * This file replaces eslint if the former is not found, to avoid failure in 'arc lint'.
 * It is written in PHP as we can assume php is installed if arcanist is to work at all.
 * It mimics `eslint --format json`.
 * Set the VERBOSE env variable to generate an 'advice' level lint message.
 */

$verbose = getenv("VERBOSE") ? getenv("VERBOSE") : false;

$advice = <<<EOD
{
	"filePath": "build/arclint/dummies/eslint.php",
	"messages": [{
		"ruleId": "skipped",
		"severity": 0,
		"message": "ESLint not found - skipped",
		"line": 23,
		"column": 0
	}]
}
EOD;

fwrite(STDOUT, $verbose ? "[$advice]" : "[]");
?>

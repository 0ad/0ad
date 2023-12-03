<?php
/**
 * Copyright 2023 Wildfire Games.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Linter for the project name 0 A.D..
 */
final class ProjectNameLinter extends ArcanistLinter {

  public function getInfoName() {
    return pht('Project Name Linter');
  }

  public function getLinterName() {
    return 'Project Name';
  }

  public function getLinterConfigurationName() {
    return 'project-name';
  }

  const BAD_NAME = 1;

  public function getLintSeverityMap() {
    return array(
      self::BAD_NAME => ArcanistLintSeverity::SEVERITY_WARNING,
    );
  }

  public function getLintNameMap() {
    return array(
      self::BAD_NAME => pht('Incorrect project name. Notice the non-breaking space in 0 A.D.'),
    );
  }

  public function lintPath($path) {
    $binaries_prefix = "binaries";
    if (substr($path, 0, strlen($binaries_prefix)) != $binaries_prefix) {
        return;
    }
    $txt = $this->getData($path);

    $matches = null;
    $preg = preg_match_all(
      "/((?!0 A\\.D\\.|0ad)0\\s?(?:A|a)\\.?(?:D|d)\\.?)/",
      $txt,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::BAD_NAME,
        pht('Incorrect project name. Notice the non-breaking space in 0 A.D.'),
        $string);
    }
  }
}

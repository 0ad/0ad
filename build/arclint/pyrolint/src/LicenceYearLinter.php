<?php
/**
 * Copyright 2021 Wildfire Games.
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
 * Linter for copyright years - if the modification time is incorrect, suggests update.
 */
final class LicenceYearLinter extends ArcanistLinter {

  public function getInfoName() {
    return pht('Licence Year Linter');
  }

  public function getLinterName() {
    return 'LICENCE YEAR';
  }

  public function getLinterConfigurationName() {
    return 'licence-year';
  }

  const BAD_YEAR = 1;

  public function getLintSeverityMap() {
    return array(
      // Error makes it appear even if on an unmodified line, too.
      self::BAD_YEAR => ArcanistLintSeverity::SEVERITY_ERROR,
    );
  }

  public function getLintNameMap() {
    return array(
      self::BAD_YEAR => pht('Inaccurate Copyright Year'),
    );
  }

  public function lintPath($path) {
    $txt = $this->getData($path);

    $matches = null;
    $preg = preg_match_all(
      "/Copyright( \(C\))? (20[0-9]{2}) Wildfire Games/",
      $txt,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    $year = date("Y", filemtime($path));
    foreach ($matches[2] as $match) {
      list($string, $offset) = $match;
      if ($string == $year) {
        continue;
      }
      $this->raiseLintAtOffset(
        $offset,
        self::BAD_YEAR,
        pht('Inaccurate Copyright Year'),
        $string,
        "$year");
    }
  }
}

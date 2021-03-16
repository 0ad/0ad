<?php
/**
 * Copyright 2016 Pinterest, Inc.
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
 * Copyright 2021 Wildfire Games.
 * Lints JavaScript via ESlint.
 * Heavily modified from pinterest/arcanist-linters. Original licence above.
 */
final class ESLintLinter extends ArcanistExternalLinter {

  private $config;

  public function getLinterName() {
    return 'ESLINT';
  }

  public function getLinterConfigurationName() {
    return 'eslint';
  }

  public function getDefaultBinary() {
    return 'eslint';
  }

  public function getInstallInstructions() {
    return pht(
      'Install eslint using npm install -g eslint.',
      'Install the brace-rule plugin using',
      'npm install -g eslint-plugin-brace-rules');
  }

  protected function getMandatoryFlags() {
    list($err, $stdout, $stderr) = exec_manual('npm root -g');
    return array(
      '--format=json',
      '--no-color',
      '--config',
      $this->config,
      // This allows globally installing plugins even with eslint 6+
      '--resolve-plugins-relative-to',
      strtok($stdout, "\n")
    );
  }

  public function getLinterConfigurationOptions() {
    $options = array(
      'config' => array(
        'type' => 'optional string',
        'help' => pht('Link to the config file.'),
      ),
    );
    return $options + parent::getLinterConfigurationOptions();
  }

  public function setLinterConfigurationValue($key, $value) {
    switch ($key) {
      case 'config':
        $this->config = $value;
        return;
    }
    return parent::setLinterConfigurationValue($key, $value);
  }

  protected function parseLinterOutput($path, $err, $stdout, $stderr) {
    // Gate on $stderr b/c $err (exit code) is expected.
    if ($stderr) {
      return false;
    }

    $json = json_decode($stdout, true);
    $messages = array();

    foreach ($json as $file) {
      foreach ($file['messages'] as $offense) {
        // Skip file ignored warning: if a file is ignored by .eslintingore
        // but linted explicitly (by arcanist), a warning will be reported,
        // containing only: `{fatal:false,severity:1,message:...}`.
        if (strpos($offense['message'], "File ignored ") === 0) {
          continue;
        }

        $message = new ArcanistLintMessage();
        $message->setPath($file['filePath']);
        $message->setSeverity($this->mapSeverity(idx($offense, 'severity', '0')));
        $message->setName(nonempty(idx($offense, 'ruleId'), 'unknown'));
        $message->setDescription(idx($offense, 'message'));
        $message->setLine(idx($offense, 'line'));
        $message->setChar(idx($offense, 'column'));
        $message->setCode($this->getLinterName());
        $messages[] = $message;
      }
    }

    return $messages;
  }

  private function mapSeverity($eslintSeverity) {
    switch($eslintSeverity) {
      case '0':
        return ArcanistLintSeverity::SEVERITY_ADVICE;
      case '1':
        return ArcanistLintSeverity::SEVERITY_WARNING;
      case '2':
      default:
        return ArcanistLintSeverity::SEVERITY_ERROR;
    }
  }
}

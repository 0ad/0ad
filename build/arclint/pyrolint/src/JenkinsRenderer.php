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
 * The phabricator-jenkins-plugin expects a lint format
 * that isn't quite compatible with `arc lint --output json`.
 * This adds a custom output style `jenkins` that is.
 * It's heavily based on Arcanist's regular JSON renderer.
 * See https://github.com/uber/phabricator-jenkins-plugin/issues/255.
 * The expected format is one line per message, as a dictionary.
 */

function remove_null($val) {
    return !is_null($val);
}

final class JenkinsRenderer extends ArcanistLintRenderer {

  const RENDERERKEY = 'jenkins';

  const LINES_OF_CONTEXT = 3;

  public function renderLintResult(ArcanistLintResult $result) {
    $messages = $result->getMessages();
    $path = $result->getPath();
    $data = explode("\n", $result->getData());
    array_unshift($data, ''); // make the line numbers work as array indices

    foreach ($messages as $message) {
      $dictionary = $message->toDictionary();
      $dictionary['context'] = implode("\n", array_slice(
        $data,
        max(1, $message->getLine() - self::LINES_OF_CONTEXT),
        self::LINES_OF_CONTEXT * 2 + 1));
      $dictionary['path'] = $path;
      $this->writeOut(json_encode(array_filter($dictionary, 'remove_null'))."\n");
    }
  }
}

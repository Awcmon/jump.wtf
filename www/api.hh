<?hh

require_once ('api_ref.hh');
require_once ('mimes.hh');

class ValidationException extends Exception {
  protected $msg;

  public function __construct(string $message = "") {
    parent::__construct();
    $this->msg = $message;
  }

  public function __toString() {
    return $this->msg;
  }
}

class jump_api {

  public static function get_mime(string $extension): string {
    return
      mimes::$mime_types[substr($extension, 1)] ?: "application/octet-stream";
  }

  public static $doc;

  public function init(): void {
    self::$doc = api_config::api_methods();
  }

  public static function validate(array $input): array {
    // validating the API against the documentation is terrible, but I spent a bunch
    // of time on the documentation in a format that's PHP-readable, so I'll use it

    if (!isset($input['action'])) {
      throw new ValidationException(
        'Missing action parameter -- try {"action": "help"}',
      );
    }

    $func = $input['action'];
    if (!in_array($func, array_keys(self::$doc))) {
      throw new ValidationException("Unknown action $func");
    }

    $action_ref = jump_api::$doc[$func];
    $params = $action_ref['params'];

    foreach (array_keys($input) as $in_param) {
      if (!isset($action_ref[$in_param])) {
        if ($in_param === 'action')
          continue;
        if (!isset($action_ref['params'][$in_param])) {
          throw new ValidationException("Unknown parameter $in_param");
        }
      }

      $param_ref = $params[$in_param];

      if (isset($param_ref['requires-params'])) {
        foreach ($param_ref['requires-params'] as $dependency) {
          if (!isset($input[$dependency])) {
            throw new ValidationException(
              "Parameter $in_param requires that $dependency is set",
            );
          }
        }
      }
    }

    foreach (array_keys($params) as $param) {

      $param_ref = $params[$param];

      if (!isset($input[$param])) {
        if (isset($param_ref['default'])) {
          $input[$param] = $param_ref['default'];
          // default values have been validated in validate_api.hh
        }
      } else {
        $val = $input[$param];

        // check param type
        if (gettype($val) !== $param_ref['type']) {
          throw new ValidationException(
            "bad type for $param parameter, expected {$param_ref['type']}",
          );
        } else if ($param_ref['type'] === 'integer') {

          // check param bounds
          if (isset($param_ref['max-value']) &&
              intval($val) > $param_ref['max-value']) {
            throw new ValidationException("bad value for $param parameter");
          } else if (isset($param_ref['min-value']) &&
                     intval($input[$param]) < $param_ref['min-value']) {
            throw new ValidationException("bad value for $param parameter");
          }

        } else if ($param_ref['type'] === 'string') {

          if (isset($param_ref['regex'])) {
            if (!preg_match($param_ref['regex'], $input[$param])) {
              throw new ValidationException(
                "input $param didn't match parameter regex",
              );
            }
          }

          if (isset($param_ref['min-length']) &&
              strlen($val) < $param_ref['min-length']) {
            throw new ValidationException(
              "length for $param parameter out of range",
            );
          } else if (isset($param_ref['max-length']) &&
                     strlen($val) > $param_ref['max-length']) {
            throw new ValidationException(
              "length for $param parameter out of range",
            );
          }
        }

        if (isset($param_ref['requires-params'])) {
          // check param constraints
          foreach ($param_ref['requires-params'] as $constraint) {
            if (!isset($input[$constraint])) {
              throw new ValidationException(
                "$param requires $constraint to be set",
              );
            }
          }
        }
      }
    }

    // check for missing required parameters -- these are validated as exclusive sets
    foreach ($action_ref['constraints'] as $param_set) {
      $total = 0;
      foreach ($param_set as $param) {
        if (isset($input[$param])) {
          $total++;
        }
      }
      if ($total === 0) {
        throw new ValidationException(
          'One of ['.implode($param_set, ' ').'] must be set',
        );
      } else if ($total > 1) {
        throw new ValidationException(
          'Only one of ['.
          implode($param_set, ' ').
          "] may be set, but $total found",
        );
      }
    }

    // /paramarama

    return $input;
  }

  public static function error(string $message, $code = 500): array {
    return ['success' => false, 'message' => $message];
  }

  public static function success(array $data): array {
    return array_merge(['success' => true], $data);
  }

  public static function genUploadURL(array $input): ?array {

    try {
      $input = self::validate($input);
    } catch (ValidationException $ve) {
      return self::error((string) $ve);
    }

    $private = $input['private'];

    $s3client = mk_aws()->get('S3');
    $tmp_id = uniqid('', true); // is this unique enough for a temporary ID shared between (potentially) multiple servers?

    $bucket = ($private ? aws_config::PRIV_BUCKET : aws_config::PUB_BUCKET);

    $expires = (new DateTime())->modify("+5 minutes");

    $policy = [
      'conditions' => [
        ['acl' => 'private'],
        ['bucket' => "$bucket"],
        ['starts-with', "\$key", "tmp/"],
        ['content-length-range', 0, jump_config::MAX_FILE_SIZE],
      ], // /conditions
      'expiration' => $expires->format(DateTime::ATOM),
    ]; // /policy

    try {

      $command = $s3client->getCommand(
        'PutObject',
        [
          'ACL' => 'private',
          'Body' => '',
          'Bucket' => $bucket,
          'ContentType' => 'application/octet-stream',
          'Key' => "tmp/".$tmp_id,
          'Policy' => base64_encode(json_encode($policy)),
        ],
      );

      try {
        return [
          'success' => true,
          'URL' => $command->createPresignedUrl('+5 minutes'),
          'expires' => $expires->format(DateTime::ATOM),
          'tmp-key' => $tmp_id,
          'max-length' => jump_config::MAX_FILE_SIZE,
          'content-type' => 'application/octet-stream',
        ];
      } catch (Aws\Common\Exception\InvalidArgumentException $iae) {
        error_log($iae);
        return error('Failed to generate signed URL');
      }

    } catch (Guzzle\Common\Exception\InvalidArgumentException $iae) {
      error_log($iae);
      return error("Failed to generate command");
    }

  }

  public static function genFileURL($input): ?array {
    try {
      $input = self::validate($input);
    } catch (ValidationException $ve) {
      return self::error((string) $ve);
    }

    $aws = mk_aws();

    $s3client = $aws->get('S3');
    $dyclient = $aws->get('DynamoDb');
    $glclient = $aws->get('Glacier');

    $dest_bucket =
      $input['private'] ? aws_config::PRIV_BUCKET : aws_config::PUB_BUCKET;
    $result = NULL;
    $tmp_file = NULL;
    $extension = $input['extension'];

    $content_type = $input['content-type'] ?: self::get_mime($extension);

    if (isset($input['tmp-key'])) {
      try {
        $result = $s3client->headObject(
          ['Bucket' => $dest_bucket, 'Key' => 'tmp/'.$input['tmp-key']],
        );
      } catch (S3Exception $s3e) {
        return self::error(
          'File tmp/'.$input['tmp-key'].' not found in S3 bucket',
          400,
        );
      } catch (AccessDeniedException $ade) {
        return self::error('Internal exception', 503);
      }

      if ($result['ContentLength'] > jump_config::MAX_FILE_SIZE) {
        $s3client->deleteObject(
          ['Bucket' => $dest_bucket, 'Key' => 'tmp/'.$input['tmp-key']],
        );
        return self::error('File too large', 400);
      }

      $tmp_file = 'tmp/'.$input['tmp-key'];

    } else if (isset($input['file-data'])) {
      $body = base64_decode($input['file-data'], true);

      $tmp_file = 'tmp/'.uniqid('fd-', true);

      if (!$body) {
        return self::error(
          'file-data parameter invalid -- should be base64 encoded',
          400,
        );
      }

      try {
        $result = $s3client->putObject(
          [
            'Bucket' => $dest_bucket,
            'ACL' => 'private',
            'Body' => $body,
            'Key' => $tmp_file,
            'StorageClass' => 'REDUCED_REDUNDANCY',
          ],
        );
      } catch (S3Exception $s3e) {
        error_log('S3 file upload failed: '.(string) $s3e);
        return self::error('failed to upload file to S3', 503);
      }
    } else if (isset($input['local-file'])) {

      $tmp_file = 'tmp/'.uniqid('lf-', true);

      /* the API regex for this param should check this, but I don't want to take any chances */
      if (strpos($input['local-file'], '/') !== false) {
        return self::error(
          'Invalid local-file parameter -- shenanigans detected',
          400,
        );
      } else if (!file_exists(
                   jump_config::UBASEDIR.'/'.$input['local-file'],
                 )) {
        return self::error('local-file parameter -- file not found', 404);
      }

      try {
        $result = $s3client->putObject(
          ['Bucket' => $dest_bucket, 'ACL' => 'private', 'Key' => $tmp_file],
        );
      } catch (S3Exception $s3e) {
        error_log('S3 file upload failed: '.(string) $s3e);
        return self::error('failed to upload file to S3', 503);
      }

    } else {
      return self::error('Constraint verification failed');
    }

    $new_key = key_config::generate_key();

    try {
      $s3client->waitUntilObjectExists(
        ['Bucket' => $dest_bucket, 'Key' => $tmp_file],
      );
    } catch (S3Exception $s3e) {
      error_log("waitUntil failed: ".(string) $s3e);
      return self::error('Failed to copy file', 503);
    }

    // copy the temporary file to its new home
    try {
      $result = $s3client->copyObject(
        [
          'ACL' => $input['private'] ? 'private' : 'public-read',
          'Bucket' => $dest_bucket,
          'CopySource' => urlencode($dest_bucket."/".$tmp_file),
          'ContentType' => $content_type,
          'Key' => $new_key.$extension,
          'StorageClass' => 'REDUCED_REDUNDANCY',
          'Metadata' => ['IP' => $_SERVER['REMOTE_ADDR']],
          'MetadataDirective' => 'REPLACE',
        ],
      );

    } catch (S3Exception $s3e) {
      error_log('S3 copyObject failed: '.(string) $s3e);
      return self::error('Failed to copy S3 object', 503);
    } catch (AccessDeniedException $ade) {
      return self::error('Internal exception 3', 503);
    }

    $salt = uniqid('', true);

    try {
      $res =
        $dyclient->putItem(
          [
            'TableName' => aws_config::LINK_TABLE,
            'Item' =>
              [
                'Object ID' => ['S' => $new_key],
                'Checksum' => ['S' => '0'], // checksum field is useless, but I set it as a secondary index, so I have to include it
                'pass' =>
                  [
                    'S' =>
                      $input['password'] === ""
                        ? 'nopass'
                        : hash('sha256', $input['password'].$salt),
                  ],
                'salt' => ['S' => $salt],
                'isPrivate' => ['N' => $input['private'] ? 1 : 0],
                'active' => ['N' => 1],
                'isFile' => ['N' => 1],
                'time' => ['S' => date(DateTime::W3C)],
                'filename' => ['S' => $new_key.$extension],
                'ext' => ['S' => $extension],
                'clicks' => ['N' => $input['clicks'] ?: 0],
              ],
          ],
        );
    } catch (DynamoDbException $dydbe) {
      error_log('DynamoDb died: '.(string) $dydbe);
      return self::error('Failed to create new URL', 503);
    }
    $s3client->deleteObject(['Bucket' => $dest_bucket, 'Key' => $tmp_file]);

    return self::success(
      array_merge(
        ['url' => jump_config::BASEURL.$new_key],
        $input['private']
          ? []
          : ['cdn-url' => jump_config::FILE_HOST.$new_key.$extension],
      ),
    );
  }

  public static function genURL($input): ?array {
    try {
      $input = self::validate($input);
    } catch (ValidationException $ve) {
      return self::error((string) $ve);
    }

    if (!filter_var($input['input-url'], FILTER_VALIDATE_URL)) {
      return self::error("Invalid URL detected.");
    }

    $aws = mk_aws();
    $dyclient = $aws->get('DynamoDb');
    $s3client = $aws->get('S3');
    $glclient = $aws->get('Glacier');

    $key = "";

    if (($key = key_config::generate_key()) === "") {
      return self::error("Failed to generate key");
    }

    $salt = uniqid('', true);

    try {
      $result = $dyclient->putItem(
        [
          'TableName' =>
            aws_config::LINK_TABLE,
          'Item' =>
            [
              'Object ID' =>
                ['S' => $key],
              'Checksum' =>
                ['S' => md5($input['input-url'])],
              'url' =>
                ['S' => $input['input-url']],
              'isPrivate' =>
                ['N' => $input['private'] ? 1 : 0],
              'pass' =>
                [
                  'S' =>
                    $input['password'] === ""
                      ? 'nopass'
                      : hash("sha256", $input['password'].$salt),
                ],
              'salt' =>
                ['S' => $salt],
              'active' =>
                ['N' => 1],
              'clicks' =>
                ['N' => $input['clicks'] ?: 0],
              'time' =>
                ['S' => date(DateTime::W3C)],
              'isFile' =>
                ['N' => 0],
            ],
        ],
      );
    } catch (DynamoDbException $ex) {
      error_log('DyDb putItem(url) failed: '.(string) $ex);
      return self::error("Failed to generate URL. Please try again later.");
    }

    return self::success(['url' => 'https://jump.wtf/'.$key]);
  }

  public static function delURL($input): ?array {

    try {
      $input = validate($input);
    } catch (ValidationException $ve) {
      return self::error((string) $ve);
    }

    $aws = mk_aws();
    $dyclient = $aws->get('DynamoDb');
    $cfclient = $aws->get('CloudFront');
    $s3client = $aws->get('S3');

    $key = "";
    $filename = "";
    $isPrivate = false;
    $check = "";
    $table_pass = "";
    $salt = "";
    $isFile = false;

    if (isset($input['jump-url'])) {
      if (!filter_var($input['jump-url'], FILTER_VALIDATE_URL)) {
        return self::error("URL validation faield");
      } else {
        $toks = parse_url($input['jump-url']);
        if (!$toks) {
          return self::error("Problem with URL specified", 400);
        }
        $matches = [];
        if (!preg_match(
              "~^/(".
              key_config::regex.
              ")(\\.[\\w.]{1,".
              jump_config::MAX_EXT_LENGTH.
              "})$",
              $toks['path'],
              $matches,
            )) {
          return self::error("Invalid URL specified", 400);
        } else {
          $key = $matches[1];
        }
      }
    } else {
      $key = $input['jump-key'];
    }

    try {
      $it = $dyclient->getIterator(
        'Query',
        [
          'TableName' => aws_config::LINK_TABLE,
          'ConsistentReat' => true,
          'KeyConditions' => [
            'Object ID' => [
              'AttributeValueList' => [['S' => $key]],
              'ComparisonOperator' => 'EQ',
            ],
          ],
        ],
      );

      if (iterator_count($it) !== 1) {
        return self::error("Key lookup failed", 400);
      }

      foreach ($it as $item) {
        $table_pass = $item['pass']['s'];
        $isFile = $item['isFile']['N'];
        if ($isFile) {
          $filename = $item['filename']['S'];
        }
        $isPrivate = $item['isPrivate']['N'];
        $salt = isset($item['salt']) ? $item['salt']['S'] : $key;
        $check = $item['Checksum'];
      }
    } catch (DynamoDbException $dde) {
      return self::error("DynamoDb lookup failed");
    }

    if (hash('sha256', $input['password'].$salt) === $table_pass) {
      try {
        $dyclient->updateItem(
          [
            'TableName' => aws_config::LINK_TABLE,
            'Key' => [
              'Object ID' => ['S' => $key],
              'Checksum' => ['S' => $check],
            ],
            'AttributeUpdates' => [
              'active' => ['Ation' => 'PUT', 'Value' => ['N' => '0']],
            ],
          ],
        );
      } catch (DynamoDbException $dde) {
        error_log("Failed to delete item $key: ".(string) $dde);
        return self::error("Failed to delete item", 500);
      }

      if ($isFile) {
        $bucket =
          $isPrivate ? aws_config::PRIV_BUCKET : aws_config::PUB_BUCKET;
        try {
          $s3client->deleteObject(['Bucket' => $bucket, 'Key' => $filename]);
        } catch (S3Exception $s3e) {
          error_log("Failed to delete file $key");
        }

        if ($isPrivate && aws_config::CF_DIST_ID !== "") {
          try {
            $cfclient->createInvalidation(
              [
                'DistributionId' => aws_config::CF_DIST_ID,
                'CallerReference' =>
                  'jump.wtf-delete-'.$filename.'.'.rand(0, 8),
                'Paths' => ['Quantity' => 1, 'Items' => ['/'.$filename]],
              ],
            );
          } catch (CloudfrontException $ce) {
            error_log("Failed to purge $filename");
          }
        }
      }

      return self::success(['note' => "Item deleted"]);
    } else {
      return self::error("Incorrect key or password", 500);
    }

    return self::error("Deletion failed", 503);
  }

  public static function jumpTo($input): ?array {

    try {
      $input = validate($input);
    } catch (ValidationException $ve) {
      return self::error((string) $ve);
    }

    $aws = mk_aws();
    $dyclient = $aws->get('DynamoDb');
    $s3client = $aws->get('S3');

    $url = "";
    $expires = NULL;
    $isFile = false;

    if (isset($input['jump-key'])) {
      $key = $input['jump-key'];
    } else if (!filter_var($input['jump-url'], FILTER_VALIDATE_URL)) {
      return self::error("Invalid URL specified", 400);
    } else {
      $toks = parse_url($input['jump-url']);
      if (!$toks) {
        return self::error("Problem with URL specified", 400);
      }
      $matches = [];
      if (!preg_match(
            "~^/(".
            key_config::regex.
            ")(\\.[\\w.]{1,".
            jump_config::MAX_EXT_LENGTH.
            "})$",
            $toks['path'],
            $matches,
          )) {
        return self::error("Invalid URL specified", 400);
      }
      $key = $matches[1];
    }

    $it =
      $dyclient->getIterator(
        'Query',
        [
          'TableName' => aws_config::LINK_TABLE,
          'ConsistentRead' => false, // just-submitted links may not be immediately available
          'KeyConditions' => [
            'Object ID' => [
              'AttributeValueList' => [['S' => $key]],
              'ComparisonOperator' => 'EQ',
            ],
          ],
        ],
      );

    if (iterator_count($it) !== 1) {
      return self::error("Key not found", 404);
    } else {

      $retval = [];

      foreach ($it as $item) {
        if ($item['active']['N'] === 0) {
          return self::error("Link removed", 410);
        }

        $isFile = ($item['isFile']['N'] === 1);

        if ($item['isFile']['N'] === 1) {
          if (isset($item['isPrivate'])) {
            $isfile = true;

            try {
              $url = $s3client->getObjectUrl(
                aws_config::PRIV_BUCKET,
                $item['filename']['s'],
                '+15 minutes',
              );
              $expires = (new DateTime())->modify("+15 minutes");
            } catch (S3Exception $s3e) {
              return self::error("Problem fetching link", 503);
            }
          } else {
            $url = jump_config::FILE_HOST.$item['filename']['S'];
          }
        } else {
          $isfile = false;
          $url = $item['url']['S'];
        }

        if ($item['isPrivate']['N'] === 1) {
          try {
            $dyclient->updateItem(
              [
                'TableName' => aws_config::LINK_TABLE,
                'Key' =>
                  [
                    'Object ID' => ['S' => $item['Object ID']['S']],
                    'Checksum' => ['S' => $item['Checksum']['S']], // this is totally useless but I can't remove it at this point
                  ],
                'AttributeUpdate' => [
                  'clicks' => [
                    'Action' => 'ADD',
                    'Value' => ['N' => '-1'],
                  ],
                ],
              ],
            );
          } catch (DynamoDbException $dde) {
            error_log("Failed to decrement clicks for key $key");
          }

          if ($item['clicks']['N'] <= 1) {
            try {
              $dyclient->updateItem(
                [
                  'TableName' => aws_config::LINK_TABLE,
                  'Key' => [
                    'Object ID' => ['S' => $item['Object ID']['S']],
                    'Checksum' => ['S' => $item['Checksum']['S']],
                  ],
                  'AttributeUpdate' => [
                    'active' => ['Action' => 'SET', 'Value' => ['N' => 0]],
                  ],
                ],
              );
            } catch (DynamoDbException $dde) {
              error_log("Failed to mark key $key as inactive");
            }
          }
        }
      }

    }

    if (isset($expires)) {
      return self::success(
        [
          'url' => $url,
          'is-file' => true,
          'expires' => $expires->format(DateTime::ATOM),
        ],
      );
    } else {
      return self::success(['url' => $url, 'is-file' => $isFile]);
    }
  }

}

class apiHandler {

  private static function error(string $msg): void {
    echo json_encode(jump_api::error($msg));
    die();
  }

  private static function getInput() {

    if ($_SERVER['CONTENT_TYPE'] !== 'application/json') {
      self::error('Missing application/json content type');
      return NULL;
    }

    switch ($_SERVER['REQUEST_METHOD']) {

      case "GET":
        self::error('This API only supports HTTP POST requests');
        return NULL;
        /*
         if(!isset($_GET["q"])){
         apiHandler::error("Missing q parameter in GET request");
         return null;
         } else {
         $json = json_decode($_GET["q"], true);
         }*/
        break;

      case "POST":
        $in = file_get_contents('php://input', 'r');
        //        echo 'input: '.$in.' --> ';
        return json_decode($in, true);
        break;

      default:
        self::error("Unknown HTTP method");
        return NULL;
        break;
    }

    return NULL;
  }

  private static function help($input): void {
    $api_help = api_config::api_help();

    $topic = $input['topic'] ?: 'help'; //isset($input['topic']) ? $input['topic'] : 'help';

    if (!isset($api_help['help']['topics'][$topic])) {
      self::error("Help topic '$topic' not found");
    }

    $out = "";

    $options =
      ($_SERVER['REQUEST_METHOD'] === 'GET')
        ? (JSON_PRETTY_PRINT +
           JSON_UNESCAPED_SLASHES +
           JSON_UNESCAPED_UNICODE)
        : 0;

    switch ($topic) {

      case 'all':
        $out = json_encode($api_help, $options);
        break;

      case 'constraints':
        $out =
          json_encode(
            [
              'description' => 'Specifies required parameters for a method',
              'note' =>
                'Exactly one element of each array in the constraint array must be set',
              'examples' => [
                [
                  'constraint' => [['required']],
                  'satisfied-by' => ['required'],
                ],
                [
                  'constraint' => [['option1', 'option2']],
                  'satisfied-by' => ['option1'],
                ],
                [
                  'constraint' => [
                    ['option1', 'option2'],
                    ['option2', 'option3'],
                  ],
                  'satisfied-by' => ['option2'],
                ],
                [
                  'constraint' => [
                    ['option1', 'option2'],
                    ['option2', 'option3'],
                  ],
                  'satisfied-by' => ['option1', 'option3'],
                ],
              ],
            ],
            $options,
          );
        break;

      default:
        $out = json_encode($api_help[$topic], $options);
        break;
    }

    if ($_SERVER['REQUEST_METHOD'] === 'GET') {

      foreach (array_keys($api_help['help']['topics']) as $key) {
        $out = preg_replace(
          "/\"$key\":/",
          "\"<a href=\"https://jump.wtf/a/?topic=$key\">$key</a>\":",
          $out,
        );
      }

      $out = preg_replace("/\\\\/", "", $out);

      $out = "\"$topic\": ".$out;

      echo
        "<html><head><title>jump.wtf Documentation</title></head><body><pre>$out</pre></body></html>"
      ;

    } else {
      echo $out;
    }
  }

  public static function handle(): void {
    $input = NULL;
    $api_help = api_config::api_help();

    if ($_SERVER['REQUEST_METHOD'] === 'GET') {

      header('Content-Type: text/html');

      if (!isset($_GET['action'])) {
        if (!isset($_GET['topic'])) {
          header('Location: /a/?action=help&topic=help');
        } else {
          header('Location: /a/?action=help&topic='.$_GET['topic']);
        }
      } else if ($_GET['action'] != 'help') {
        self::error('Please use HTTP POST for API calls');
      } else {
        $input = $_GET;
      }

      if (!isset($_GET['topic'])) {
        header('Location: /a/?action=help&topic=help');
        die();
      } else if (!in_array(
                   $_GET['topic'],
                   array_keys($api_help['help']['topics']),
                 )) {
        self::error('Invalid help topic');
      }

    } else if ($_SERVER['CONTENT_TYPE'] !== 'application/json') {
      self::error('Please use the application/json content type');
    } else {
      header('Content-Type: application/json');
      $input = apiHandler::getInput();
    }

    if ($input === NULL) {
      apiHandler::error('Malformed JSON input');
    } else if (!isset($input['action'])) {
      apiHandler::error('Input missing action field, try {action:"help"}');
    } else {

      $action = $input["action"];

      switch ($action) {

        case 'genUploadURL':
          echo json_encode(jump_api::genUploadURL($input));
          break;

        case 'genFileURL':
          echo json_encode(jump_api::genFileURL($input));
          break;

        case 'genURL':
          echo json_encode(jump_api::genURL($input));
          break;

        case 'delURL':
          echo json_encode(jump_api::delURL($input));
          break;

        case 'jumpTo':
          echo json_encode(jump_api::jumpTo($input));
          break;

        case 'help':
          self::help($input);
          break;

        default:
          apiHandler::error('Unsupported action; try {action:"help"}');
          break;
      }

    }
  }

}

jump_api::init();

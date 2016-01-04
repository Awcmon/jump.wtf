<?hh

include ('blackhole/blackhole.hh');

require_once ('vendor/facebook/xhp-lib/init.php');

require_once ('header.hh');
require_once ('forms.hh');
require_once ('footer.hh');

require_once ('aws.hh');

require_once ('config/aws_config.hh');
require_once ('config/jump_config.hh');
require_once ('config/key_config.hh');

require ('index.hh');
require ('go.hh');
require ('submit.hh');
require ('result.hh');
require ('error.hh');
require ('api.hh');

function main(): void {

  $matches = array();

  $uri = "";

  if (!preg_match("~/([^/?]*)(\?.*)?~", $_SERVER['REQUEST_URI'], $matches)) {
    i_main();
  } else {
    $uri = $matches[1];
  }

  if ($uri === 'a') {
    apiHandler::handle();
  } else if ($uri === 's') {
    s_main();
  } else if ($uri === 'r') {
    r_main();
  } else if ($uri === '404') {
    error_page(404, $uri);
  } else if ($uri !== '') {
    g_main($uri);
  } else /*if ($uri === "")*/ {
    i_main();
  }
}

main();

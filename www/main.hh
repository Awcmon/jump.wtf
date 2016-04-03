<?hh

include ('blackhole/blackhole.hh');

//require_once ('vendor/facebook/xhp-lib/init.php');

function main(): void {

  $matches = array();

  $uri = "";

  if (!preg_match("~/([^/?]*)(\?.*)?~", $_SERVER['REQUEST_URI'], $matches)) {
      require ('index.hh');
      i_main();
  } else {
    $uri = $matches[1];
  }

  if ($uri === 'a') {
      require ('api.hh');
    apiHandler::handle();
  } else if ($uri === 's') {
      require ('submit.hh');
    s_main();
  } else if ($uri === 'r') {
      require ('result.hh');
    r_main();
  } else if ($uri === '404') {
      require ('error.hh');
    error_page(404, $uri);
  } else if ($uri !== '') {
      require ('go.hh');  
      g_main($uri);
  } else /*if ($uri === "")*/ {
      require ('index.hh');
      i_main();
  }
}

main();

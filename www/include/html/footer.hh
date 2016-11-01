<?hh

require_once ('vendor/facebook/xhp-lib/init.php');

require_once ('helpers.hh');

if (!include_once('config/jump_config.hh')) {
  require_once ('config/jump_config.hh');
}

function gen_footer(): mixed {
  return
    <footer class="footer">
      <div class="container text-center">
        <p class="text-muted">&copy; Phillip Goldfarb {date("Y")}</p>
      </div>
    </footer>;
}

function gen_footer_scripts(): mixed {
  $cdn_host = cdn_host();

  if (substr($_SERVER['SERVER_NAME'], -strlen('.onion')) === '.onion') {
    $cdn_host = jump_config::H_CDN_HOST;
  }

  return
    <p>
      <!--
        <script src={$cdn_host."/vendor/jquery-1.11.1.min.js"}></script>
      --> <script src="//code.jquery.com/jquery-2.2.4.min.js"></script>
      <script src={$cdn_host."/vendor/clipboard.js/dist/clipboard.min.js"}>
      </script>
      <script src={$cdn_host."/vendor/bootstrap/dist/js/bootstrap.min.js"}>
      </script>
      <script src={$cdn_host."/vendor/clippy.js/build/clippy.min.js"}>
      </script>
      <script
        async={true}
        src={$cdn_host."/js/".file_get_contents("htdocs/js/main.js.latest")}>
      </script>
    </p>;
}

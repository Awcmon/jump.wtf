<?hh
/*
include('b/blackhole.hh');

require('p/header.hh');
require('p/footer.hh');
*/

function r_main(): void{

	session_start();

	echo gen_html_tag();

	echo gen_head();

	$body = <body></body>;
	$body->appendChild(gen_nav());

	$con = <div class="container centered"></div>;

	if($_SESSION['action'] == 'del_success'){
		$con->appendChild(<h2>Deletion successful</h2>);
		$con->appendChild(<p>Link deleted.</p>);
	} else if($_SESSION['action'] != 'del_success') {
		if($_SESSION['action'] == 'gen_success'){
			$con->appendChild(<h1>Link Generated</h1>);
		} else {
			$con->appendChild(<h1>File Uploaded</h1>);
		}
		$con->appendChild(<p>Your link has been generated: <a href={$_SESSION['new_link']} target="_blank">{$_SESSION['new_link']}</a></p>);
		$con->appendChild(<br />);
		$con->appendChild(<button id="copybutton" class="btn btn-default" data-clipboard-text={$_SESSION["new_link"]}><span class="glyphicon glyphicon-share" aria-hidden="true"></span>Copy to clipboard</button>);
		$con->appendChild(<script src="https://cdn.jump.wtf/h/js/vendor/ZeroClipboard.min.js"></script>);
		$con->appendChild(<script src="https://cdn.jump.wtf/h/js/clip-0.min.js"></script>);
	} else {
		$con->appendChild(<h1>Error!</h1>);
		$con->appendChild(<p>$_SESSION['problem']</p>);
	}

	for( $i = 0; $i < 4; $i++){
		$con->appendChild(<br />);
	}

	$con->appendChild(<a href=".">Go back</a>);

	$body->appendChild(<div class="jumbotron">{$con}</div>);

	echo $body;

	echo "</html>";

//	session_unset();
}

//r_main();

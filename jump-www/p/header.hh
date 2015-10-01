<?hh

function gen_html_tag(){
return "<!DOCTYPE html>
<!--[if lt IE 7]>      <html class=\"no-js lt-ie9 lt-ie8 lt-ie7\"> <![endif]-->
<!--[if IE 7]>         <html class=\"no-js lt-ie9 lt-ie8\"> <![endif]-->
<!--[if IE 8]>         <html class=\"no-js lt-ie9\"> <![endif]-->
<!--[if gt IE 8]><!--> <html class=\"no-js\"> <!--<![endif]-->";
}

function gen_head(){
return <head>
<meta charset="utf-8" />
<meta http-equiv="X-UA-Compatible" content="IE=edge" />
<title>JUMP.WTF</title>
<meta name="description" content="" />
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no" />
<link rel="shortcut icon" type="image/x-icon" href="https://f.jump.wtf/favicon.ico" />
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/vendor/bootstrap/dist/css/bootstrap.min.css"} />
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/vendor/bootstrap/dist/css/bootstrap-theme.min.css"} />
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/css/" . file_get_contents("h/css/main.css.latest")} />
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/vendor/clippy.js/build/clippy.css"} media="all" />
<link rel="stylesheet" href="//fonts.googleapis.com/css?family=Montserrat:700" type="text/css" data-noprefix="" />
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/vendor/fork-ribbon/gh-fork-ribbon.min.css"} />
<!--[if lt IE 9]>
<link rel="stylesheet" href={jump_config::CDN_HOST . "/h/vendor/fork-ribbon/gh-fork-ribbon.ie.min.css" />
<![endif]-->
</head>;
}

function gen_nav(){

//echo "<body>";
return <nav class="navbar navbar-inverse navbar-fixed-top">
		<div class="container">
				<div class="navbar-header">
				    <div class="github-fork-ribbon-wrapper left">
					    <div class="github-fork-ribbon">
							<a href="https://github.com/millinon/jump.wtf">Fork me on GitHub</a>
						</div>
					</div>
<a class="navbar-brand" href="https://jump.wtf">JUMP.WTF</a>
				</div>
				<div id="navbar" class="navbar-collapse collapse">
				</div>
		</div>
</nav>;
}

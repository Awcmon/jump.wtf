<?hh

		function gen_form(){
		return
		<div class="container-fluid">
			<div class="row equalrow">
				<div class="formgroup col-xs-12 col-sm-12 col-md-4 col-lg-4 col-md-offset-1 col-lg-offset-1 equal">
					<h2>Submit a Link or File</h2>
					<form id="new_submit" action="s" method="post"  enctype="multipart/form-data" onsubmit="subbutton()">
						<div class="input-group" style="width:80%">
							<span class="input-group-addon" style="width:6em; text-align:left">
								<input type="radio" name="sub_type" value="url" id="sel_url" checked={true} />&nbsp;URL:
							</span>
							<input type="url" class="form-control" name="new_url" id="new_url" placeholder="http://www.example.com/" maxlength={128} autofocus={true} required={true} autocomplete="off" />
						</div>
						<div class="input-group" style="width:80%">
							<span class="input-group-addon" style="width:6em; text-align:left">
									<input type="radio" name="sub_type" value="file" id="sel_file" />&nbsp;File:
							</span>
							<input type="hidden" name="MAX_FILE_SIZE" value="8388608" />
							<input type="file" class="form-control" name="file" id="new_file" disabled={true} required={true}/>
						</div>
						<div class="input-group" style="width:80%">
							<span class="input-group-addon">Password for deletion:</span>
							<input type="password" class="form-control" name="pass" id="pass" maxlength={20} autocomplete="off" placeholder="(optional)" />
						</div>
						<div class="input-group" style="width: 50%">
							<span class="input-group-addon">
							<input type="checkbox" name="expires" id="expires" onclick="t1()" />&nbsp;Expire after clicks:
							</span>
							<input type="number" class="form-control" name="clicks" id="clicks" min={1.0} max={1000.0} step={1.0} value="" style="width:6em" disabled={true} />
						</div>
						<input type="hidden" name="action" id="action" value="new" />
						<button id="newsub" type="submit" class="btn btn-primary">
							<span id="newsub_glyph" class="glyphicon glyphicon-link" aria-hidden="true"></span>&nbsp;Submit
						</button>
					</form>
				</div>
				<div class="clearfix visible-sm-block"></div>
				<div class="formgroup col-xs-12 col-sm-12 col-md-4 col-lg-4 col-md-offset-2 col-lg-offset-2 equal toosmall" style="height:100%">
					<h2>Delete a Link</h2>
					<form id="del_form" action="s" method="post">
						<div class="input-group" style="width:80%">
							<span class="input-group-addon">ID:</span>
								<input type="text" class="form-control" name="del_url" id="del_url" pattern="(https?://jump.wtf/)?[A-Za-z0-9]{2,6}" placeholder="AbcD0" required={true} autocomplete="off" style="width:8em" />
						</div>
						<div class="input-group" style="width:80%">
							<span class="input-group-addon">
								Password:
							</span>
							<input type="password" class="form-control" name="del_pass" id="del_pass" maxlength={20} autocomplete="off" style="width:12em" />
						</div>
						<input type="hidden" name="action" id="d_action" value="del" />
						<button type="submit" class="btn btn-primary">
							<span class="glyphicon glyphicon-trash" aria-hidden="true"></span>&nbsp;Submit
						</button>
					</form>
				</div>
			</div>
		</div>;
		}

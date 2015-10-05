<?hh

require('p/aws_config.hh');
require('p/jump_config.hh');
require('p/aws.hh');

function jump_log(){

	if(jump_config::DO_LOG){

		$page = preg_replace('|^/?([a-z])(\.hh)?(/.*)?|', '$1', $_SERVER['PHP_SELF']);

		$dyclient = mk_aws()->get('DynamoDb');

		$dyclient->updateItem(array(
			'TableName' => aws_config::LOG_TABLE,
			'Key' => array(
				'IP' => array(
					'S' => "TOTAL")),
			'AttributeUpdates' => array(
				$page => array(
					'Action' => 'ADD',
					'Value' => array(
						'N' => 1)),
				'ALL' => array(
					'Action' => 'ADD',
					'Value' => array(
						'N' => 1))
			)
		));

		$dyclient->updateItem(array(
			'TableName' => aws_config::LOG_TABLE,
			'Key' => array(
				'IP' => array(
					'S' => $_SERVER['REMOTE_ADDR'])),
			'AttributeUpdates' => array(
				$page => array(
					'Action' => 'ADD',
					'Value' => array(
						'N' => 1)),
				'ALL' => array(
					'Action' => 'ADD',
					'Value' => array(
						'N' => 1)),
				'last_user' => array(
					'Action' => 'PUT',
					'Value' => array(
						'S' => $_SERVER['HTTP_USER_AGENT']))
			)
		));

	}

}

jump_log();

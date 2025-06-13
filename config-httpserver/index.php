<?php
require_once('Mobile_Detect.php');
$detect = new Mobile_Detect;

$head_type='index';
if($detect->isMobile() && !$detect->isTablet())
{
	include('mobile_index.php');
	die();
}
else
{
	include('pc_index.php');
	die();
}
?>
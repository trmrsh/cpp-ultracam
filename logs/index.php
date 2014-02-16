<!DOCTYPE html>
<html>
<head>
<!-- insert directory component before logs -->
<title>
<?php
$cwd  = getcwd();
$dirs = explode("/", $cwd);
$len  = count($dirs);
echo strtoupper($dirs[$len-2]);
?> logs</title>

<link rel="stylesheet" type="text/css" href="ultra.css" />

<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js">
</script>

<!-- allow initialisation to a particular night using php. The php
part is replaced by the server and sent to the client. Pass the night
when calling the page using ?night= ...
 -->
<script type="text/javascript">
var night =
<?php
  if(array_key_exists('night', $_GET)){
    echo '"' . htmlspecialchars($_GET["night"]) . '"';
  }else{
    echo '"undef"';
  }
?>;
</script>

<script src="ultra_guide.js">
</script>

</head>

<body>

<div id="guidecontent">
Guide
</object></div>

<div id="titlecontent">
Title
</object></div>

<div id="logcontent">
Log
</object></div>

</body>
</html>

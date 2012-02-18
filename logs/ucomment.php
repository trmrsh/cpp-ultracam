<html>
<head>
<title>ULTRACAM comments</title>
<link rel="stylesheet" href="ultracam_ilogs.css">
</head>

<body>

<?php 

$user      = '"' . $_SERVER["REMOTE_USER"] . '"';
$wtype     = '"' . htmlspecialchars($_REQUEST['wtype']) . '"';
$date      = '"' . htmlspecialchars($_REQUEST['date']) . '"';
$run       = '"' . htmlspecialchars($_REQUEST['run']) . '"';
$attribute = '"' . htmlspecialchars($_REQUEST['attribute']) .'"';
$comment   = '"' . htmlspecialchars($_REQUEST['comment']) .'"';

system('./ucomment.py ' . $user . ' ' .$wtype . ' ' . $date 
   . ' ' . $run . ' ' . $attribute . ' ' . $comment);

?>

<address>
Tom Marsh, Warwick
</address>
</body>
</html>

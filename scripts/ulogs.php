<html>
<head>
<title>ULTRACAM interactive logs</title>
<link rel="stylesheet" href="ultracam_ilogs.css">
</head>


<body>

<h1>ULTRACAM interactive logs</h1>

<p>
This page is designed to help trace ULTRACAM runs, i.e. to answer burning
questions such as <a title="ULTRACAM in-joke">"did we ever observe NN Ser?"</a>. 

<?php 

$slimits = htmlspecialchars($_REQUEST['slimits']);
$target  = htmlspecialchars($_REQUEST['target']);
$delta   = htmlspecialchars($_REQUEST['delta']);
$ra1     = htmlspecialchars($_REQUEST['RA1']);
$ra2     = htmlspecialchars($_REQUEST['RA2']);
$dec1    = htmlspecialchars($_REQUEST['Dec1']);
$dec2    = htmlspecialchars($_REQUEST['Dec2']);
$emin    = htmlspecialchars($_REQUEST['emin']);
$unique  = htmlspecialchars($_REQUEST['unique']);

if($slimits == "simbad"){
  $args = $slimits . ' ' . '"' . $target . '"' . ' ' . $delta . ' ' . $emin . ' ' . $unique;
}else{
  $args = $slimits . ' ' . $ra1 . ' ' . $ra2 . ' ' . $dec1 . ' ' . $dec2 . ' '
  . $emin . ' ' . '"' . $target . '"' . ' ' . $delta . ' ' . $unique;
}
system('../../python/ulogs.py ' . $args)
?>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>

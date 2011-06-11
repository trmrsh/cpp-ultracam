<html>
<head>
<title>ULTRACAM interactive logs</title>
<link rel="stylesheet" href="ultracam_test.css">

<script type="text/javascript">

function submitenter(myfield,e) {
   var keycode;
   if (window.event) {
      keycode = window.event.keyCode;
   } else if (e) {
      keycode = e.which;
   } else {
     return true;
   }

   if (keycode == 13){
     myfield.form.submit();
     return false;
   } else {
     return true;
   }
}

</script>

</head>


<body>

<h1>ULTRACAM interactive logs</h1>

<p>
This page is designed to help trace ULTRACAM runs, i.e. to answer burning
questions such as "did we ever observe NN Ser, and if so, when?". Below,
the RA and Dec should be specified in decimal hours and degrees respectively.
Once you have set your limits, press enter on any of the fields and the page
should update. 

<?php 

$ra1    = $_REQUEST['RA1'];
$ra2    = $_REQUEST['RA2'];
$dec1   = $_REQUEST['Dec1'];
$dec2   = $_REQUEST['Dec2'];
$unique = $_REQUEST['unique'];

if($unique == "Unique nights only"){
  $args = $ra1 . ' ' . $ra2 . ' ' . $dec1 . ' ' . $dec2 . ' y';
}else{
  $args = $ra1 . ' ' . $ra2 . ' ' . $dec1 . ' ' . $dec2 . ' n';
}

system('../../python/ulogs.py ' . $args)
?>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>

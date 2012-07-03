<html>
<head>
<title>ULTRACAM comments</title>
<link rel="stylesheet" href="ultracam_ilogs.css">

<script type="text/javascript">
<!--
  function display(obj) {

    // Hides/reveals file upload entry depending upon whether
    // 'Reduced' has been selected.

    if(navigator.appName.indexOf("Microsoft") > -1){
      var canSee = "block"
    } else {
      var canSee = "table-row";
    }

    var selection = obj.options[obj.selectedIndex].value;

    var fupload   = document.getElementById("fupload");

    fupload.style.display = "none";
    if ( selection.match("Reduced") ) {
      fupload.style.display = canSee;
    }
  }

  // some client-side validation to avoid having to oback to the server
  function validate() {

    var att = document.getElementById("attsel");

    if(!att || att.value == "Noselection"){
       alert("You must select an attribute. Select 'Unspecified' if you do" +
       " not want to define an attribute but still wish to make a comment.");
       return false;

    }else if(att.value == "Reduced"){

      var fup = document.getElementById("upload");

      if(!fup.value){
         alert("The 'Reduced' attribute requires upload of the corresponding" +
         " as a fits.gz file.");
         return false;
      }
    }

    var com = document.getElementById("comment");

    if(!com || com.value.length < 3){
       alert("You must make some sort of comment.");
       return false;
    }

    return true;
  }

//-->
</script>

</head>

<body>

<?php 

$user      = '"' . $_SERVER["REMOTE_USER"] . '"';
$wtype     = '"' . htmlspecialchars($_REQUEST['wtype']) . '"';
$date      = '"' . htmlspecialchars($_REQUEST['date']) . '"';
$run       = '"' . htmlspecialchars($_REQUEST['run']) . '"';
$attribute = '"' . htmlspecialchars($_REQUEST['attribute']) .'"';
$comment   = '"' . htmlspecialchars($_REQUEST['comment']) .'"';
$ofile     = '"' . $_FILES['redfile']['name'] . '"';
$tfile     = '"' . $_FILES['redfile']['tmp_name'] . '"';

system('../../python/ucomment.py ' . $user . ' ' .$wtype . ' ' . $date 
   . ' ' . $run . ' ' . $attribute . ' ' . $comment . ' ' . 
   $ofile . ' ' . $tfile);

?>

<address>
Tom Marsh, Warwick
</address>
</body>
</html>

<?php
// Script to search databases of ULTRACAM and ULTRASPEC targets
// for access by instrument drivers.

// Get target
$target = htmlspecialchars($_REQUEST['target']);

// instrument
$inst = htmlspecialchars($_REQUEST['instrument']);

// Load json file
if($inst == "ultraspec"){
    $file = "ultraspec/logs/ultra.json";
}else if($inst == "ultracam"){
    $file = "ultracam/logs/ultra.json";
}else{
    echo "Instrument = $inst not recognised\n";
    break;
}

$data = json_decode(file_get_contents($file), true);
$ndata = count($data);

$found = -1;
for($n=0; $n < $ndata; $n++)
    if($target == $data[$n]['target']) $found = $n;

if($found < 0){
    echo "Target name = $target not recognized.\n";
}else{
    $ref = $data[$found];
    echo "Target name = $target successfully matched.\n";
    echo "ID = {$ref['id']}, RA = {$ref['ra']}, Dec = {$ref['dec']}\n";
}

?>

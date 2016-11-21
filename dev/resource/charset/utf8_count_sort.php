<?

$con = file_get_contents("../../sample/charset/count.txt");


function utf8_len($ch)
{
    $ch  = ord($ch);
    if (($ch <= 0x7F))
    {
        return 1;
    }
    else if (($ch & 0xF0) == 0xF0)
    {
        return 4;
    }
    else if (($ch & 0xE0) == 0xE0)
    {
        return 3;
    }
    else if (($ch & 0xC0) == 0xC0)
    {
        return 2;
    }
    else
    {
        return 5;
    }

    return 0;
}

$w2=Array();
$w3=Array();
for($i=0;$i<strlen($con);)
{
    $wlen = utf8_len($con[$i]);
    if($wlen < 2)
    {
        $i++;
        die("find ASCII\n");
    }
    if ($wlen>3)
    {
        $i+= $wlen + 1;
        //echo("find ASCII which's length is bigger 3\n");
        continue;
    }
    if ($wlen == 2)
    {
        $idx = $con[$i].$con[$i+1];
        $i+= $wlen;
        $w2[$idx] = ord($con[$i]);
        $i+=1;
        continue;
    }
    if ($wlen == 3)
    {
        $idx=$con[$i].$con[$i+1].$con[$i+2];
        $i+= $wlen;
        $w3[$idx] = ord($con[$i]);
        $i+=1;
        continue;
    }
}
$w_keys=array_keys($w2);
sort($w_keys);
$w22=Array();
foreach($w_keys as $w)
{
    $w22[] = sprintf("\x%2x\x%2x\x%02x", ord($w[0]), ord($w[1]), $w2[$w]);
}
echo "unsigned char utf8_list2[]=\"", join("", $w22),"\";\n";
echo "int utf8_list_count2=", count($w22), ";\n";

$w3_keys=array_keys($w3);
sort($w3_keys);
$WWW=Array();
for($i=0;$i<16;$i++)
{
    $WWW[$i] = Array();
}
foreach($w3_keys as $w)
{
    $p = ord($w[0]);
    $p = $p & 0X0F;
    $WWW[$p][$w[1].$w[2]] = $w3[$w];
}
$L="unsigned char *utf8_list3[17] = {\n";
$C="int utf8_list_count3[17] = {";
for($i=0;$i<16;$i++)
{
    $list = $WWW[$i];
    $w_keys=array_keys($list);
    sort($w_keys);
    $w22=Array();
    foreach($w_keys as $w)
    {
        if(strlen($w) < 2)
        {
            die("strlen: ".strlen($w));
        }
        $w22[] = sprintf("\x%2x\x%2x\x%02x", ord($w[0]), ord($w[1]), $list[$w]);
    }

    $L .= "    \"".join("", $w22)."\",\n";
    $C .= count($w22).", ";
}
$L .= "    0\n};";
$C .= "0};";
echo $L, "\n";
echo $C, "\n";

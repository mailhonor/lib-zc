<?

$con = file_get_contents("a.txt");


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
        continue;
    }
    if ($wlen>3)
    {
        $i+= $wlen;
        continue;
    }
    if ($wlen == 2)
    {
        $w2[$con[$i].$con[$i+1]]=1;
        $i+= $wlen;
        continue;
    }
    if ($wlen == 3)
    {
        $w3[$con[$i].$con[$i+1].$con[$i+2]]=1;
        $i+= $wlen;
        continue;
    }
}
$w2=array_keys($w2);
sort($w2);
$w22=Array();
foreach($w2 as $w)
{
    $w22[] = sprintf("\x%2x\x%2x", ord($w[0]), ord($w[1]));
}
echo "unsigned char utf8_list2[]=\"", join("", $w22),"\";\n";
echo "int utf8_list_count2=", count($w22), ";\n";



$w3=array_keys($w3);
sort($w3);
$WWW=Array();
for($i=0;$i<16;$i++)
{
    $WWW[$i] = Array();
}
foreach($w3 as $w)
{
    $p = ord($w[0]);
    $p = $p & 0X0F;
    $WWW[$p][] = sprintf("\x%2x\x%2x", ord($w[1]), ord($w[2]));
}
$L="unsigned char *utf8_list3[17] = {\n";
$C="int utf8_list_count3[17] = {";
for($i=0;$i<16;$i++)
{
    $list = $WWW[$i];
    $L .= "    \"".join("", $list)."\",\n";
    $C .= count($list).", ";
}
$L .= "    0\n};";
$C .= "0};";
echo $L, "\n";
echo $C, "\n";

<?
if ($argc < 3) {
	echo $argv[0], " d/e name-string/filename\n";
}
$decode_mode = false;
if ($argv[1] == 'd') {
	$decode_mode = true;
}
$a = $argv[2];
if (!is_file($a)) {
	if ($decode_mode) {
		$b = mb_convert_encoding($a, 'UTF-8', 'UTF7-IMAP');
	} else {
		$b = mb_convert_encoding($a, 'UTF7-IMAP', 'UTF-8');
	}
	echo "$b\n";
	exit(0);
}
$fp = fopen($a, "r");
if (!$fp) {
	echo "Cannot open file $a\n";
	exit(1);
}
while (($line = fgets($fp)) !== false) {
	$line = rtrim($line, "\r\n");
	if ($decode_mode) {
		$b = mb_convert_encoding($line, 'UTF-8', 'UTF7-IMAP');
	} else {
		$b = mb_convert_encoding($line, 'UTF7-IMAP', 'UTF-8');
	}
	echo "$b\n";
}
fclose($fp);

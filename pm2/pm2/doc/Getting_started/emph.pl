
$emph = '(?:here\s*[!]?)';

$emph_pattern = '(?:\s*/\*[*\s]*' . $emph . '[*\s]*\*/\s*)';
$substpattern = "\001verbatimemph|";

if ($#ARGV != 0) {
    die "Usage: $0 file-to-emph";
}

$filename = $ARGV[0];

$in = "$filename";
$out = "$in-emph";

print "Emphasizing |$in|...\n";

open (IN, "<$in") || 
die ("Cannot open |$in| for reading");

open (OUT, ">$out") || 
die ("Cannot open |$out| for writing");

while (defined ($line = <IN>)) {
    if  ($line =~ m/^(\s*)(.*)$emph_pattern$/io) {
	$line = "$1$substpattern$2\n";
    }
    print OUT $line;
}

close (IN);
close (OUT);


$cut_here = '(?:Cut\s+here)';

$start_pattern = '(?:\A\s*)';
$cut_pattern = '(?:\s*/\*[*\s]*' . $cut_here . '[*\s]*\*/\s*)';
$end_pattern = '(?:\s*\Z)';
$match_pattern = '(?:[^\s].*[^\s])';

if ($#ARGV != 0) {
    die "Usage: $0 file-to-split";
}

$filename = $ARGV[0];

$in = "$filename";
$out0 = "${filename}-0";
$out1 = "${filename}-1";

print "Splitting |$in|...\n";

open (IN, "<$in") || 
die ("Cannot open |$in| for reading");
$text = "";
while (defined ($line = <IN>)) {
    $text .= $line;
}
close (IN);

unless ($text =~ m/$start_pattern($match_pattern)$cut_pattern/sio) {
    die ("Bad match for file \|in|, first part");
}

open (OUT, ">$out0") ||
    die ("Cannot open |$out0| for writing");
print OUT $1;

unless ($text =~ m/$cut_pattern($match_pattern)$end_pattern/sio) {
    die ("Bad match for file \|in|, second part");
}

close (OUT);
open (OUT, ">$out1") ||
    die ("Cannot open |$out1| for writing");
print OUT $1;

close (OUT);

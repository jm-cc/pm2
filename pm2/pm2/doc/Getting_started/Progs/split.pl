
$cut = 'Cut\s+here';

if ($#ARGV != 0) {
    die "Usage: $0 file-to-split";
}

$filename = $ARGV[0];

$in = "$filename";
$out0 = "${filename}-0";
$out1 = "${filename}-1";

print "Splitting |$in| into |$out0| and |$out1| at |$cut|\n";

open (IN, "<$in") || 
die ("Cannot open |$in| for reading");

open (OUT, ">$out0") ||
    die ("Cannot open |$out0| for writing");

while (defined($line = <IN>)) {
    if ($line =~ /$cut/) {
	close (OUT);
	open (OUT, ">$out1") ||
	    die ("Cannot open |$out1| for writing");
	while (($line = <IN>) =~ /^\s*$/) {;}
	print OUT "$line";
	next;
    }
    print OUT "$line";
}
    

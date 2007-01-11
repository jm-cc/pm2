#!/usr/bin/perl

$line_emph = '(?:[Hh]ere\s*[!]?)';

$start_emph = '(?:[Ss]tart\s*[!]?)';
$end_emph = '(?:[Ee]nd\s*[!]?)';

$line_emph_pattern = '(?:\s*/\*[*\s]*' . $line_emph . '[*\s]*\*/\s*)';
$start_emph_pattern = '(?:\s*/\*[*\s]*' . $start_emph . '[*\s]*\*/\s*)';
$end_emph_pattern = '(?:\s*/\*[*\s]*' . $end_emph . '[*\s]*\*/\s*)';

if ($#ARGV != 2) {
    die "Usage: $0 file-to-emph out-file mode";
}

$filename = $ARGV[0];
$out = $ARGV[1];
$mode = $ARGV[2];

if ($mode eq "ps") {
  $start_subst_pattern = "\001";
  $end_subst_pattern = "\002";
}
else {
  $start_subst_pattern = "<B>";
  $end_subst_pattern = "</B>";
}

$in = "$filename";

print "Emphasizing |$in|...\n";

open (IN, "<$in") || 
die ("Cannot open |$in| for reading");

open (OUT, ">$out") || 
die ("Cannot open |$out| for writing");

while (defined ($line = <IN>)) {

## Suppress EOL

    $line =~ s/$//o;

    if ($mode eq "html") {
      $line =~ s/&/&amp;/g;
      $line =~ s/</&lt;/g;
    }

## Line emphasize: start + line + end + EOL

    if  ($line =~ s/$line_emph_pattern//o) {
	$line =~ s/\A/$start_subst_pattern/o;
	$line =~ s/\Z/$end_subst_pattern/o;
	$line .= "\n";
    }

## Start/Stop emphasize: replace with start/stop and suppress EOF so
## that they can stand on dedicated line without disturbing

    else {
	$line =~ s/$start_emph_pattern/$start_subst_pattern/o;
	$line =~ s/$end_emph_pattern/$end_subst_pattern/io;
    }

## Print out line

    print OUT $line;
}

close (IN);
close (OUT);

#!/usr/bin/perl

sub error {
  my $msg = $_[0];

  if ($msg) {
    print $msg;
  }
  print "Error. Syntax: pm2-indent.log.pl <log file> [<maximum depth>]\n";
  die;
}

sub printLine {
  my $line = $_[0];
  my $depth = $_[1];
  my $maxDepth = $_[2];

  if ($maxDepth != -1 && $depth > $maxDepth) {
    return;
  }
  for(my $i=0 ; $i<$depth ; $i++) {
    print "  ";
  }
  print $line;
}

$ARGV[0] || error();
my $depth = -1;
my $maxDepth = -1;
my $nbArgs = scalar(@ARGV);
if ($ARGV[1] =~ m/^\d*$/) {
  if ($nbArgs >= 2) {
    $maxDepth = $ARGV[1];
  }
}
else {
  error("Error. The 2nd argument must be an integer\n");
}

open(FILE,"< $ARGV[0]") || error("Cannot open file $ARGV[0]\n");
while (<FILE>) {
  my $line = $_;

  if ($line =~ m/-->/) {
    $depth = $depth+1;
    printLine($line, $depth, $maxDepth);
  }
  elsif ($line =~ "<--") {
    printLine($line, $depth, $maxDepth);
    $depth = $depth - 1;
  }
  else {
    printLine($line, $depth, $maxDepth);
  }
}
close(FILE);

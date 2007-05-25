#!/usr/bin/perl

# TODO: utiliser tput setf 1; tput setb 7 et tput sgr0 plutôt

my $blueOnWhite = "\033[0;34;47m";
my $normal = "\033[0m";

sub error {
  my $msg = $_[0];

  if ($msg) {
    print $msg;
  }
  print "Error. Syntax: pm2-indent-log.pl [-bold] <log file> [<maximum depth>]\n";
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
  print $line."\n";
}

$ARGV[0] || error();
my $maxDepth = -1;
my $bold = 0;

my $file = $ARGV[0];
my $argDepth = $ARGV[1];

if ($ARGV[0] =~ m/-bold/) {
  $bold = 1;
  $file = $ARGV[1];
  $argDepth = $ARGV[2];
}

if ($argDepth =~ m/^\d+$/) {
  $maxDepth = $argDepth;
}
else {
  $maxDepth = -1;
}

my $depth = -1;
open(FILE,"< $file") || error("Cannot open file $file\n");
while (<FILE>) {
  my $line = $_;
  chop($line);

  if ($line =~ m/-->/) {
    $depth = $depth+1;
    printLine($line, $depth, $maxDepth);
  }
  elsif ($line =~ "<--") {
    printLine($line, $depth, $maxDepth);
    $depth = $depth - 1;
  }
  else {
    $depth = $depth+1;
    if ($bold == 1) {
      printLine($blueOnWhite.$line.$normal, $depth, $maxDepth);
    }
    else {
      printLine($line, $depth, $maxDepth);
    }
    $depth = $depth-1;
  }
}
close(FILE);

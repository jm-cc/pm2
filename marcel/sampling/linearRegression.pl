#!/usr/bin/perl

sub mean {
    my ($dataref) = @_;

    my $sum = 0;
    my $length = scalar(@$dataref);
    foreach $value (@$dataref) {
	$sum += $value;
    }
    return $sum / $length;
}

sub covar {
    my ($xdataref, $ydataref) = @_;

    my $prod = 0;
    my $length = scalar(@$xdataref);
    for(my $i=0 ; $i<$length ; $i++) {
        #print "f(@$xdataref[$i]) = @$ydataref[$i]\n";
        $prod += @$xdataref[$i] * @$ydataref[$i];
    }
    my $x_m = mean($xdataref);
    my $y_m = mean($ydataref);
    return $prod/$length - $x_m*$y_m;
}

sub var {
    my ($dataref) = @_;

    my $var = 0;
    my $mean = mean($dataref);
    my $length = scalar(@$dataref);
    foreach $value (@$dataref) {
	$var += ($value - $mean)**2;
    }
    return $var/$length;
}

sub linearRegression {
    my ($xdataref, $ydataref) = @_;
    my $mean_x = mean($xdataref);
    my $mean_y = mean($ydataref);

    my $var_x = var($xdataref);
    my $var_y = var($ydataref);

    my $covar_xy = covar($xdataref, $ydataref);

    my $a = $covar_xy / $var_x;
    my $b = -$a * $mean_x + $mean_y;
    my $r = $covar_xy / sqrt($var_x) / sqrt($var_y);

    return ($a, $b, $r);
}

sub help {
    print "Syntax: linearRegression.pl [<hostname>] [-file <sampling filename>] \n";
    print "            [-min minimum value for the x coordinates] [-max maximum value for the x coordinates]\n";
    print "            [-plot] [-nodumb]\n";
    print "  Performs a linear regression for the cost of the memory migration on hostname\n";
    print "  with:\n";
    print "      -plot   displays the plot for the sampling result (by default does not display the plot)\n";
    print "      -nodumb displays the plot in graphical mode (by default in dumb mode)\n";
    exit;
}
# Main program
# Linear regression to exhibit a cost model y = a*x + b.
#      x = number of pages,
#      y = migration cost.
# xdatas and ydatas are matrices indiced on the node id. The cell
# [source][dest] of the x (resp. y) matrix will contain all the x
# (resp. y) values for the sampling done between the nodes source and
# dest.

# Set the default parameters
my $maxnode=10;
my $plot = 0;
my $dumb = 1;
my $filename = "";
my $hostname = "";
my $x_min = 0;               # Minimum size for the x coordinates
my $x_max = 100000;          # Maximum size for the x coordinates

# Read the command-line parameters
for(my $i=0 ; $i<scalar(@ARGV) ; $i++) {
    if ($ARGV[$i] eq "-help") {
        help();
    }
    elsif ($ARGV[$i] eq "-min") {
	$x_min=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-max") {
	$x_max=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-plot") {
	$plot = 1;
    }
    elsif ($ARGV[$i] eq "-nodumb") {
	$dumb = 0;
    }
    elsif ($ARGV[$i] eq "-file") {
	$filename = $ARGV[$i+1];
        $i++;
    }
    else {
        $hostname = $ARGV[$i];
    }
}

# Initialise the datas to store the x and y values.
my @xdatas;
my @ydatas;
for $source (0 .. $maxnode) {
    for $dest (0 .. $maxnode) {
	my @xlist = ();
	$xdatas[$source][$dest] = \@xlist;
	my @ylist = ();
	$ydatas[$source][$dest] = \@ylist;
    }
}
for $source (0 .. $maxnode) {
    for $dest (0 .. $maxnode) {
	my $xlistref = $xdatas[$source][$dest];
	my $ylistref = $ydatas[$source][$dest];
	#print "Lists $xlistref $ylistref\n";
    }
}

# Get the location of the sampling results output file
if ($filename eq "") {
    my $pathname = $ENV{PM2_CONF_DIR};
    if ($pathname ne "") {
        $pathname .= "/marcel";
    }
    else {
        $pathname = $ENV{PM2_HOME};
        if ($pathname eq "") {
            $pathname = $ENV{HOME};
        }
        $pathname .= "/.pm2/marcel";
    }
    $filename = "$pathname/sampling_$hostname.txt";
}

# Read the x and y values and store them in the appropriate list
open input,$input="$filename" or die "Cannot open $input: $!";
my $head = <input>; # read out the first line
while(<input>) {
    chomp;
    ($source, $dest, $pages, $size, $cost) = split();
    if ($pages >= $x_min && $pages <= $x_max) {
	my $xlistref = $xdatas[$source][$dest];
	push(@$xlistref, $pages);
	my $ylistref = $ydatas[$source][$dest];
	push(@$ylistref, $cost);
    }
}
close(input);

# For each pair $source $dest, perform the linear regression and plot
# the result
for $source (0 .. $maxnode) {
    for $dest (0 .. $maxnode) {
	my $xlistref = $xdatas[$source][$dest];
	my $ylistref = $ydatas[$source][$dest];
	if (scalar(@$xlistref) != 0) {
	    print "\n\nProcessing values for source = $source and dest = $dest\n";
	    my ($a, $b, $r) = linearRegression($xlistref, $ylistref);
	    print "y = $a * x + $b\n";
	    print "r (pearson coefficient) = $r\n";

	    if ($plot) {
		my $filename = "sampling_${source}_${dest}";
		open output,$output=">${filename}.txt" or die "Cannot open $output: $!";
		my $l = scalar(@$xlistref);
		for $i (0 .. $l) {
		    my $reg=$a * @$xlistref[$i] + $b;
		    print output "@$xlistref[$i] @$ylistref[$i] $reg\n";
		}
		close(output);
		open gnuplot,$gnuplot=">${filename}.gnu" or die "Cannot open $gnuplot: $!";
		if ($dumb) {
		    print gnuplot "set terminal dumb\n";
		}
		print gnuplot "set title \"Source $source - Dest $dest\"\n";
		print gnuplot "plot '${filename}.txt' using 1:2 title \"Original\" with lines, '${filename}.txt' using 1:3 title \"Regression\" with lines\n";
		print gnuplot "pause -1\n";
		close(gnuplot);
		system("gnuplot $filename.gnu");
	    }
	}
    }
}

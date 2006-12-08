#!/usr/bin/env pm2perl

my @tokens = ();

sub parse_config_file {
    my ($file) = @_;
    my $contents = "";
    open(INFILE,"<$file") || die "Cannot open file $file: $!\n";
    while (<INFILE>) {
        my $line = $_;
        chop($line);
        $contents .= $line;
    }
    close(INFILE);
    $contents =~ s/,/ , /g;
    $contents =~ s/\(/ \( /g;
    $contents =~ s/\)/ \) /g;
    $contents =~ s/;/ ; /g;
    $contents =~ s/:/ : /g;
    $contents =~ s/{/ { /g;
    $contents =~ s/}/ } /g;
    $contents =~ s/\s+/ /g;

    (my $network, my $sep, @tokens) = split(/\s/, $contents);
    if ($network ne "networks") {
        die "File <$file> incorrect. Tag <networks> expected. Found <$network>\n";
    }
}


sub get_next_cluster {
    my $complete = 0;
    my %cluster = ();

    my $token = shift(@tokens);
    while ($token ne "") {
        if ($token eq "name") {
            shift(@tokens);
            $cluster{"name"} = shift(@tokens);
            $complete ++;
            if ($complete == 3) {
                return %cluster;
            }
        }
        elsif ($token eq "dev") {
            shift(@tokens);
            $cluster{"device"} = shift(@tokens);
            $complete ++;
            if ($complete == 3) {
                return %cluster;
            }
        }
        elsif ($token eq "hosts") {
            shift(@tokens);
            my $hosts = shift(@tokens);
            if ($hosts eq "(") {
                $hosts = shift(@tokens);
                if ($hosts eq "{") {
                    my $tmp_hosts = $hosts;
                    my $tmp = shift(@tokens);
                    while ($tmp ne ")") {
                        $tmp_hosts .= $tmp;
                        $tmp = shift(@tokens);
                    }
                    $hosts = "";
                    foreach $tmp_host (split(/,/,$tmp_hosts)) {
                        $tmp_host =~ s/;parameter.*//;
                        $tmp_host =~ s/.name://;
                        $hosts .= ",$tmp_host";
                    }
                    $hosts =~ s/^,//;
                }
                else {
                    my $tmp = shift(@tokens);
                    while ($tmp ne ")") {
                        $hosts .= $tmp;
                        $tmp = shift(@tokens);
                    }
                }
            }
            $cluster{"hosts"} = $hosts;
            $complete ++;
            if ($complete == 3) {
                return %cluster;
            }
        }
        $token = shift(@tokens);
    }
    return %cluster;
}

sub check_hosts {
    my ($available_hosts, @required_hosts) = @_;    
    foreach my $host (@required_hosts) {
        if (!($available_hosts =~ m/$host/)) {
            return 0;
        }
    }
    return 1;

}

# ---------------------------------
# Main program
# ---------------------------------

my $config_file = $ARGV[0];
my $required_device = $ARGV[1];
shift(@ARGV);
shift(@ARGV);
my @required_hosts = @ARGV;

#print "Looking for @required_hosts with $required_device in $config_file\n";

parse_config_file($config_file);

my %cluster = get_next_cluster();
while (exists $cluster{name}) {
    if ((exists $cluster{device}) && ($cluster{device} eq $required_device)) {
        #print "cluster[$cluster{name}] = $cluster{hosts}\n";
        my $found = check_hosts($cluster{"hosts"}, @required_hosts);
        if ($found == 1) {
            print "$cluster{name}\n";
            exit 0;
        }
    }
    %cluster = get_next_cluster();
}
print "Network not found\n";
exit 1;



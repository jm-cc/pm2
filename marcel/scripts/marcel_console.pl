#!/usr/bin/perl -w
##################

use strict;
use IO::Handle;
use Getopt::Std;

# Globals
my $output_path	= '/tmp';
my $fifo_path	= '/tmp';
my $user	= $ENV{'USER'};


sub usage() {
	print "marcel_console.pl [<prog1> [<prog2>]] < <command_file>";
	exit;
}

sub mkfifo($) {
	my $f = shift;
	unless (-p $f) {
		system "mkfifo ${f}"
			and die "mkfifo ${f}: $!\n";
	}
}

sub mkprog($$) {
	my $num	= shift;
	my $prog	= shift;
	my $fifo	= "${fifo_path}/${user}_marcel_console_${num}.fifo";
	my $rfifo	= "${fifo_path}/${user}_marcel_console_${num}.rfifo";
	my $output	= "${output_path}/${user}_marcel_console_${num}.log";

	mkfifo($fifo);
	mkfifo($rfifo);

	if ($prog ne '') {
		print "system: MARCEL_SUPERVISOR_FIFO=${fifo} MARCEL_SUPERVISOR_RFIFO=${rfifo} $prog > $output 2>&1 &\n";
		system "MARCEL_SUPERVISOR_FIFO=${fifo} MARCEL_SUPERVISOR_RFIFO=${rfifo} $prog > $output 2>&1 &";
	}

	my $fifo_fh;
	print "Opening fifo ${num}...\n";
	open ($fifo_fh, "> $fifo")
		or die "open ${fifo}: $!\n";
	autoflush $fifo_fh 1;

	my $rfifo_fh;
	print "Opening reverse fifo ${num}...\n";
	open ($rfifo_fh, "< $rfifo")
		or die "open ${rfifo}: $!\n";
	autoflush $rfifo_fh 1;

	my $ref = {};
	${$ref}{'num'}	= $num;
	${$ref}{'prog'}	= $prog;
	${$ref}{'fifo'}	= $fifo;
	${$ref}{'fifo_fh'}	= $fifo_fh;
	${$ref}{'rfifo'}	= $rfifo;
	${$ref}{'rfifo_fh'}	= $rfifo_fh;
	${$ref}{'output'}	= $output;
	
	return $ref;
}

my %cmd_set	= (
	enable	=> 1,
	disable	=> 2,
	enable_list	=> 3,
	disable_list	=> 4,
	enable_list_item	=> 5,
	disable_list_item	=> 6,
);

my $async_mode	= 0;
my $async_nb	= 0;

my %opts;
my $ret	= getopts('ha:', \%opts);

if (exists $opts{'h'}) {
	usage();
}

if (exists $opts{'a'}) {
	$async_mode	= 1;
	$async_nb	= $opts{'a'};
	die "invalid number of programs for asynchronous mode\n"
		if ($async_nb < 1  or  $async_nb > 2);
	print "Running in asynchronous mode, monitoring $async_nb program(s)\n";
}

my $prog1;
my $prog2;

if ($async_mode) {
	$prog1	= '';
	$prog2	= '';
} else {
	$prog1	= shift;
	if (defined $prog1) {
		$prog2	= shift;
	} else {
		$async_mode	= 1;
		$async_nb	= 1;
		$prog1	= '';
		$prog2	= '';
	}
}

my %prog_hash;
$prog_hash{'1'}	= mkprog(1, $prog1);
if ($async_nb == 2  or  $prog2 ne '') {
	$prog_hash{'2'}	= mkprog(2, $prog2);
}

print "Ready...\n";
while (<>) {
	chomp;

	s/\#.*$//;
	s/^\s+//;
	s/\s+$//;
	s/\s+/ /;
	next
		if (/^$/);
	my @args	= split / /;
	my $cmd	= shift @args;
	if ($cmd eq 'q') {
		# q: Quit
		print "Quit\n";
		last;
	} elsif ($cmd eq 't') {
		# t: Temporisation (arg = duration in seconds)
		print "Temporisation: $args[0] seconds\n";
		sleep $args[0];
	} elsif ($cmd eq 'e' or $cmd eq 'd') {
		# e: Enable (args = <1|2> <vpnum>)
		# d: Disable (args = <1|2> <vpnum>)
		my $cmd_name	= ($cmd eq 'e')?'enable':'disable';
		my $cmd_code	= $cmd_set{$cmd_name};
		my $prog_num	= shift @args;
		my $vpnum	= shift @args;
		my $data	= pack 'LL', ($cmd_code, $vpnum);
		my $prog;
		unless (exists $prog_hash{$prog_num}) {
			die "invalid prog_num ${prog_num}\n";
		}
		$prog	= $prog_hash{$prog_num};
		my $fh	= ${$prog}{'fifo_fh'};
		print "${cmd_name} VP ${vpnum} on prog ${prog_num}\n";
		syswrite $fh, $data or die "write to fifo: $!\n";
	} elsif ($cmd eq 'E' or $cmd eq 'D') {
		# E: Enable list (args = <1|2> <vpnum> [<vpnum>]*)
		# D: Disable list (args = <1|2> <vpnum> [<vpnum>]*)
		my $cmd_name	= ($cmd eq 'E')?'enable':'disable';
		my $cmd_code	= $cmd_set{"${cmd_name}_list"};
		my $sub_cmd_code	= $cmd_set{"${cmd_name}_list_item"};
		my $prog_num	= shift @args;
		my $nb_vps	= scalar (@args);
		my $prog;
		unless (exists $prog_hash{$prog_num}) {
			die "invalid prog_num ${prog_num}\n";
		}
		$prog	= $prog_hash{$prog_num};
		my $fh	= ${$prog}{'fifo_fh'};
		my $data	= pack 'LL', ($cmd_code, $nb_vps);
		print "${cmd_name} ${nb_vps} vp(s) on prog ${prog_num}\n";
		syswrite $fh, $data or die "write to fifo: $!\n";
		foreach my $vp (@args) {
			$data	= pack 'LL', ($sub_cmd_code, $vp);
			print ". ${cmd_name} vp ${vp}\n";
			syswrite $fh, $data or die "write to fifo: $!\n";
		}
	} elsif ($cmd eq 's') {
		# s: Sync with prog 'arg'
		my $prog_num	= shift @args;
		my $prog;
		unless (exists $prog_hash{$prog_num}) {
			die "invalid prog_num ${prog_num}\n";
		}
		$prog	= $prog_hash{$prog_num};
		my $fh	= ${$prog}{'fifo_fh'};
		print "Sync with prog ${prog_num}...\n";
		sysread $fh, my $data, 8 or die "read from fifo: $!\n";
		print "Sync with prog ${prog_num} complete\n\n";
	} elsif ($cmd eq 'S') {
		print "Sync with any prog...\n";
		my $prog_num;
		my $rin = '';
		foreach $prog_num (keys %prog_hash) {
			my $prog	= $prog_hash{$prog_num};
			vec($rin, fileno(${$prog}{'rfifo_fh'}), 1) = 1;
		}
		my $nfound	= select($rin, undef, undef, undef);
		foreach $prog_num (keys %prog_hash) {
			my $prog	= $prog_hash{$prog_num};
			if (vec($rin, fileno(${$prog}{'rfifo_fh'}), 1) == 1) {
				sysread ${$prog}{'rfifo_fh'}, my $data, 8 or die "read from rfifo ${prog_num}: $!\n";
				print "Sync-any with prog ${prog_num} complete\n";
			}
		}
	} else {
		die "unknown command: $cmd\n";
	}
}


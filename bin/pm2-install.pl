#!/usr/bin/perl
######

use strict;
use warnings;
use Getopt::Std;

# global variables
my $flavor	= 'default';
my $pm2_root;
my $install_dir;

# working directory
if (exists $ENV{'PM2_ROOT'}) {
	$pm2_root	= $ENV{'PM2_ROOT'};
	chdir $pm2_root	or die "chdir ${pm2_root}: $!\n";
} elsif ( -x 'bin/pm2-install.pl' ) {
	$pm2_root	= `pwd`;
	$ENV{'PM2_ROOT'}	= $pm2_root;
} else {
	die "pm2-install.pl must be run from PM2 extraction directory\n";
}

print "using PM2_ROOT: \t${pm2_root}\n";

# read command line args
my %opts;
my $ret		= getopts('f:d:', \%opts);	# -f flavor

if (exists $opts{'f'}) {
	$flavor	= $opts{'f'};
}

print "flavor: \t${flavor}\n";

# make sure the install directory is dedicated, for safety
if (exists $opts{'d'} and $opts{'d'}) {
	$install_dir	= $opts{'d'};

	if ( -d $install_dir ) {
		rmdir $install_dir or die "rmdir ${install_dir}: $!\n";
	}
} else {
	die "install directory not specified\n";
}

print "install directory: \t${install_dir}\n";

# clean-up PM2 tree
print "initializing PM2 tree\n";
system 'make initnoflavor';
system "pm2-create-sample-flavors ${flavor}";

# build flavor
print "building PM2 tree\n";
system "make FLAVOR=${flavor}";

# make install directory
print "making install directory: ${install_dir}\n";
mkdir $install_dir	or die "mkdir ${install_dir}: $!\n";

foreach my $i ( 'include', 'lib', 'mak', 'bin' ) {
	my $d	= "${install_dir}/${i}";
	print "making install directory: ${d}\n";
	mkdir ${d} or die "mkdir ${d}: $!\n";
}

# get pm2 settings
my $build_dir	= `pm2-config --flavor=${flavor} --builddir`;
chomp $build_dir;
print "build dir: \t${build_dir}\n";

my $modules	= `pm2-config --flavor=${flavor} --modules`;
chomp $modules;
my @modules	= split ' ', $modules;
print "modules: \t${modules}\n";

my $cflags	= `pm2-config --flavor=${flavor} --cflags`;
chomp $cflags;
print "cflags: \t${cflags}\n";

my $ldflags	= `pm2-config --flavor=${flavor} --libs-only-L`;
chomp $ldflags;
print "ldflags: \t${ldflags}\n";

my $ldlibs	= `pm2-config --flavor=${flavor} --libs-only-l`;
chomp $ldlibs;
print "ldlibs: \t${ldlibs}\n";

# install libraries
print "installing libraries\n";
foreach my $m ( @modules ) {
	foreach my $s ( '.a', '.so' ) {
		my $l	= "${build_dir}/lib/lib${m}${s}";

		if ( -r $l ) {
			print ".. installing ${l}\n";
			system "cp -v ${l} ${install_dir}/lib"
		}
	}

	if ($m eq 'marcel') {
		# marcel.lds special case
		my $f	= "${pm2_root}/marcel/scripts/marcel.lds";

		if ( -r $f ) {
			system "cp -v ${f} ${install_dir}/lib"
		}
	}
}

# install headers
print "installing headers\n";
foreach my $m ( @modules ) {
	foreach my $d ( 'include', 'autogen-include' ) {
		my $p	= "${m}/${d}";

		if ( -d "$p" ) {
			print ".. installing ${p}\n";
			system "cp -r ${p}/. ${install_dir}/include";
		}
	}

	if ($m eq 'nmad') {
	    my @flags	= split ' ', $cflags;
	    foreach my $flag (@flags) {
		if ($flag =~ /^-I/ && $flag =~ /nmad/) {
		    $flag =~ s/^-I//;

		    if ( -d "$flag" ) {
			print ".. installing ${flag}\n";
			system "cp -r ${flag}/. ${install_dir}/include";
		    }
		}
	    }
	}

	if ($m eq 'marcel') {
		# marcel 'asm' & 'scheduler' symlinks
		my $inc_dir	= `pm2-config --flavor=${flavor} --includedir marcel`;
		chomp $inc_dir;

		print "marcel include dir: \t${inc_dir}\n";

		my @inc_files	= glob("${inc_dir}/*");
		foreach my $f (@inc_files) {
			if ( -l $f ) {
				my $l	= readlink $f;
				print ".. $f --> $l\n";

				my $nf	= $f;
				$nf	=~ s,.*/,${install_dir}/include/,;

				my $nl	= $l;
				$nl	=~ s,.*autogen-include/,${install_dir}/include/,;

				print "  ** $nf ==> $nl\n";
				symlink $nl, $nf or die "symlink $nl, $nf: $!\n";
			}
		}
	}
}

# tweak cflags
my $install_cflags	= $cflags;
$install_cflags		=~ s/-I\S+//g;
$install_cflags		.= " -I${install_dir}/include";
$install_cflags		=~ s/^\s+//g;
$install_cflags		=~ s/\s+$//g;
$install_cflags		=~ s/\s\s+/ /g;
print "install_cflags: \t${install_cflags}\n";

# tweak ldflags
my $install_ldflags	= $ldflags;
$install_ldflags	=~ s/-L\S+//g;
$install_ldflags	.= " -L${install_dir}/lib";
$install_ldflags	=~ s/^\s+//g;
$install_ldflags	=~ s/\s+$//g;
$install_ldflags	=~ s/\s\s+/ /g;
print "install_ldflags: \t${install_ldflags}\n";

# tweak ldlibs
my $install_ldlibs	= $ldlibs;
$install_ldlibs		=~ s,\S+/marcel.lds,${install_dir}/lib/marcel.lds,;
$install_ldlibs		=~ s/^\s+//g;
$install_ldlibs		=~ s/\s+$//g;
$install_ldlibs		=~ s/\s\s+/ /g;
print "install_ldlibs: \t${install_ldlibs}\n";

# Makefile snippet
my $mak	= <<END_MAK;
PM2_CFLAGS	= $install_cflags
PM2_LDFLAGS	= $install_ldflags
PM2_LDLIBS	= $install_ldlibs
END_MAK

print $mak;
my $mak_p	= "${install_dir}/mak/pm2.mak";
open my $mak_fd, "> ${mak_p}" or die "open ${mak_p}: $!\n";
print $mak_fd $mak;
close $mak_fd;

# Script snippets (.sh)
my $sh	= <<END_SH;
#! /bin/sh
PM2_CFLAGS='$install_cflags'
export PM2_CFLAGS
PM2_LDFLAGS='$install_ldflags'
export PM2_LDFLAGS
PM2_LDLIBS='$install_ldlibs'
export PM2_LDLIBS
END_SH

print $sh;
my $sh_p	= "${install_dir}/bin/pm2.sh";
open my $sh_fd, "> ${sh_p}" or die "open ${sh_p}: $!\n";
print $sh_fd $sh;
close $sh_fd;

chmod 0755, $sh_p or die "chmod 0755, ${sh_p}: $!\n";

my $cfg_sh	= <<END_CFG_SH;
#! /bin/sh
case \$1 in 
  --cflags) echo '$install_cflags' ;;
  --ldflags) echo '$install_ldflags' ;;
  --ldlibs) echo '$install_ldlibs' ;;
esac
END_CFG_SH

print $cfg_sh;
my $cfg_sh_p	= "${install_dir}/bin/pm2-config.sh";
open my $cfg_sh_fd, "> ${cfg_sh_p}" or die "open ${cfg_sh_p}: $!\n";
print $cfg_sh_fd $cfg_sh;
close $cfg_sh_fd;

chmod 0755, $cfg_sh_p or die "chmod 0755, ${cfg_sh_p}: $!\n";

# Script snippets (.csh)
my $csh	= <<END_CSH;
#! /bin/csh -f
setenv PM2_CFLAGS '$install_cflags'
setenv PM2_LDFLAGS '$install_ldflags'
setenv PM2_LDLIBS '$install_ldlibs'
END_CSH

print $csh;
my $csh_p	= "${install_dir}/bin/pm2.csh";
open my $csh_fd, "> ${csh_p}" or die "open ${csh_p}: $!\n";
print $csh_fd $csh;
close $csh_fd;

chmod 0755, $csh_p or die "chmod 0755, ${csh_p}: $!\n";

my $cfg_csh	= <<END_CFG_CSH;
#! /bin/csh -f
switch ( \$1 ) 
  case --cflags:
    echo '$install_cflags'
  breaksw
  case --ldflags:
    echo '$install_ldflags'
  breaksw
  case --ldlibs:
    echo '$install_ldlibs'
  breaksw
endsw
END_CFG_CSH

print $cfg_csh;
my $cfg_csh_p	= "${install_dir}/bin/pm2-config.csh";
open my $cfg_csh_fd, "> ${cfg_csh_p}" or die "open ${cfg_csh_p}: $!\n";
print $cfg_csh_fd $cfg_csh;
close $cfg_csh_fd;

chmod 0755, $cfg_csh_p or die "chmod 0755, ${cfg_csh_p}: $!\n";

#


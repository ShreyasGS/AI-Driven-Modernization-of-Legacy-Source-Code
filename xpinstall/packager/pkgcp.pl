#!/usr/bin/perl -w
# 
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#  
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#  
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation. Portions created by Netscape are
# Copyright (C) 1998-1999 Netscape Communications Corporation. All
# Rights Reserved.
# 
# Contributor(s): 
# Jonathan Granrose (granrose@netscape.com)


# pkgcp.pl -
#
# Parse a package file and copy the specified files for a component
# from the given source directory into the given destination directory
# for packaging by the install builder.
#
# Todo:
#	- port to MacPerl
#	- change warn()s to die()s to enforce updating package files.
#	- change var names to standard form

# load modules
use Getopt::Long;
use File::Basename;

# initialize variables
%components        = ();	# list of components to copy
$line             = "";		# line being processed
$srcdir           = "";		# root directory being copied from
$destdir          = "";		# root directory being copied to
$package          = "";		# file listing files to copy
$os               = "";  	# os type (MacOS, MSDOS, Unix, OS/2)
$verbose          = 0;		# shorthand for --debug 1
$lineno           = 0;		# line # of package file for error text
$debug            = 0;		# controls amount of debug output
$help             = 0;		# flag: if set, print usage


# get command line options
$return = GetOptions(
			"source|s=s",           \$srcdir,
			"destination|d=s",      \$destdir,
			"file|f=s",             \$package,
			"os|o=s",               \$os,
			"component|c=s",        \@components,
			"help|h",               \$help,
			"debug=i",              \$debug,
			"verbose|v",            \$verbose,
			"flat|l",               \$flat,
			"<>",                   \&do_badargument
			);

# set debug level
if ($verbose && !($debug)) {
	$debug = 1;
} elsif ($debug != 0) {
	$debug = abs ($debug);
	($debug >= 2) && print "debug level: $debug\n";
}

# check usage
if (! $return)
{
	die "Error: couldn't parse command line options.  See \'$0 --help' for options.\nExiting...\n";
}

# ensure that Packager.pm is in @INC, since we might not be called from
# mozilla/xpinstall/packager.
$top_path = $0;
if ( $os eq "dos" ) {
  $top_path =~ s/\\/\//g;
}
push(@INC, dirname($top_path));
require Packager;

Packager::Copy($srcdir, $destdir, $package, $os, $flat, $help, $debug, @components);

#
# This is called by GetOptions when there are extra command line arguments
# it doesn't understand.
#
sub do_badargument
{
	warn "Warning: unknown command line option specified: @_.\n";
}


# EOF

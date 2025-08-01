                       Signing Tool (signtool)
                         1.3 Release Notes
               ========================================

Documentation is provided online at mozilla.org

Problems or questions not covered by the online documentation can be
discussed in the DevEdge Security Newsgroup.

=== New Features in 1.3
=======================

The security library components have been upgraded to utilize NSS_2_7_1_RTM.
This means that the maximum RSA keysize now supported should be 4096 bits.

=== Zigbert 0.6 Support
=======================
This program was previously named Zigbert.  The last version of zigbert
was Zigbert 0.6.  Because all the functionality of Zigbert is maintained in
signtool 1.2, Zigbert is no longer supported.  If you have problems
using Zigbert, please upgrade to signtool 1.2.

=== New Features in 1.2
=======================

Certificate Generation Improvements
-----------------------------------
Two new options have been added to control generation of self-signed object
signing certificates with the -G option. The -s option takes the size (in bits)
of the generated RSA private key.  The -t option takes the name of the PKCS #11
token on which to generate the keypair and install the certificate.  Both
options are optional.  By default, the private key is 1024 bits and is generated
on the internal software token.


=== New Features in 1.1
=======================

File I/O
--------
Signtool can now read its options from a command file specified with the -f
option on the command line. The format for the file is described in the
documentation.
Error messages and informational output can be redirected to an output file
by supplying the "--outfile" option on the command line or the "outfile="
option in the command file.

New Options
-----------
"--norecurse" tells Signtool not to recurse into subdirectories when signing
directories or parsing HTML with the -J option.
"--leavearc" tells Signtool not to delete the temporary .arc directories
produced by the -J option.  This can aid debugging.
"--verbosity" tells Signtool how much information to display. 0 is the
default. -1 suppresses most messages, except for errors.

=== Bug Fixes in 1.1
====================

-J option revamped
------------------
The -J option, which parses HTML files, extracts Java and Javascript code,
and stores them in signed JAR files, has been re-implemented. Several bugs
have been fixed:
- CODEBASE attribute is no longer ignored
- CLASS and SRC attributes can be be paths ("xxx/xxx/x.class") rather than
  just filenames ("x.class").
- LINK tags are handled correctly
- various HTML parsing bugs fixed
- error messages are more informative

No Password on Key Database
---------------------------
If you had not yet set a Communicator password (which locks key3.db, the
key database), signtool would fail with a cryptic error message whenever it
attempted to verify the password.  Now this condition is detected at the
beginning of the program, and a more informative message is displayed.

-x and -e Options
-----------------
Previously, only one of each of these options could be specified on the command
line. Now arbitrarily many can be specified.  For example, to sign only files
with .class or .js extensions, the arguments "-eclass -ejs" could both be
specified. To exclude the directories "subdir1" and "subdir2" from signing,
the arguments "-x subdir1 -x subdir2" could both be specified.

New Features in 1.0
===================

Creation of JAR files
----------------------
The -Z option causes signtool to output a JAR file formed by storing the
signed archive in ZIP format.  This eliminates the need to use a separate ZIP
utility.  The -c option specifies the compression level of the resulting
JAR file.

Generation of Object-Signing Certificates and Keys
--------------------------------------------------
The -G option will create a new, self-signed object-signing certificate
which can be used for testing purposes.  The generated certificate and 
associated public and private keys will be installed in the cert7.db and
key3.db files in the directory specified with the -d option (unless the key
is generated on an external token using the -t option). On Unix systems,
if no directory is specified, the user's Netscape directory (~/.netscape)
will be used. In addition, the certificate is output in X509 format to the
files x509.raw and x509.cacert in the current directory.  x509.cacert can
be published on a web page and imported into browsers that visit that page.

Extraction and Signing of JavaScript from HTML
----------------------------------------------
The -J option activates the same functionality provided by the signpages
Perl script.  It will parse a directory of html files, creating archives
of the JavaScript called from the HTML. These archives are then signed and
made into JAR files.

Enhanced Smart Card Support
---------------------------
Certificates that reside on smart cards are displayed when using the -L and
-l options.

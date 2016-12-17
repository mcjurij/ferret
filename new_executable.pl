#!/usr/bin/perl -w
#

my $module;
my $name;

print "What is the module of the new executable (can be the same for entire tree)?\n";
$module = <STDIN>;
chomp $module;

exit 1 if( $module eq "");

print "What is the name of the new executable (muste be unique)?\n";

$name = <STDIN>;
chomp $name;

exit 1 if( $name eq "");

!system( "mkdir -p $name/src" ) or die( "could not create directory" );


print "Enter the libraries that this one is going to use.\n";
print "(empty to finish)\n";
my $sublib;
my $subs;
while( $sublib = <STDIN> )
{
    chomp $sublib;
    last if( $sublib eq "" );
    my $p = "$sublib/project.xml";
    if( -f $p )
    {
        $subs .= "  <sub name=\"$sublib\" />\n";
    }
    else
    {
        print "Project file $p not found\n";
    }
}

my $proj_templ = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<project module=\"$module\" name=\"$name\" target=\"$name\" type=\"executable\">\n$subs</project>\n";

open(my $fh, '>', "$name/project.xml");
print $fh $proj_templ;
close $fh;

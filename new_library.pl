#!/usr/bin/perl -w
# yum install perl-File-Slurp
# aptitude install libfile-slurp-perl

use File::Slurp qw(read_file write_file);
 
my $module;
my $name;

print "What is the module of the new library (can be the same for entire tree)?\n";
$module = <STDIN>;
chomp $module;

exit 1 if( $module eq "");

print "What is the name of the new library (muste be unique)?\n";

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

my $proj_templ = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<project module=\"$module\" name=\"$name\" target=\"$name\" type=\"library\">\n$subs</project>\n";

open(my $fh, '>', "$name/project.xml");
print $fh $proj_templ;
close $fh;

print "Enter the libraries or executables that are going to use this one.\n";
print "(empty to finish)\n";

my $suplib;
while( $suplib = <STDIN> )
{
    chomp $suplib;
    last if( $suplib eq "" );
    
    my $p = "$suplib/project.xml";
    if( -f $p )
    {
        add_subentry( $name, $p);
        print "modified $p\n";
    }
}


sub add_subentry
{
    my $sub_proj = shift;
    my $filename = shift;
    
    my $data = read_file $filename, {binmode => ':utf8'};
    $data =~ s/<\/project>/  \<sub name="$sub_proj" \/>\n<\/project>\n/;
    write_file $filename, {binmode => ':utf8'}, $data;
}

#include <iostream>

#include "output_collector.h"
#include "project_xml_node.h"
#include "file_manager.h"

using namespace std;


string OutputEntry::getOutput() const
{
    return o;
}


string OutputEntry::getStdOut() const
{
    return so;
}


string OutputEntry::getStdOutHtml() const
{
    return soh;
}


string OutputEntry::getErrOut() const
{
    return eo;
}


string OutputEntry::getErrOutHtml() const
{
    return eoh;
}


void OutputEntry::setError( bool err )
{
    error = err;
}
    
// -----------------------------------------------------------------------------

static string replaceEntities( const string &s )
{
    size_t i;
    string r;
    
    for( i = 0; i < s.length(); i++)
    {
        char c = s[i];
        switch( c )
        {
            case '"':
                r.append( "&quot;" );
                break;
            case '&':
                r.append( "&amp;" );
                break;
            case '\'':
                r.append( "&apos;" );
                break;
            case '<':
                r.append( "&lt;" );
                break;
            case '>':
                r.append( "&gt;" );
                break;
                
            default:
                r += c;
        }
    }

    return r;
}


OutputCollector *OutputCollector::theOutputCollector = 0;

OutputCollector *OutputCollector::getTheOutputCollector()
{
    if( theOutputCollector == 0 )
        theOutputCollector = new OutputCollector;
    
    return theOutputCollector;
}


void OutputCollector::append( int file_id, const string &s)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
    
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }
    
    entryMap[ file_id ].append( s );
}


void OutputCollector::appendStd( int file_id, const std::string &s)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
    
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }
    
    entryMap[ file_id ].appendStd( s );
    entryMap[ file_id ].appendStdHtml( "<pre>" + replaceEntities( s ) + "</pre>" );
}


void OutputCollector::appendStdHtml( int file_id, const std::string &s)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
    
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }
    
    entryMap[ file_id ].appendStdHtml( s );
}


void OutputCollector::appendErr( int file_id, const std::string &s)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
    
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }

    if( s.length() > 0  )
    {
        entryMap[ file_id ].appendErr( s );
        entryMap[ file_id ].appendErrHtml( "<pre>" + replaceEntities( s ) + "</pre>" );
    }
}


void OutputCollector::appendErrHtml( int file_id, const std::string &s)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
    
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }
    
    entryMap[ file_id ].appendErrHtml( s );
}


string OutputCollector::getOutput( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return entryMap.at( file_id ).getOutput();
    
    return "";
}


string OutputCollector::getStdOut( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return entryMap.at( file_id ).getStdOut();
    
    return "";
}


string OutputCollector::getErrOut( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return entryMap.at( file_id ).getErrOut();
    
    return "";
}

string OutputCollector::getOutputHtml( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return replaceEntities( entryMap.at( file_id ).getOutput() );
    
    return "";
}


string OutputCollector::getStdOutHtml( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return entryMap.at( file_id ).getStdOutHtml();
    
    return "";
}


string OutputCollector::getErrOutHtml( int file_id ) const
{
    map<int,OutputEntry>::const_iterator it = entryMap.find( file_id );
    
    if( it != entryMap.end() )
        return entryMap.at( file_id ).getErrOutHtml();
    
    return "";
}

void OutputCollector::text( ofstream &os )
{
    size_t i;
    
    for( i = 0; i < file_ids.size(); i++)
        os << getOutput( file_ids[i] );    
}


void OutputCollector::html( ofstream &os )
{
    os << "<!DOCTYPE HTML>\n"
        "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" /><title>ferret works for you</title>"
        "<style>\n"
        "tr.stdout {\n"
        "   background-color : #99FFCC;\n"
        "   border: 1px solid #008000;\n"
        "   font-family:Consolas,Monaco,Lucida Console,Liberation Mono,DejaVu Sans Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "tr.stderr {\n"
        "   color: red;\n"
        "   background-color : #B0B0A5;\n"
        "   border: 3px solid #940000;\n"
        "   font-family:Consolas,Monaco,Lucida Console,Liberation Mono,DejaVu Sans Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "tr.target {\n"
        "   color: sienna;\n"
        "   background-color : #B0B0A5;\n"
        "   border: 3px solid #C40000;\n"
        "   font-family:Consolas,Monaco,Lucida Console,Liberation Mono,DejaVu Sans Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "</style>"
        "</head>\n"
        "<body>\n"
        "<table>\n";
    
    size_t i;
    for( i = 0; i < file_ids.size(); i++)
    {
        map<int,OutputEntry>::const_iterator it = entryMap.find( file_ids[i] );
        const OutputEntry &e = it->second;
        
        if( e.getErrOut().length() > 0 )
        {
            os << "<tr><td>\n";
            os << "<a href=\"#" << file_ids[i] << "\">";
            if( e.hasError() )
                os << "Errors/warnings of #";
            else
                os << "Warnings of #";
            os << file_ids[i] << "</a>\n"
                "</td>\n"
                "</tr>\n";
        }
    }
    
    os << "</table>\n"
        "<table>\n";

    for( i = 0; i < file_ids.size(); i++)
    {
        string so = getStdOutHtml( file_ids[i] );
        if( so.length() > 0 )
        {
            os << "<tr class=\"stdout\">\n<td>";
            os << so;
            os << "</td>\n"
                "</tr>\n";
        }

        string err = getErrOutHtml( file_ids[i] );
        if( err.length() > 0 )
        {
            os << "<tr class=\"stderr\">\n<td id=\"" << file_ids[i] << "\">\n";

            os << err;
            os << "</td>\n"
                "</tr>\n";
        }
    }
    
    os << "</table>\n</body>\n</html>\n";
}


void OutputCollector::resetHtml( std::ofstream &os )
{
     os << "<!DOCTYPE HTML>\n"
         "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" /><title>ferret works for you</title>"
         "<style>\n"
         "p {\n"
         "   font-family:DejaVu Sans Mono,Consolas,Lucida Console,Liberation Mono,Bitstream Vera Sans Mono,monospace;\n"
         "}\n"
         "</style>\n"
         "</head>\n"
         "<body>\n"
         "<p>Please wait...</p>\n"
         "</body>\n</html>\n";
}


void OutputCollector::setError( int file_id, bool err)
{
    map<int,OutputEntry>::iterator it = entryMap.find( file_id );
     
    if( it == entryMap.end() )
    {
        file_ids.push_back( file_id );
        entryMap[ file_id ] = OutputEntry( file_id );
    }
    
    entryMap[ file_id ].setError( err );
}


void OutputCollector::setProjectNodesDir( const string &dir )
{
    projectdir = dir;
}


static map<string,string> dirTargetMap;

static string mkXmlNodeFn( ProjectXmlNode *node )
{
    return node->getModule() + "__" + node->getName() + ".html";
}


void OutputCollector::htmlProjectNodes( ProjectXmlNode *node, FileManager &fileMan, int level)
{
    map<string,string>::const_iterator it = dirTargetMap.find( node->getDir() );
    if( it != dirTargetMap.end() )
        return;
    
    for( size_t i=0; i < node->childNodes.size(); i++)
    {
        ProjectXmlNode *d = node->childNodes[ i ];
        htmlProjectNodes( d, fileMan, level + 1);
    }
    
    string fn;
    if( level==0 )
    {
        fn = "index.html";
        htmlProjectNode( true, node, fileMan, level);
    }
    else
    {
        fn = mkXmlNodeFn( node );
        htmlProjectNode( false, node, fileMan, level);
    }
    
    dirTargetMap[ node->getDir() ] = fn;
}


void OutputCollector::htmlProjectNode( bool index, ProjectXmlNode *node, FileManager &fileMan, int level)
{
    assert( node );
    
    string fn;
    if( !index )
        fn = mkXmlNodeFn( node );
    else
        fn = "index.html";
    ofstream os( (projectdir + "/" + fn).c_str() );
    
    os << "<!DOCTYPE HTML>\n"
        "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n<title>"
       << node->getDir()
       << "</title>\n"
        "<style>\n"
        "p {\n"
        "   font-family:DejaVu Sans Mono,Consolas,Lucida Console,Liberation Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "td {\n"
        "   font-family:DejaVu Sans Mono,Consolas,Lucida Console,Liberation Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";
    
    os << "<p>XML node " << node->getModule() + " / " + node->getName() << " at " << node->getDir();
    if( node->getScriptTarget().length() > 0 )
        os << " with target '" << node->getScriptTarget() << "'"; 
    os << "</p>";
    
    os << "<p>Uses:\n";
    os << "<table>\n";
    for( size_t i=0; i < node->childNodes.size(); i++)
    {
        ProjectXmlNode *d = node->childNodes[ i ];
        os << "<tr><td><a href=\"" << dirTargetMap[ d->getDir() ] << "\">\n";
        os << d->getModule() + " / " + d->getName() + " / " + d->getTarget();
        os << "</a></td></tr>\n";
    }
    
    os << "</table></p>\n\n";
    
    os << "<p>Files:\n<table>\n";
    vector<File> files = node->getFiles();
    for( size_t i=0; i < files.size(); i++)
    {
        const string &fn = files[i].getPath();

        if( fileMan.hasFileName( fn ) )
        {
            file_id_t id = fileMan.getIdForFile( fn );
            os << "<tr><td><a href=\"" << id << ".html\">\n";
            os << fn;
            os << "</a></td></tr>\n";

            htmlProjectFile( index, id, node, fileMan);
        }
        else
        {
            os << "<tr><td>\n";
            os << fn;
            os << " (ignored)</td></tr>\n";
        }
    }
    os << "</table></p>\n";

    if( !index )
        os << "<p><a href=\"index.html\">Jump to root node</a></p>\n";
    os << "</body>\n</html>\n";
}


static map<file_id_t,bool> idDoneMap;

void OutputCollector::htmlProjectFile( bool index, file_id_t id, ProjectXmlNode *node, FileManager &fileMan)
{
    assert( node );
    
    map<file_id_t,bool>::iterator dm = idDoneMap.find( id );
    if( dm != idDoneMap.end() )
        return;
    
    set<file_id_t>::iterator dit;
    stringstream ss;
    ss << id;
    string fn = ss.str() + ".html";
    ofstream os( (projectdir + "/" + fn).c_str() );
    
    string nodefile;
    if( !index )
        nodefile = mkXmlNodeFn( node );
    else
        nodefile = "index.html";
    
    string pfile = fileMan.getFileForId( id );
    bool is_cmd = fileMan.isTargetCommand( id );
    
    os << "<!DOCTYPE HTML>\n"
        "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n<title>"
       << pfile
       << "</title>\n"
        "<style>\n"
        "p {\n"
        "   font-family:DejaVu Sans Mono,Consolas,Lucida Console,Liberation Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "td {\n"
        "   font-family:DejaVu Sans Mono,Consolas,Lucida Console,Liberation Mono,Bitstream Vera Sans Mono,monospace;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";

    os << "<p>File " << pfile << " belonging to node <a href=\"" << nodefile << "\">" << node->getDir() << "</a>";
    if( node->getScriptTarget().length() > 0 )
    {
        os << " with target " << node->getScriptTarget();
    }
    os << "</p>\n";
    if( is_cmd )
        os << "<p>Depends on:</p>\n";
    else
        os << "<p>Uses:</p>\n";
    os << "<table>\n";

    set<file_id_t> deps = fileMan.getDependencies( id );
    for( dit = deps.begin(); dit != deps.end(); dit++)
    {
        file_id_t up = *dit;

        string fn = fileMan.getFileForId( up );
        os << "<tr><td><a href=\"" << up << ".html\">\n";
        os << fn;
        os << "</a></td></tr>\n";
    }
    
    os << "</table>\n";
    if( is_cmd )
        os << "<p>Prerequisite for:</p>\n";
    else
        os << "<p>Used by:</p>\n";
    
    os << "<table>\n";

    set<file_id_t> down_deps = fileMan.prerequisiteFor( id );
    for( dit = down_deps.begin(); dit != down_deps.end(); dit++)
    {
        file_id_t down = *dit;

        string fn = fileMan.getFileForId( down );
        os << "<tr><td><a href=\"" << down << ".html\">\n";
        os << fn;
        os << "</a></td></tr>\n";

        htmlProjectFile( index, down, fileMan.getXmlNodeFor( down ), fileMan);
    }
    
    os << "</table>\n";
    os << "</body>\n</html>\n";

    idDoneMap[ id ] = true;
}

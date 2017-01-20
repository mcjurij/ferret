#include <iostream>

#include "output_collector.h"
#include "project_xml_node.h"
#include "file_manager.h"

using namespace std;


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


OutputCollector::OutputCollector()
{
#ifdef  USE_CURSES
    screen = 0;
#endif
}


OutputCollector *OutputCollector::getTheOutputCollector()
{
    if( theOutputCollector == 0 )
        theOutputCollector = new OutputCollector;
    
    return theOutputCollector;
}


unsigned int OutputCollector::createJob( const string &fn, const vector<string> &args, int file_id)
{
    return jobStorer.createJob( fn, args, file_id);
}


unsigned int OutputCollector::createJob( const string &psuedo_fn )
{
    return jobStorer.createJob( psuedo_fn, vector<string>(), -1);
}


void OutputCollector::cursesEnable( unsigned int parallel )
{
#ifdef  USE_CURSES
    if( !screen )
        screen = new CursesScreen( parallel );
#endif
}


void OutputCollector::cursesDisable()
{
#ifdef  USE_CURSES
    if( screen )
    {
        delete screen;
        screen = 0;
    }
#endif
}


void OutputCollector::cursesEventHandler()
{
#ifdef  USE_CURSES
    if( screen )
        screen->eventHandler();
#endif
}


void OutputCollector::cursesUpdate()
{
#ifdef  USE_CURSES
    if( screen )
        screen->update();
#endif
}


void OutputCollector::appendJobOut( unsigned int job_id, const string &s)
{
    jobStorer.append( job_id, s);
}


void OutputCollector::appendJobStd( unsigned int job_id, const string &s)
{
    jobStorer.appendStd( job_id, s);
}


void OutputCollector::appendJobErr( unsigned int job_id, const string &s)
{
    if( s.length() > 0  )
        jobStorer.appendErr( job_id, s);
}


bool OutputCollector::hasJobOut( unsigned int job_id ) const
{
    return jobStorer.hasOutput( job_id );
}


void OutputCollector::cursesAppend( unsigned int job_id, const string &s)
{
#ifdef  USE_CURSES
    if( screen )
    {
        screen->jobAppend( job_id, s);
        screen->setUpdateBottomNeeded( true );
    }
#endif
}


void OutputCollector::cursesEnd( unsigned int job_id )
{
#ifdef  USE_CURSES
    if( screen )
    {
        screen->jobEnd( job_id );
        screen->setUpdateBottomNeeded( true );
    }
#endif
}


void OutputCollector::cursesSetTopLine( int y, const string &l)
{
#ifdef  USE_CURSES
    if( screen )
    {
        screen->setTopLine( y, l);
        screen->setUpdateTopNeeded( true );
    }
#endif
}


string OutputCollector::getJobOut( unsigned int job_id ) const
{
    return jobStorer.getOutput( job_id );
}


string OutputCollector::getJobStd( unsigned int job_id ) const
{
    return jobStorer.getStdOut( job_id );
}


string OutputCollector::getJobErr( unsigned int job_id ) const
{
    return jobStorer.getErrOut( job_id );
}


string OutputCollector::getJobOutHtml( unsigned int job_id ) const
{
    return "<pre>" + replaceEntities( jobStorer.getOutput( job_id ) ) + "</pre>";
}


string OutputCollector::getJobStdHtml( unsigned int job_id  ) const
{
    return "<pre>" + replaceEntities( jobStorer.getStdOut( job_id ) ) + "</pre>";
}


string OutputCollector::getJobErrHtml( unsigned int job_id ) const
{
    return "<pre>" + replaceEntities( jobStorer.getErrOut( job_id ) ) + "</pre>";
}


void OutputCollector::text( ofstream &os )
{
    size_t i;
    
    for( i = 0; i < jobStorer.getSize(); i++)
        os << jobStorer.getOutput( i+1 );
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
    for( i = 0; i < jobStorer.getSize(); i++)
    {
        unsigned int job_id = i + 1;
        
        if( getJobErr( job_id ).length() > 0 )
        {
            os << "<tr><td>\n";
            os << "<a href=\"#" << getJobFileId( job_id ) << "\">";
            if( hasJobError( job_id ) )
                os << "Errors/warnings of #";
            else
                os << "Warnings of #";
            os << getJobFileId( job_id ) << "</a>\n"
                "</td>\n</tr>\n";
        }
    }
    
    os << "</table>\n"
        "<table>\n";

    for( i = 0; i < jobStorer.getSize(); i++)
    {
        unsigned int job_id = i + 1;

        os << "<!-- job id: " << job_id << " begins -->\n";
        os << "<!-- job state is ";
        os << jobStorer.getStateAsString( job_id );
        os << " -->\n";
        
        os << "<!-- file is ";
        os << jobStorer.getFileName( job_id );
        os << " -->\n";
        
        string so = getJobStdHtml( job_id );
        if( getJobStd( job_id ).length() > 0 )
        {
            os << "<tr class=\"stdout\">\n<td>";
            os << so;
            os << "</td>\n"
                "</tr>\n";
        }
        
        string err = getJobErrHtml( job_id );
        if( getJobErr( job_id ).length() > 0 )
        {
            os << "<tr class=\"stderr\">\n<td id=\"" << getJobFileId( job_id ) << "\">\n";
            os << err;
            os << "</td>\n"
                "</tr>\n";
        }
        
        string msg = getHtml( job_id );
        if( msg.length() > 0 )
        {
            os << "<tr>\n<td>";
            os << msg;
            os << "</td>\n"
                "</tr>\n";
        }
        os << "<!-- job id: " << job_id << " ends -->\n\n";
    }

    os << "</table>\n\n" << freeHtml
       << "\n</body>\n</html>\n";
}


void OutputCollector::addHtml( unsigned int job_id, const string &s)
{
    //if( job_id == (unsigned int)-1 )
    if( job_id == -1 )
        return;
    
    assert( job_id >= 1 && job_id <= jobStorer.getSize() );
    
    map<unsigned int,string>::iterator it = jobHtml.find( job_id );
    
    if( it != jobHtml.end() )
        it->second.append( s );
    else
        jobHtml[ job_id ] = s;
}


string OutputCollector::getHtml( unsigned int job_id ) const
{
    map<unsigned int,string>::const_iterator it = jobHtml.find( job_id );

    if( it != jobHtml.end() )
        it->second;
    
    return "";
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


void OutputCollector::setJobError( unsigned int job_id, bool err)
{
    jobStorer.setError( job_id, err);
}


bool OutputCollector::hasJobError( unsigned int job_id )
{
    return jobStorer.hasError( job_id );
}


int  OutputCollector::getJobFileId( unsigned int job_id )
{
    return jobStorer.getFileId( job_id );
}


void OutputCollector::clear()
{
    jobStorer.clear();
    jobHtml.clear();
    freeHtml = "";
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

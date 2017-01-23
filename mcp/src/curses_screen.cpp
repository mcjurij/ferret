#ifdef   USE_CURSES

#include "curses_screen.h"
#include "output_collector.h"

using namespace std;


void CursesScreen::JobOutput::appendTo( vector<string> &lines, int from)
{
    int i;
    for( i = from; i < (int)cursesLines.size(); i++)
        lines.push_back( cursesLines[i] );
}


void CursesScreen::JobOutput::setError( bool e )
{
    if( e )
        show = FAILED;
    else if( show == HAS_STDERR )
        show = WARNINGS;
    else
        show = ALL;
}


CursesScreen::CursesScreen( unsigned int parallel )
    : p(parallel), menu(0)
{
    WINDOW *mainwin = initscr();
    clear();
    noecho();
    cbreak();     // line buffering disabled. pass on everything
    wtimeout( mainwin, 0 );
    //leaveok( mainwin, TRUE);
    //clearok( mainwin, TRUE);
    //nodelay( mainwin, TRUE);
    
    top = bottom = 0;

    initScreen();

    lastSel.insert( 1 );
    lastSel.insert( 2 );
    
    topLines.resize( getTopMaxY() );
}


string CursesScreen::JobOutput::getFirstLine() const
{
    if( cursesLines.size() > 0 )
        return cursesLines[0];
    else
        return "waiting for output...";
}


CursesScreen::~CursesScreen()
{
    if( menu )
        delete menu;
    
    delwin( top );

    if( bottom )
        delwin( bottom );
    
    endwin();
}


void CursesScreen::initScreen()
{
    getmaxyx( stdscr, maxY, maxX);
    // keypad( stdscr, TRUE);
    
    initTop( p + 2 );
    initBottom( maxY-(p + 2) );
}


void CursesScreen::initTop( int height )
{
    if( top )
    {
        werase( top );
        wrefresh( top );
        delwin( top );
    }
    
    top = newwin( height, maxX, 0, 0);
    //leaveok( top, TRUE);
    //clearok( top, FALSE);
    //nodelay( top, TRUE);
    wtimeout( top, 0);
    
    mvwhline( top, height-1, 0, ACS_HLINE, maxX);
    wrefresh( top );
    
    getmaxyx( top, tmaxY, tmaxX);
    updateTopNeeded = true;
}


void CursesScreen::initBottom( int height )
{
    if( bottom )
    {
        werase( bottom );
        wrefresh( bottom );
        delwin( bottom );
    }
    
    bottom = newwin( height, maxX, maxY-height, 0);
    //leaveok( bottom, TRUE);
    //clearok( bottom, FALSE);
    //nodelay( bottom, TRUE);
    wtimeout( bottom, 0 );
    
    getmaxyx( bottom, bmaxY, bmaxX);
    updateBottomNeeded = true;
}


void CursesScreen::setShowTop( bool b )
{
    if( b )
    {
        initTop( p + 2 );
        initBottom( maxY-(p + 2) );
    }
    else if( top )
    {
        delwin( top );
        top = 0;

        initBottom( maxY );
    }
}


void CursesScreen::update()
{
    int i;
    
    if( top && updateTopNeeded )
    {
        cursesTop( tmaxY );
        werase( top );
        
        for( i = 0; i < (int)topLines.size(); i++)
            mvwaddnstr( top, i, 0, topLines[i].c_str(), tmaxX);
        
        mvwhline( top, p+1, 0, ACS_HLINE, maxX);

        wnoutrefresh( top );
        updateTopNeeded = false;
    }
    
    if( bottom && updateBottomNeeded )
    {
        cursesBottom( bmaxY );
        werase( bottom );
        
        int no_lines = (int)bottomLines.size();
        
        int offs = 0;
        if( no_lines < bmaxY )
            offs = bmaxY - no_lines;
        
        for( i = 0; i < no_lines; i++)
            mvwaddnstr( bottom, offs + i, 0, bottomLines[i].c_str(), bmaxX);
        
        wnoutrefresh( bottom );
        updateBottomNeeded = false;
    }
    
    if( menu )
        menu->update();
    
    doupdate();     // do all refreshs here only at a single point in time
}


void CursesScreen::eventHandler()
{
    if( menu )
    {
        if( !menu->eventHandler() )
            setShowMenu( false );
    }
    else
    {
        int c = getch();       // do not block here
        if( c == 'm' )
            if( !menu )
                setShowMenu( true );
    }
}


void CursesScreen::setShowMenu( bool b )
{
    if( b )
    {
        if( menu )
            delete menu;
        
        menu = new CursesMenu( 8, 5, true);
        menu->addOption( 1, "Show top", 0, getLastSelState( 1 ));
        menu->addOption( 2, "All", 1, getLastSelState( 2 ));
        menu->addOption( 3, "Errors/Warnings", 1, getLastSelState( 3 ));
        menu->addOption( 4, "Errors", 1, getLastSelState( 4 ));
        menu->addOption( -1, "Exit", -1);
        menu->setShow( true );
    }
    else if( menu )
    {
        menu->setShow( false );
        lastSel = menu->getSelectionSet();
        delete menu;
        menu = 0;

        setShowTop( getLastSelState( 1 ) );
    }
}


void CursesScreen::setTopLine( int y, const std::string &l)
{
    if( y < (int)topLines.size() )
        topLines[ y ] = l;
}


void CursesScreen::topShowJob( unsigned int job_id )
{
    topShowJobs.insert( job_id );
    updateTopNeeded = true;
}


void CursesScreen::jobAppend( unsigned int job_id, const string &s)
{
    size_t ji = job_id - 1;

    if( ji >= jobs.size() )
        jobs.resize( ji + 1 );

    JobOutput &j = jobs[ ji ];
    string line = j.cursesLast;
    size_t i;
    
    for( i = 0; i < s.length(); i++)
    {
        if( s[i] != '\n' )
            line += s[i];
        else
        {
            j.cursesLines.push_back( line );
            line = "";
        }
    }

    j.cursesLast = line;
    updateTopNeeded = true;
}


void CursesScreen::jobEnd( unsigned int job_id )
{
    size_t ji = job_id - 1;

    if( ji >= jobs.size() )
        jobs.resize( ji + 1 );

    JobOutput &j = jobs[ ji ];
    if( j.cursesLast.length() > 0 )
    {
        j.cursesLines.push_back( j.cursesLast );
        j.cursesLast = "";
    }

    topShowJobs.erase( job_id );
    updateTopNeeded = true;
}


void CursesScreen::jobStderr( unsigned int job_id )
{
    size_t ji = job_id - 1;

    if( ji >= jobs.size() )
        jobs.resize( ji + 1 );

    JobOutput &j = jobs[ ji ];
    j.setStderr();

    if( showErrWarnSet.find( job_id ) == showErrWarnSet.end() )
    {
        showErrWarnSet.insert( job_id );
        showErrWarn.push_back( job_id );
        updateBottomNeeded = true;
    }
}


void CursesScreen::jobSetError( unsigned int job_id, bool e)
{
    size_t ji = job_id - 1;
    JobOutput &j = jobs.at( ji );

    j.setError( e );

    if( e && showErrSet.find( job_id ) == showErrSet.end() )
    {
        showErrSet.insert( job_id );
        showErr.push_back( job_id );
        updateBottomNeeded = true;
    }
}


void CursesScreen::jobAppendTo( unsigned int job_id, int from)
{
    size_t i = job_id - 1;

    if( i >= jobs.size() )
        jobs.resize( i + 1 );
    
    JobOutput &j = jobs[ i ];
    
    j.appendTo( bottomLines, from);
}


bool CursesScreen::getLastSelState( int idx )
{
    return lastSel.find( idx ) != lastSel.end();
}


void CursesScreen::cursesTop( int last )
{
    set<unsigned int>::const_iterator it = topShowJobs.begin();
    int y = 1;
    for( ; it != topShowJobs.end() && y < (int)topLines.size(); it++, y++)
    {
        unsigned int job_id = *it;

        if( job_id-1 < jobs.size() )
        {
            const JobOutput &j = jobs[ job_id-1 ];
            setTopLine( y, j.getFirstLine());
        }
        else
            setTopLine( y, "waiting for first line...");
    }
}


void CursesScreen::cursesBottom( int last )
{
    int i;
    bottomLines.clear();
    if( jobs.size() == 0 )
        return;

    if( getLastSelState( 3 ) )
        cursesAssembleErrWarn( last, showErrWarn);
    else if( getLastSelState( 4 ) )
        cursesAssembleErrWarn( last, showErr);
    else
    {
        int k = 0, idx=-1;
        int first_line = 0;
        for( i = 0; i < (int)jobs.size(); i++)
        {
            idx = (jobs.size()-1) - i;
            
            k += jobs[ idx ].numLines();
            
            if( k > last )
            {
                first_line = k - last;
                break;
            }
        }
        
        for( i = idx; i < (int)jobs.size(); i++)
        {
            jobAppendTo( i+1, first_line);
            first_line = 0;
        }
    }
}


void CursesScreen::cursesAssembleErrWarn( int last, vector<unsigned int> &show)
{
    int i;
    
    if( show.size() == 0 )
        return;
    
    int k = 0;
    int start_idx = 0;
    int first_line = 0;
    for( i = 0; i < (int)show.size(); i++)
    {
        start_idx = (show.size()-1) - i;
        unsigned int job_id = show[ start_idx ];
        int idx = job_id - 1;
        
        k += jobs[ idx ].numLines();
        
        if( k > last )
        {
            first_line = k - last;
            break;
        }
    }
    
    for( i = start_idx; i < (int)show.size(); i++)
    {
        unsigned int job_id = show[ i ];
        jobAppendTo( job_id, first_line);
        first_line = 0;
    }
}

#endif

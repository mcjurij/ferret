
#ifdef   USE_CURSES



#include "curses_screen.h"
#include "output_collector.h"

using namespace std;


CursesScreen::CursesScreen( unsigned int parallel )
    : p(parallel), menu(0)
{
    WINDOW *mainwin = initscr();
    clear();
    noecho();
    cbreak();   // Line buffering disabled. pass on everything
    wtimeout( mainwin, 0 );
    //nodelay( mainwin, TRUE);
    
    top = bottom = 0;

    initScreen();
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
    if( top )
    {
        werase( top );
        wrefresh( top );
        delwin( top );
    }
  
    
    getmaxyx( stdscr, maxY, maxX);
    // keypad( stdscr, TRUE);
    
    top = newwin( p+2, maxX, 0, 0);
    leaveok( top, TRUE);
    clearok( top, FALSE);
    //nodelay( top, TRUE);
    wtimeout( top, 0);
    
    mvwhline( top, p+1, 0, ACS_HLINE, maxX);
    wrefresh( top );
    
    getmaxyx( top, tmaxY, tmaxX);
    
    initBottom();
}


void CursesScreen::initBottom()
{
    if( bottom )
    {
        werase( bottom );
        wrefresh( bottom );
        delwin( bottom );
    }
    
    bottom = newwin( maxY-(p+2), maxX, p+2, 0);
    leaveok( bottom, TRUE);
    clearok( bottom, FALSE);
    //nodelay( bottom, TRUE);
    wtimeout( bottom, 0 );
    
    getmaxyx( bottom, bmaxY, bmaxX);
}


void CursesScreen::update()
{
    int i;
    
    if( top )
    {
        vector<string> top_lines = OutputCollector::getTheOutputCollector()->cursesTop( tmaxY );
        //wclear( top );
        werase( top );
        
        for( i = 0; i < (int)top_lines.size(); i++)
            mvwaddnstr( top, i, 0, top_lines[i].c_str(), tmaxX);
        
        mvwhline( top, p+1, 0, ACS_HLINE, maxX);
    }
    
    if( bottom )
    {
        vector<string> tail_lines = OutputCollector::getTheOutputCollector()->cursesBottom( bmaxY );
        //wclear( bottom );
        werase( bottom );
        
        int no_lines = (int)tail_lines.size();
        
        int offs = 0;
        if( no_lines < bmaxY )
            offs = bmaxY - no_lines;
        
        for( i = 0; i < no_lines; i++)
        {
            mvwaddnstr( bottom, offs + i, 0, tail_lines[i].c_str(), bmaxX);
        }
    }

    if( top )
        wrefresh( top );
    if( bottom )
        wrefresh( bottom );
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
        if( bottom )
        {
            delwin( bottom );
            bottom = 0;
        }
        
        if( menu )
            delete menu;
        
        menu = new CursesMenu( 8, 5, true);
        menu->addOption( "Show top", 0, true);
        menu->addOption( "Choice 2", 0, false);
        menu->addOption( "Choice 3", 0, false);
        menu->addOption( "Exit", -1);
        menu->setShow( true );
    }
    else if( menu )
    {
        menu->setShow( false );
        delete menu;
        menu = 0;

        initBottom();
    }
}

#endif

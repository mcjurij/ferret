
#ifdef   USE_CURSES



#include "curses_screen.h"
#include "output_collector.h"

using namespace std;


CursesScreen::CursesScreen( unsigned int parallel )
    : p(parallel)
{
    initscr();
    clear();
    noecho();
    cbreak();   // Line buffering disabled. pass on everything

    top = bottom = 0;

    initScreen();
}


CursesScreen::~CursesScreen()
{
    delwin( top );
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
    if( bottom )
    {
        werase( bottom );
        wrefresh( bottom );
        delwin( bottom );
    }
    
    getmaxyx( stdscr, maxY, maxX);
    // keypad( stdscr, TRUE);
    
    top = newwin( p+2, maxX, 0, 0);
    bottom = newwin( maxY-(p+2), maxX, p+2, 0);
    leaveok( bottom, TRUE);
    clearok( bottom, FALSE);
    
    mvwhline( top, p+1, 0, ACS_HLINE, maxX);
    wrefresh( top );
    
    getmaxyx( top, tmaxY, tmaxX);
    getmaxyx( bottom, bmaxY, bmaxX);
}


void CursesScreen::update()
{
    vector<string> top_lines = OutputCollector::getTheOutputCollector()->cursesTop( tmaxY );
    //wclear( bottom );
    werase( bottom );
    
    int i;
    for( i = 0; i < (int)top_lines.size(); i++)
        mvwaddnstr( top, i, 0, top_lines[i].c_str(), tmaxX);
    
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
    
    wrefresh( top );
    wrefresh( bottom );
}
    
#endif

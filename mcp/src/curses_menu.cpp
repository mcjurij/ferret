#ifdef  USE_CURSES

#include <cassert>
#include <curses.h>

#include "curses_menu.h"

using namespace std;


CursesMenu::CursesMenu( int y, int x, bool poll)
    : y(y), x(x), win(0), poll(poll), maxOptLen(0)
{
    highlight = 1;
}


CursesMenu::~CursesMenu()
{
    if( win )
        delwin( win );
}


void CursesMenu::addOption( const string &s, int group, bool sel)    // group == 0 => belongs to no group
{
    if( s.length() > 0 )
        options.push_back( Option( s, sel, group) );
}


void CursesMenu::setShow( bool b )
{
    if( b )
    {
        if( win )
            delwin( win );

        maxOptLen = 0;
        size_t i;
        for( i = 0; i < options.size(); i++)
            if( (int)options[i].opts.length() > maxOptLen )
                maxOptLen = (int)options[i].opts.length();
        
        for( i = 0; i < options.size(); i++)
            if( (int)options[i].opts.length() < maxOptLen )
                options[i].displ = options[i].opts + string( maxOptLen - options[i].opts.length(), ' ');
            else
                options[i].displ = options[i].opts;
        
        win = newwin( (int)options.size()+2, maxOptLen+8, y, x);
        keypad( win, TRUE);
        leaveok( win, TRUE);
          
        if( poll )
            //nodelay( win, TRUE);
            wtimeout( win, 1);
        draw();
    }
    else if( win )
    {
        wclear( win );
        wrefresh( win );
        
        delwin( win );
    }
}


void CursesMenu::update()
{
    if( win )
    {
        highlightChoice();
        wrefresh( win );
    }
}


bool CursesMenu::eventLoop()
{
    int n_choices = (int)options.size();
    bool done = false;
    bool close = false;
    
    do {
        int g, c = wgetch( win );
        if( c == ERR )      // in poll mode, ERR is returned when no input available
            break;
        
        bool pos_changed = false;
        switch( c )
        {
            case KEY_UP:
                if( highlight == 1 )
                    highlight = n_choices;
                else
                    highlight--;
                pos_changed = true;
                break;
            case KEY_DOWN:
                if( highlight == n_choices )
                    highlight = 1;
                else
                    highlight++;
                pos_changed = true;
                break;
            case KEY_LEFT:
            case KEY_RIGHT:
                break;
            case 10:
            case KEY_ENTER:
                g = options[ highlight - 1 ].group;
                if( g == -1 )
                {
                    done = true;
                    close = true;
                }
                break;
                
            case ' ':
                select( highlight );
                pos_changed = true;
                break;
                
            default:
                // cont = true;
                break;
        }
        
        if( pos_changed )
            highlightChoice();
    }
    while( !done );

    return !close;
}


bool CursesMenu::eventHandler()
{
    // make sure you have called ctor with poll==true
    assert( poll );
    
    return eventLoop();
}


void CursesMenu::draw()
{
    box( win, 0, 0);
    
    highlightChoice();
}


set<int> CursesMenu::getSelectionSet() const
{
    int i;
    set<int> s;
    
    for( i = 0; i < (int)options.size(); i++)
    {
        if( options[i].selected && options[i].group >= 0 )
            s.insert( i );
    }
    
    return s;
}


void CursesMenu::highlightChoice()
{
    int i;    
    
    for( i = 0; i < (int)options.size(); i++)
    {
        if( highlight == i + 1 )   // high light choice 
            wattron( win, A_REVERSE);
        
        mvwaddstr( win, 1+i, 2, options[i].displ.c_str());
        
        if( options[i].group != -1 )
            mvwprintw( win, 1+i, 2+maxOptLen, " [%c]", options[i].selected ? '*' : ' ');
        else
            mvwaddstr( win, 1+i, 2+maxOptLen, "    ");
        
        if( highlight == i + 1 )
            wattroff( win, A_REVERSE);
    }
}


void CursesMenu::select( int highlight )
{
    int i, g = 0;
    g = options[ highlight - 1 ].group;

    if( g == 0 && options[ highlight - 1 ].selected )
        options[ highlight - 1 ].selected = false;
    else
        options[ highlight - 1 ].selected = true;
    
    if( g > 0 )
    {
        for( i = 0; i < (int)options.size(); i++)
        {
            if( highlight != i + 1 && g == options[i].group )
                options[i].selected = false;
        }
    }
}

#endif

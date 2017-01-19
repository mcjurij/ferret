#ifndef FERRET_CURSES_SCREEN_H_
#define FERRET_CURSES_SCREEN_H_

#ifdef  USE_CURSES

#include <curses.h>

#include "curses_menu.h"


class CursesScreen {

private:
    CursesScreen();

    struct JobOutput {
        std::vector<std::string> cursesLines;
        std::string cursesLast;
        
        int numLines() const
        { return (int)cursesLines.size(); }
        
        void appendTo( std::vector<std::string> &lines, int from)
        {
            int i;
            for( i = from; i < (int)cursesLines.size(); i++)
                lines.push_back( cursesLines[i] );
        }
    };
    
public:
    CursesScreen( unsigned int parallel );
    ~CursesScreen();
    
    void initScreen();
    void initTop( int height );
    void initBottom( int height );
    
    void setShowTop( bool b );
    
    void setUpdateTopNeeded( bool b )
    { updateTopNeeded = b; }
    void setUpdateBottomNeeded( bool b )
    { updateBottomNeeded = b; }
    
    void update();
    
    void eventHandler();
    void setShowMenu( bool b );
    
    int getTopMaxY() const
    { return tmaxY-1; }
    
    void setTopLine( int y, const std::string &l);
    
    void jobAppend( unsigned int job_id, const std::string &s);
    void jobEnd( unsigned int job_id );
    void jobAppendTo( unsigned int job_id, std::vector<std::string> &lines, int from);
    
private:
    bool getLastSelState( int idx );
    
    std::vector<std::string> cursesTop( int last );
    std::vector<std::string> cursesBottom( int last );
    
private:
    int maxX, maxY;
    unsigned int p;

    WINDOW *top, *bottom;
    bool updateTopNeeded, updateBottomNeeded;
    
    int tmaxX, tmaxY;
    int bmaxY, bmaxX;

    CursesMenu *menu;
    std::set<int> lastSel;
    
    std::vector<std::string> topLines;
    std::vector<JobOutput> jobs;
};


#endif
#endif

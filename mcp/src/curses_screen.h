#ifndef FERRET_CURSES_SCREEN_H_
#define FERRET_CURSES_SCREEN_H_

#ifdef  USE_CURSES

#include <curses.h>

// class OutputCollector;

class CursesScreen {

private:
    CursesScreen();
    
public:
    CursesScreen( unsigned int parallel );
    ~CursesScreen();
    
    void initScreen();
    
    void update();

    int getTopY() const
    { return tmaxY-1; }
    
private:
    int maxX, maxY;
    unsigned int p;

    WINDOW *top, *bottom;

    int tmaxX, tmaxY;
    int bmaxY, bmaxX;
    
};


#endif
#endif

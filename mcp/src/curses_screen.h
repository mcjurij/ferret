#ifndef FERRET_CURSES_SCREEN_H_
#define FERRET_CURSES_SCREEN_H_

#ifdef  USE_CURSES

#include <set>
#include <curses.h>

#include "curses_menu.h"


class CursesScreen {

private:
    CursesScreen();
    
    struct JobOutput {
        typedef enum { UNDECIDED, ALL, HAS_STDERR, WARNINGS, FAILED} show_t;

        JobOutput()
            : show( UNDECIDED )
        {}
        
        int numLines() const
        { return (int)cursesLines.size(); }
        
        void appendTo( std::vector<std::string> &lines, int from);
        
        void setStderr()
        { show = HAS_STDERR; }
        
        void setError( bool e );
        
        bool showWhenErrorsWarnings() const
        {
            return( show == UNDECIDED || show == HAS_STDERR || show == WARNINGS || show == FAILED );
        }
        
        bool showWhenErrors() const
        {
            return( show == UNDECIDED || show == HAS_STDERR || show == FAILED );
        }

        std::string getFirstLine() const;
        
        show_t show;
        std::vector<std::string> cursesLines;
        std::string cursesLast;
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
    void topShowJob( unsigned int job_id );
    
    void jobAppend( unsigned int job_id, const std::string &s);
    void jobEnd( unsigned int job_id );
    void jobStderr( unsigned int job_id );
    void jobSetError( unsigned int job_id, bool e);
    
private:
    void jobAppendTo( unsigned int job_id, int from);
    
    bool getLastSelState( int idx );
    
    void cursesTop( int last );
    void cursesBottom( int last );
    
private:
    int maxX, maxY;
    unsigned int p;

    WINDOW *top, *bottom;
    bool updateTopNeeded, updateBottomNeeded;
    
    int tmaxX, tmaxY;
    int bmaxY, bmaxX;

    CursesMenu *menu;
    std::set<int> lastSel;

    std::set<unsigned int> topShowJobs;
    std::vector<JobOutput> jobs;
    std::vector<std::string> topLines;
    std::vector<std::string> bottomLines;
};


#endif
#endif

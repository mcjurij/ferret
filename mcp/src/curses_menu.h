#ifndef FERRET_CURSES_MENU_H_
#define FERRET_CURSES_MENU_H_

#ifdef  USE_CURSES

#include <vector>
#include <string>
#include <set>

class CursesMenu {

    struct Option {
        Option( const std::string &s, bool sel, int group)
            : opts(s), selected(sel), group(group)
        {}
        
        std::string  opts, displ;
        bool    selected;
        int     group;
    };

private:
    CursesMenu();
    
public:
    CursesMenu( int y, int x, bool poll = false);
    
    ~CursesMenu();
    
    void addOption( const std::string &s, int group = 0, bool sel = false); // group == 0 => belongs to no group
    void setShow( bool b );
    
    void eventLoop();   // will not block when poll == true
    
    std::set<int> getSelectionSet() const;
    
private: 
    void draw();
    void highlightChoice( int highlight );
    void select( int highlight );
    
private:
    int y,x;
    WINDOW *win;
    bool poll;
    std::vector<Option> options;
    int maxOptLen;
};

#endif
#endif

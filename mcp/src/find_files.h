#ifndef FERRET_FIND_FILES_H_
#define FERRET_FIND_FILES_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string>
#include <map>
#include <vector>
#include <set>


class File {
public:
    File()
    {last_modification.tv_sec = 0; last_modification.tv_nsec = 0;}
    
    File( const char *path, const char *d, const char *bn, const struct stat *fst)
        : path(path), dir(d), basename(bn)
    {
        last_modification.tv_sec = fst->st_mtim.tv_sec;
        last_modification.tv_nsec = fst->st_mtim.tv_nsec;  // see http://man7.org/linux/man-pages/man2/stat.2.html
    }
    
    explicit File( const std::string &path, const std::string &dir, const std::string &bn, struct stat *fst)
        : path(path), dir(dir), basename(bn)
    {
        last_modification.tv_sec = fst->st_mtim.tv_sec;
        last_modification.tv_nsec = fst->st_mtim.tv_nsec;
    }
    
    explicit File( const std::string &path, struct stat *fst)
        : path(path), dir(path)
    {
        last_modification.tv_sec = fst->st_mtim.tv_sec;
        last_modification.tv_nsec = fst->st_mtim.tv_nsec;
    }
    
    bool isOlderThan( const File& other ) const;    
    bool isNewerThan( const File& other ) const;
    
    std::string getPath() const
    { return path; }
    
    std::string getDirectory() const
    { return dir; }
    
    std::string getBasename() const
    { return basename; }

    std::string getDepFileName() const;
    
    std::string extension() const;
    std::string woExtension() const;

    long long getTimeMs() const;
    
private:
    std::string path;
    std::string dir;
    std::string basename;
    struct timespec last_modification;
};


class FindFiles {
    
private:
    static
    int handle_entry( const char *fpath, const struct stat *sb,
                      int tflag, struct FTW *ftwbuf);
public:
    FindFiles()
    {}
    
    void traverse( const std::string &start_dir, bool deep);
    void appendTraverse( const std::string &start_dir, bool deep);
    
    std::vector<File> getFiles() const
    {
        return files;
    }
    
    std::vector<File> getSourceFiles() const;
    std::vector<File> getHeaderFiles() const;
    
    static bool exists( const std::string &path );
    static File getCachedFile( const std::string &path );
    static File getUncachedFile( const std::string &path );
    static bool existsUncached( const std::string &path );
    
    static void clearCache();

private:
    std::vector<File> readDirectory( const std::string &dir );
    
private:
    static std::map<std::string,File> allFiles;
    static std::vector<File> ftwFiles;
    std::vector<File> files;
    static std::set<std::string> noexistingFiles;
};


class QueryFiles  // only for the engine
{

public:
    QueryFiles();
    
    bool exists( const std::string &path );
    File getFile( const std::string &path );
    
private:
};


#endif

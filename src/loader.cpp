#include <twig/loader.hpp>
#include <twig/exceptions.hpp>

#include <fstream>
#include <sstream>

using namespace std ;

namespace twig {


FileSystemTemplateLoader::FileSystemTemplateLoader(const std::initializer_list<string> &root_folders, const string &suffix):
    root_folders_(root_folders), suffix_(suffix) {
}

string FileSystemTemplateLoader::load(const string &key) {

    for ( const string &r: root_folders_ ) {

        string p(r) ;

        if ( key.rfind(suffix_) != string::npos ) p += '/' + key ;
        else p += '/' + key + suffix_;

        ifstream in(p) ;

        if ( in ) {
            return static_cast<stringstream const&>(stringstream() << in.rdbuf()).str() ;
        }
    }

    throw TemplateLoadException("Cannot find template: " + key) ;
}

}

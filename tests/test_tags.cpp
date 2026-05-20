#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>

using namespace twig;
using namespace std ;

class TagTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};

// Test Variant construction and type detection
TEST_F(TagTest, ForBlockArray) {
  TemplateRenderer rdr(nullptr) ;

  const string loop1  =  R"({% for user in users %}{{loop.index}}. {{ user.name }} {% else %}No users have been found.{% endfor %})";
 
  Variant::Object ctx1{
        {"users", Variant::Array{
            Variant::Object{{"name", "Alice"}},
            Variant::Object{{"name", "Bob"}},
            Variant::Object{{"name", "Charlie"}}
        }}
    } ;

    try {
        string output =  rdr.renderString(loop1, ctx1) ;
        EXPECT_STREQ(output.c_str(), "1. Alice 2. Bob 3. Charlie ") ;
      
        output =  rdr.renderString(loop1, {}) ;
        EXPECT_STREQ(output.c_str(), "No users have been found.") ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(TagTest, ForBlocDictionary) {
    TemplateRenderer rdr(nullptr) ;

    const string loop2 = R"(<ul>{% for key, user in users %}<li>{{ key }} {{ user|e }}</li>{% endfor %}</ul>)";
    
     Variant::Object ctx2{
        {"users", Variant::Object{
            {"Alice", "Cooper"},
            {"Bob", "Smith"},
            {"Charlie", "Brown"}    
        }}
    } ;

    try {
        string output =  rdr.renderString(loop2, ctx2) ;
        EXPECT_STREQ(output.c_str(), "<ul><li>Alice Cooper</li><li>Bob Smith</li><li>Charlie Brown</li></ul>") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, ForBlockSequence) {
    TemplateRenderer rdr(nullptr) ;

    const string loop1 = R"({% for i in 'a'|upper..'d'|upper %}* {{ i }} {% endfor %})";
    const string loop2 = R"({% for i in range(0, 3, 2) %}* {{ i }} {% endfor %})";
    try {
        string output =  rdr.renderString(loop1, {}) ;
        EXPECT_STREQ(output.c_str(), "* A * B * C * D ") ;
       output =  rdr.renderString(loop2, {}) ;
       EXPECT_STREQ(output.c_str(), "* 0 * 2 ") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};
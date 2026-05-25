#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>
#include <ctime>

using namespace twig;
using namespace std ;

class FunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};


TEST_F(FunctionTest, DateFilterNow) {
    TemplateRenderer rdr(nullptr) ;
    const string tmpl = R"({{ date() | date('Y') }})" ;

    time_t now = time(nullptr) ;
    std::tm tm = *std::localtime(&now) ;
    string expected = to_string(1900 + tm.tm_year) ;

    try {
        string output = rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_EQ(output, expected) ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(FunctionTest, DateFilterStatic) {
    TemplateRenderer rdr(nullptr) ;
    const string tmpl = R"({{ date('2024-05-20 15:34:00', 'Europe/London') | date('Y-m-d H:i:s', 'Europe/London') }})" ;

    try {
        string output = rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_EQ(output, "2024-05-20 15:34:00") ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}


// Test Variant construction and type detection
TEST_F(FunctionTest, Cycle) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl  =  R"({% set start_year = date() | date('Y') %}{% set end_year = start_year + 5 %}{% for year in start_year..end_year %}{{ cycle(['odd', 'even'], loop.index0) }} {% endfor %})";

    try {
        string output =  rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "odd even odd even odd even ") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Abs) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl  =  R"({{ "-5x" | abs }})";

    try {
        string output =  rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "5") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Capitalize) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl  =  R"({{ "καλημέρα" | capitalize }})";

    try {
        string output =  rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "Καλημέρα") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Filter) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl  =  R"({{ [5, 10, 15, 20] | filter((item) => item > 10) | join(', ') }})";

    try {
        string output =  rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "15, 20") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Find) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl1  =  R"({{ [5, 10, 15, 20] | find(item => item > 10)  }})";
  const string tmpl2  =  R"({% set sizes = {
    xxs: 32,
    xs:  34,
    s:   36,
    m:   38,
    l:   40,
    xl:  42,
} %}{{ sizes|find((k, v) => 's' not in k) }})";


    try {
        string output =  rdr.renderString(tmpl1, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "15") ;
        output =  rdr.renderString(tmpl2, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "40") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Map) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl1  =  R"({% set people = [
    {first: "Bob", last: "Smith"},
    {first: "Alice", last: "Dupond"},
] %}{{ people|map(p => [ p.first, p.last ] | join(' ') )|join(', ') }})";
  
const string tmpl2  =  R"({% set people = {
    "Bob": "Smith",
    "Alice": "Dupond",
} %}{{ people|map((key, value) => [ key, value ] | join(' '))|join(', ') }})";

    try {
        string output =  rdr.renderString(tmpl1, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "Bob Smith, Alice Dupond") ;
        output =  rdr.renderString(tmpl2, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "Alice Dupond, Bob Smith") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, Reduce) {
    TemplateRenderer rdr(nullptr) ;

    const string tmpl1  =  R"({% set numbers = [1, 2, 3] %}{{ numbers|reduce((carry, key, value) => carry + value * key) }})";
  
    try {
        string output =  rdr.renderString(tmpl1, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "8") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }  
}

TEST_F(FunctionTest, Round) {
    TemplateRenderer rdr(nullptr) ;

    const string tmpl1  =  R"({{23.45|round}} {{23.45|round(1)}} {{23.45|round(1, "floor")}})";
  
    try {
        string output =  rdr.renderString(tmpl1, Variant::Object()) ;
        EXPECT_STREQ(output.c_str(), "23 23.5 23.4") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(FunctionTest, Slice) {
    TemplateRenderer rdr(nullptr) ;

    std::vector<std::pair<std::string, std::string>> tmpl {
        { R"({{ [1, 2, 3, 4] | slice(1,2) | join(', ') }})", "2, 3" },
        { R"({{ [1, 2, 3, 4] | slice(2) | join(', ') }})", "3, 4" },
        { R"({{ "1234" | slice(2, -1)  }})", "3" },
        { R"({{ [1, 2, 3, 4] | slice(-1) | join(', ') }})", "4" },
        { R"({% for key, value in [1, 2, 3, 4, 5]|slice(1, 2, true) %}{{ key }}{{ value }}{% endfor %})", "1223" }
    };
  
    try {
        for( const auto &t: tmpl) {
            string output =  rdr.renderString(t.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), t.second.c_str()) ;
        }
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

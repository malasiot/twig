#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>

using namespace twig;
using namespace std ;

class ExpressiongTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};
#if 0
// Test Variant construction and type detection
TEST_F(ExpressiongTest, TernaryOperator) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ true ?: 'greater' }})", "1" },
    { R"({{ false ?: 'greater' }})", "greater" },
    { R"({{ 0 ?: 'greater' }})", "greater" },
    { R"({{ 1 ?: 'greater' }})", "1" },
   { R"({{ '' ?: 'greater' }})", "greater" },
    { R"({{ 'non-empty' ?: 'greater' }})", "non-empty" },
    { R"({{ null ?: 'greater' }})", "greater" },
   { R"({{ [] ?: 'greater' }})", "greater" },
    { R"({{ {} ?: 'greater' }})", "greater" },
    { R"({{ 0.0 ?: 'greater' }})", "greater" },
    { R"({{ 0.1 ?: 'greater' }})", "0.1" },
    { R"({{ false ? 'less' : 'greater' }})", "greater" },
    { R"({{ true ? 'less' : 'greater' }})", "less" },
      { R"({{ data ?? 'less' }})", "less" }

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }

   
}

TEST_F(ExpressiongTest, Arithmetic) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ 2 + (3 - 4)/3}})", "1.66666" },
   { R"({{ 2 * (3 - 4)/3 }})", "-0.66666" },
    { R"({{ 2 / (3 - 4)/3}})", "-0.66666" },
    { R"({{ 2 % (3 - 4)/3}})", "0" },
    { R"({{ 2 ** (3 - 4)/3}})", "0.16666" },
    { R"({{ 5 // 2}})", "2" },
    { R"({{ 5 // 0}})", "0" }

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            ASSERT_NEAR(stod(output), stod(expr.second), 1e-5) ;
        }
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }

   
}
#endif

TEST_F(ExpressiongTest, Concat) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ 2 ~ 'hello' ~ ( ' ' ~ 'world' ) }})", "2hello world" },
    { R"({{ 'foo' ~ 'bar' }})", "foobar" },
    { R"({{ 'foo' ~ 123 }})", "foo123" },
   { R"({{ 123 ~ 'foo' }})", "123foo" },
    { R"({{ 'foo' ~ null }})", "foo" },
    { R"({{ null ~ 'foo' }})", "foo" },
    { R"({{ 'foo' ~ true }})", "foo1" },
    { R"({{ true ~ 'foo' }})", "1foo" },
    { R"({{ 'foo' ~ false }})", "foo0" },
    { R"({{ test ~ 'foo' }})", "foo" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}


TEST_F(ExpressiongTest, Conditional) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ true and false ? 'true' : 'false' }})", "false" },
    { R"({{ true or false ? 'true' : 'false' }})", "true" },
    { R"({{ false and false ? 'true' : 'false' }})", "false" },
    { R"({{ false or false ? 'true' : 'false' }})", "false" },
    { R"({{ true and true ? 'true' : 'false' }})", "true" },
    { R"({{ true or true ? 'true' : 'false' }})", "true" },
    { R"({{ 3 in [1, 2, 4]  ? 'true' : 'false' }})", "false" },
    { R"({{ 3 in [1, 2, 3]  ? 'true' : 'false' }})", "true" },
    { R"({{ 3+1 not in [1, 2, 4]  ? 'true' : 'false' }})", "false" },
    { R"({{ "3" in [1, 2, 3]  ? 'true' : 'false' }})", "true" },
    { R"({{ "hello" starts with "he" and "hello" ends with "lo" ? 'true' : 'false' }})", "true" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}
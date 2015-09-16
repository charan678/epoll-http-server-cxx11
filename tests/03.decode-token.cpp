#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input = "a";
    std::vector<http::token_type> expected = {{"a", {}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_2 (test::simple& ts)
{
    std::string input = ",, , a , ,, ";
    std::vector<http::token_type> expected = {{"a", {}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_3 (test::simple& ts)
{
    std::string input = "a,b";
    std::vector<http::token_type> expected = {{"a", {}}, {"b", {}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_4 (test::simple& ts)
{
    std::string input = "a ,,  , b ,  , c";
    std::vector<http::token_type> expected = {{"a", {}}, {"b", {}}, {"c", {}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_5 (test::simple& ts)
{
    std::string input = "foo,bar,baz";
    std::vector<http::token_type> expected = {{"foo", {}}, {"bar", {}}, {"baz", {}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
    ts.ok (http::index (got, "bar") == 1, input + " index bar");
    ts.ok (http::index (got, "absent") == 3, input + " index absent");
}

void
test_6 (test::simple& ts)
{
    std::string input = "t;q=0.8;r=R";
    std::vector<http::token_type> expected = {{"t", {"q","0.8","r","R"}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_7 (test::simple& ts)
{
    std::string input = "t;q=\"0.8\";r=\"\\\"R\"";
    std::vector<http::token_type> expected = {{"t", {"q","0.8","r","\"R"}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_8 (test::simple& ts)
{
    std::string input = "t;q=0.3,u;q=0.7";
    std::vector<http::token_type> expected = {{"t", {"q","0.3"}}, {"u", {"q","0.7"}}};
    std::vector<http::token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

int
main ()
{
    test::simple ts (18);
    test_1 (ts);
    test_2 (ts);
    test_3 (ts);
    test_4 (ts);
    test_5 (ts);
    test_6 (ts);
    test_7 (ts);
    test_8 (ts);
    return ts.done_testing ();
}

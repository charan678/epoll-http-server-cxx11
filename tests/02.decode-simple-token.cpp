#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input = "a";
    std::vector<http::simple_token_type> expected = {{"a"}};
    std::vector<http::simple_token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_2 (test::simple& ts)
{
    std::string input = ",, , a , ,, ";
    std::vector<http::simple_token_type> expected = {{"a"}};
    std::vector<http::simple_token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_3 (test::simple& ts)
{
    std::string input = "a,b";
    std::vector<http::simple_token_type> expected = {{"a"}, {"b"}};
    std::vector<http::simple_token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_4 (test::simple& ts)
{
    std::string input = "a ,,  , b ,  , c";
    std::vector<http::simple_token_type> expected = {{"a"}, {"b"}, {"c"}};
    std::vector<http::simple_token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_5 (test::simple& ts)
{
    std::string input = "foo,bar,baz";
    std::vector<http::simple_token_type> expected = {{"foo"}, {"bar"}, {"baz"}};
    std::vector<http::simple_token_type> got;
    ts.ok (http::decode (got, input, 1), input + " decode");
    ts.ok (got == expected, input + " got");
    ts.ok (http::index (got, "bar") == 1, input + " index bar");
    ts.ok (http::index (got, "absent") == 3, input + " index absent");
}

int
main ()
{
    test::simple ts (12);
    test_1 (ts);
    test_2 (ts);
    test_3 (ts);
    test_4 (ts);
    test_5 (ts);
    return ts.done_testing ();
}

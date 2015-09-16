#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input = "\"W/xxxxx\",\"yyy\"";
    std::vector<http::etag_type> expected = {{false, "\"W/xxxxx\""}, {false, "\"yyy\""}};
    std::vector<http::etag_type> got;
    ts.ok (http::decode (got, input), input + " good");
    ts.ok (got == expected, input + " got");
}

void
test_2 (test::simple& ts)
{
    std::string input = "W/\"xxxxx\",\"yyy\"";
    std::vector<http::etag_type> expected = {{true, "\"xxxxx\""}, {false, "\"yyy\""}};
    std::vector<http::etag_type> got;
    ts.ok (http::decode (got, input), input + " good");
    ts.ok (got == expected, input + " got");
}

void
test_3 (test::simple& ts)
{
    std::string input = "*";
    std::vector<http::etag_type> expected = {{false, "*"}};
    std::vector<http::etag_type> got;
    ts.ok (http::decode (got, input), input + " good");
    ts.ok (got == expected, input + " got");
}

int
main ()
{
    test::simple ts (6);
    test_1 (ts);
    test_2 (ts);
    test_3 (ts);
    return ts.done_testing ();
}

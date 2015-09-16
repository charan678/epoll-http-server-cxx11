#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input = "0";
    http::content_length_type expected = {200, 0};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_2 (test::simple& ts)
{
    std::string input = "1234";
    http::content_length_type expected = {200, 1234};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_3 (test::simple& ts)
{
    std::string input = "12345678";
    http::content_length_type expected = {200, 12345678};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_4 (test::simple& ts)
{
    std::string input = "128,128,128";
    http::content_length_type expected = {200, 128};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_5 (test::simple& ts)
{
    std::string input = ", 128, , 128,,   ,128, ";
    http::content_length_type expected = {200, 128};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_6 (test::simple& ts)
{
    std::string input = "2147483647";
    http::content_length_type expected = {200, 2147483647};
    http::content_length_type got;
    ts.ok (http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_7 (test::simple& ts)
{
    std::string input = "2147483648";
    http::content_length_type expected = {413, 0};
    http::content_length_type got;
    ts.ok (! http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_8 (test::simple& ts)
{
    std::string input = "127,128,128";
    http::content_length_type expected = {400, 0};
    http::content_length_type got;
    ts.ok (! http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_9 (test::simple& ts)
{
    std::string input = "128,128,127";
    http::content_length_type expected = {400, 0};
    http::content_length_type got;
    ts.ok (! http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_10 (test::simple& ts)
{
    std::string input = "";
    http::content_length_type expected = {400, 0};
    http::content_length_type got;
    ts.ok (! http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

void
test_11 (test::simple& ts)
{
    std::string input = ",  ,";
    http::content_length_type expected = {400, 0};
    http::content_length_type got;
    ts.ok (! http::decode (got, input), input + " decode");
    ts.ok (got == expected, input + " got");
}

int
main ()
{
    test::simple ts (22);
    test_1 (ts);
    test_2 (ts);
    test_3 (ts);
    test_4 (ts);
    test_5 (ts);
    test_6 (ts);
    test_7 (ts);
    test_8 (ts);
    test_9 (ts);
    test_10 (ts);
    test_11 (ts);
    return ts.done_testing ();
}

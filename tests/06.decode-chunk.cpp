#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input =
    "D\r\n"
    "Hello, world\n"
    "\r\n"
    "D;ignore;ignore2\r\n"
    "Hello, world\n"
    "\r\n"
    "00D;ignore=ignore;ignore=\"\\\"\";ignore\r\n"
    "Hello, world\n"
    "\r\n"
    "D\r\n"
    "Hello, world\n"
    "\r\n"
    "000\r\n"
    "Ignore: Ignore\r\n"
    "Ignore-1:\r\n"
    "Ignore-2: Ignore\r\n"
    " Ignore\r\n"
    "Ignore-3: Ignore\r\n"
    "\r\n"
    ;
    std::string expected =
    "Hello, world\n"
    "Hello, world\n"
    "Hello, world\n"
    "Hello, world\n"
    ;
    std::string got;
    http::decoder_chunk_type decoder;
    for (int c : input)
        if (! decoder.put (c, got))
            break;
    ts.ok (decoder.good (), "input good");
    ts.ok (got == expected, "body");
}

int
main ()
{
    test::simple ts (2);
    test_1 (ts);
    return ts.done_testing ();
}

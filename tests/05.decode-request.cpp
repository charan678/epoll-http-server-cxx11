#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::string input1 =
        "GET /example.html HTTP/1.1\r\n"
        "Host: example.net:10080  \r\n"
        "Connection: close  \r\n"
        "Accept-Language:   ,ja, en  ,  \r\n"
        "\r\n"
        ;
    http::request_type req;
    http::decoder_request_line_type decoder_line;
    http::decoder_request_header_type decoder_header;
    for (int c : input1) {
        if (! decoder_line.good ()) {
            decoder_line.put (c, req);
            if (decoder_line.bad ())
                break;
        }
        else if (! decoder_header.put (c, req))
            break;
    }
    ts.ok (decoder_line.good (), "input1 decoder_line good.");
    ts.ok (decoder_header.good (), "input1 decoder_header good.");
    ts.ok (req.method == "GET", "method GET");
    ts.ok (req.uri == "/example.html", "target-form /example.html");
    ts.ok (req.http_version == "HTTP/1.1", "version HTTP/1.1");
    ts.ok (req.header.count ("host") > 0, "Host");
    ts.ok (req.header.at ("host") == "example.net:10080", "Host: example.net:10080");
    ts.ok (req.header.count ("connection") > 0, "Connection");
    ts.ok (req.header.at ("connection") == "close", "Connection: close");
    ts.ok (req.header.count ("accept-language") > 0, "Accept-Language");
    ts.ok (req.header.at ("accept-language") == ",ja, en  ,", "Accept-Language: ,ja, en  ,");

    std::string input2 =
        "OPTION * HTTP/1.0\r\n"
        "\r\n"
        ;
    req.clear ();
    decoder_line.clear ();
    decoder_header.clear ();
    for (int c : input2) {
        if (! decoder_line.good ()) {
            decoder_line.put (c, req);
            if (decoder_line.bad ())
                break;
        }
        else if (! decoder_header.put (c, req))
            break;
    }
    ts.ok (decoder_line.good (), "input2 decoder_line good.");
    ts.ok (decoder_header.good (), "input2 decoder_header good.");
    ts.ok (req.method == "OPTION", "method OPTION");
    ts.ok (req.uri == "*", "target-form *");
    ts.ok (req.http_version == "HTTP/1.0", "version HTTP/1.0");
    ts.ok (req.header.size () == 0, "header empty");
}

int
main ()
{
    test::simple ts (17);
    test_1 (ts);
    return ts.done_testing ();
}

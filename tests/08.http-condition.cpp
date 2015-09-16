#include "../http.hpp"
#include "taptests.hpp"

void
test_1 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-none-match", "\"Wxxxxx/\",\"yyy\""},
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 14:04:06 GMT");
    http::etag_type etag (false, "\"Wxxxxx/\"");
    http::condition_type condition (etag, tm);
    ts.ok (304 == condition.check ("GET", header), "304 GET if-none-match");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-none-match");
}

void
test_2 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-none-match", "\"xxxxx\""},
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"prev\"");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-none-match");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-none-match");
}

void
test_3 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-none-match", "*"},
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 14:04:06 GMT");
    http::etag_type etag (false, "\"prev\"");
    http::condition_type condition (etag, tm);
    ts.ok (304 == condition.check ("GET", header), "304 GET if-none-match *");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-none-match *");
}

void
test_4 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-none-match", "*"},
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 14:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-none-match");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-none-match");
}

void
test_5 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (304 == condition.check ("GET", header), "304 GET if-modified-since");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT ignore if-modified-since");
}

void
test_6 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-modified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 14:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-modified-since");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT ignore if-modified-since");
}

void
test_7 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "\"xxx\""},
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (412 == condition.check ("GET", header), "412 GET if-match");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-match");
}

void
test_8 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "\"yyy\""},
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-match");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-match");
}

void
test_9 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 14:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (412 == condition.check ("GET", header), "412 GET if-unmodified-since");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-unmodified-since");
}

void
test_10 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-unmodified-since");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-unmodified-since");
}

void
test_11 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "\"yyy\""},
        {"if-none-match", "\"xxx\""},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-match or if-none-match");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-match or if-none-match");
}

void
test_12 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "\"yyy\""},
        {"if-none-match", "\"yyy\""},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (304 == condition.check ("GET", header), "304 GET if-match or if-none-match");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-match or if-none-match");
}

void
test_13 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
        {"if-none-match", "\"xxx\""},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-unmodified-since or if-none-match");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-unmodified-since or if-none-match");
}

void
test_14 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-unmodified-since", "Wed, 08 Jul 2015 13:04:06 GMT"},
        {"if-none-match", "\"yyy\""},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (304 == condition.check ("GET", header), "304 GET if-unmodified-since or if-none-match");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-unmodified-since or if-none-match");
}

void
test_15 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "*"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "\"yyy\"");
    http::condition_type condition (etag, tm);
    ts.ok (200 == condition.check ("GET", header), "200 GET if-match *");
    ts.ok (200 == condition.check ("PUT", header), "200 PUT if-match *");
}

void
test_16 (test::simple& ts)
{
    std::map<std::string, std::string> header {
        {"if-match", "*"},
    };
    std::time_t tm = http::time_decode ("%a, %d %b %Y %H:%M:%S GMT", "Wed, 08 Jul 2015 13:04:06 GMT");
    http::etag_type etag (false, "");
    http::condition_type condition (etag, tm);
    ts.ok (412 == condition.check ("GET", header), "412 GET if-match *");
    ts.ok (412 == condition.check ("PUT", header), "412 PUT if-match *");
}

int
main ()
{
    test::simple ts (32);
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
    test_12 (ts);
    test_13 (ts);
    test_14 (ts);
    test_15 (ts);
    test_16 (ts);
    return ts.done_testing ();
}

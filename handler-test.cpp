#include "server.hpp"

namespace http {

bool
handler_test_type::get (http::connection_type& r)
{
    html_builder_type html;
    html <<
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>test</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>test</h1>\n"
        "<p>method " << r.request.method <<
        " URI " << r.request.uri << ".</p>\n"
        "</body>\n"
        "</html>\n";
    r.response.body = html.string ();
    r.response.code = 200;
    r.response.header["content-type"] = "text/html; charset=UTF-8";
    r.response.header["content-length"] = std::to_string (r.response.body.size ());
    return true;
}

bool
handler_test_type::post (http::connection_type& r)
{
    html_builder_type html;
    html <<
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>test</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>test</h1>\n"
        "<p>method " << r.request.method <<
        " URI " << r.request.uri << ".</p>\n"
        "<pre>" << r.request.body << "</pre>\n"
        "</body>\n"
        "</html>\n";
    r.response.body = html.string ();
    r.response.code = 200;
    r.response.header["content-type"] = "text/html; charset=UTF-8";
    r.response.header["content-length"] = std::to_string (r.response.body.size ());
    return true;
}

}//namespace http

#include <string>
#include <vector>
#include <utility>
#include "server.hpp"

namespace http {

bool
handler_type::process (http::connection_type& r)
{
    if (r.request.method == "GET")
        return get (r);
    else if (r.request.method == "HEAD")
        return get (r);
    else if (r.request.method == "POST")
        return post (r);
    else if (r.request.method == "PUT")
        return put (r);
    return method_not_allowed (r);
}

bool
handler_type::get (http::connection_type& r)
{
    return not_found (r);
}

bool
handler_type::post (http::connection_type& r)
{
    return method_not_allowed (r);
}

bool
handler_type::put (http::connection_type& r)
{
    return method_not_allowed (r);
}

bool
handler_type::not_modified (http::connection_type& r)
{
    r.response.content_length = 0;
    r.response.code = 304;
    r.response.body.clear ();
    return true;
}

bool
handler_type::bad_request (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 400);
    html <<
        "<p>return to <a href=\"/index.html\">index page</a>.</p>\n"
        "<p>Your browser sent a request that\n"
        "this server could not understand.</p>\n";
    error_end (r, html);
    r.response.header["connection"] = "close";
    return true;
}

bool
handler_type::not_found (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 404);
    html <<
        "<p>The requested URL " << r.request.uri <<
        " was not found on this server.</p>\n"
        "<p>return to <a href=\"/index.html\">index page</a>.</p>\n";
    error_end (r, html);
    return true;
}

bool
handler_type::method_not_allowed (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 405);
    html <<
        "<p>The requested method " << r.request.method <<
        " is not allowed for the URL " << r.request.uri << ".</p>\n";
    error_end (r, html);
    return true;
}

bool
handler_type::request_timeout (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 408);
    html <<
        "<p>Server timeout waiting for the HTTP request from the client.</p>\n";
    error_end (r, html);
    r.response.header["connection"] = "close";
    return true;
}

bool
handler_type::length_required (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 411);
    html <<
        "<p>A request of the requested method " << r.request.method <<
        " requires a valid Content-length.</p>\n";
    error_end (r, html);
    r.response.header["connection"] = "close";
    return true;
}

bool
handler_type::precondition_failed (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 411);
    html <<
        "<p>The precondition on the request "
        "for the URL " << r.request.uri <<
        " evaluated to false.</p>\n";
    error_end (r, html);
    return true;
}

bool
handler_type::request_entity_too_large (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 413);
    html <<
        "<p>The requested URL " << r.request.uri << "\n"
        "does not allow request data with " << r.request.method <<
        " requests, or the amount of data provided in\n"
        "the request exceeds the capacity limit.</p>\n";
    error_end (r, html);
    r.response.header["connection"] = "close";
    return true;
}

bool
handler_type::unsupported_media_type (http::connection_type& r)
{
    html_builder_type html;
    error_start (r, html, 415);
    html <<
        "<p>The supplied request data is not in a format\n"
        "acceptable for processing by this resource.</p>\n";
    error_end (r, html);
    r.response.header["connection"] = "close";
    return true;
}

void
handler_type::error_start (http::connection_type& r, html_builder_type& html, int code)
{
    r.response.code = code;
    std::string title = r.response.statuscode ();
    html <<
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>" << title << "</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>" << title.substr (4) << "</h1>\n";
}

void
handler_type::error_end (http::connection_type& r, html_builder_type& html)
{
    html <<
        "</body>\n"
        "</html>\n";
    r.response.body = html.string ();
    r.response.header.clear ();
    r.response.header["content-type"] = "text/html; charset=UTF-8";
    r.response.header["content-length"] = std::to_string (r.response.body.size ());
}

}//namespace http

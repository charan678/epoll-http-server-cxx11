#include "http.hpp"

namespace http {

void
request_type::clear ()
{
    method.clear ();
    uri.clear ();
    http_version.clear ();
    header.clear ();
    content_length = 0;
    body.clear ();
}

}//namespace http

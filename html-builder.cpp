#include <string>
#include "html-builder.hpp"

namespace http {

html_builder_type::html_builder_type ()
    : buffer ()
{
}

html_builder_type&
html_builder_type::operator << (char const* s)
{
    buffer += s;
    return *this;
}

html_builder_type&
html_builder_type::operator << (int const i)
{
    buffer += std::to_string (i);
    return *this;
}

html_builder_type&
html_builder_type::operator << (std::string const& s)
{
    for (char ch : s)
        switch (ch) {
        default: buffer.push_back (ch); break;
        case '&': buffer += "&amp;"; break;
        case '<': buffer += "&lt;"; break;
        case '>': buffer += "&gt;"; break;
        case '"': buffer += "&quot;"; break;
        }
    return *this;
}

std::string const&
html_builder_type::string () const
{
    return buffer;
}

}//namespace http

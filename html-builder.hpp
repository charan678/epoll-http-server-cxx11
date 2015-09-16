#ifndef HTMLBUILDER_HPP
#define HTMLBUILDER_HPP

#include <string>

namespace http {

class html_builder_type {
public:
    html_builder_type ();
    html_builder_type& operator << (char const* s);
    html_builder_type& operator << (int const i);
    html_builder_type& operator << (std::string const& s);
    std::string const& string () const;

private:
    std::string buffer;
};

}//namespace http

#endif

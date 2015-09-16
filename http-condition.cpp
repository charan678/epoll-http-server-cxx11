#include <string>
#include <map>
#include <ctime>
#include "http.hpp"

namespace http {

int
condition_type::check (std::string const& method,
    std::map<std::string, std::string>& header)
{
    bool isget = (method == "GET" || method == "HEAD");
    int r = OK;
    if (header.count ("if-match") > 0)
        r = if_match (header.at ("if-match"));
    else if (header.count ("if-unmodified-since") > 0)
        r = if_unmodified_since (header.at ("if-unmodified-since"));
    if (OK != r)
        return 412;
    if (header.count ("if-none-match") > 0)
        r = if_none_match (header.at ("if-none-match"));
    else if (isget && header.count ("if-modified-since") > 0)
        r = if_modified_since (header.at ("if-modified-since"));
    if (OK != r)
        return isget ? 304 : 412;
    return 200;
}

int
condition_type::if_match (std::string const& field)
{
    std::vector<etag_type> list;
    if (! decode (list, field))
        return OK;
    if (! list.empty () && list[0].opaque == "*")
        return etag.empty () ? FAILED : OK;
    for (auto const& x : list)
        if (etag.equal_strong (x))
            return OK;
    return FAILED;
}

int
condition_type::if_none_match (std::string const& field)
{
    std::vector<etag_type> list;
    if (! decode (list, field))
        return OK;
    if (list[0].opaque == "*")
        return etag.empty () ? OK : FAILED;
    for (auto const& x : list)
        if (etag.equal_weak (x))
            return FAILED;
    return OK;
}

int
condition_type::if_unmodified_since (std::string const& field)
{
    std::time_t since = time_decode ("%a, %d %b %Y %H:%M:%S GMT", field);
    return since < 0 ? OK : mtime <= since ? OK : FAILED;
}

int
condition_type::if_modified_since (std::string const& field)
{
    std::time_t since = time_decode ("%a, %d %b %Y %H:%M:%S GMT", field);
    return since < 0 ? OK : mtime > since ? OK : FAILED;
}

}//namespace http

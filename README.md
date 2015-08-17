http-server - practical Linux epoll HTTP/1.1 origin server
==========================================================

This is a practical implementation of an HTTP origin server
with Linux epoll edge triggers facilities
based on Reactor design pattern.
Almost all of the lexical parts are hand-compiled DFA
capturing sub strings.

* keep-alive implemented
* chunked request implemented
* chunked response implemented
* conditional implemented
* static file implemented
* range not implemented
* authentications not implemented
* proxy not implemented

This server depends on Linux specific systemcalls.

Version
------

0.0.1 alpha

Build
-----

    $ make
    $ ./http-server

from other terminals

    $ ruby client/get.rb
    $ ruby client/post-chunked.rb

References
--------

 1. http://www.cs.wustl.edu/~schmidt/PDF/reactor-siemens.pdf
 2. RFC 7230 HTTP/1.1 standard

License
------

The BSD 3-Clause

Copyright (c) 2015, MIZUTANI Tociyuki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <string>

namespace test {

class simple {
private:
    bool mno_plan, mskip, mtodo, mprt_nums;
    int muse_nums, mcur_test, mtest_skip, mtest_ok, mtest_notok;

public:
    simple ()
        : mno_plan (true), mskip (false), mtodo (false), mprt_nums (false),
          muse_nums (0), mcur_test (0),
          mtest_skip (0), mtest_ok (0), mtest_notok (0) { }

    simple (int n)
        : mno_plan (false), mskip (false), mtodo (false), mprt_nums (true),
          muse_nums (n), mcur_test (0),
          mtest_skip (0), mtest_ok (0), mtest_notok (0) { }

    void skip () { mskip = true; }
    void todo () { mtodo = true; }

    bool ok (bool tst, std::string desc)
    {
        if (mprt_nums) {
            std::cout << "1.." << muse_nums << std::endl;
            mprt_nums = false;
        }
        ++mcur_test;
        if (mskip) {
            ++mtest_skip;
            std::cout << "ok " << mcur_test << " # SKIP";
            if (! desc.empty ()) {
                std::cout << " " << desc;
            }
        }
        else if (tst) {
            ++mtest_ok;
            std::cout << "ok " << mcur_test;
            if (! desc.empty ()) {
                std::cout << " - " << desc;
            }
        }
        else {
            ++mtest_notok;
            std::cout << "not ok " << mcur_test;
            if (! desc.empty ()) {
                std::cout << " - " << desc;
            }
            if (mtodo) {
                std::cout << " # TODO";
            }
        }
        std::cout << std::endl;
        mskip = false;
        mtodo = false;
        return tst;
    }

    void diag (std::string s)
    {
        std::cout << "# ";
        for (auto c : s) {
            std::cout << c;
            if (c == '\n') {
                std::cout << "# ";
            }
        }
        std::cout << std::endl;
    }

    bool done_testing ()
    {
        if (mno_plan) {
            muse_nums = mcur_test;
            mprt_nums = true;
        }
        if (mprt_nums) {
            std::cout << "1.." << mcur_test << std::endl;
            mprt_nums = false;
        }
        return muse_nums == mtest_ok + mtest_skip ? EXIT_SUCCESS : EXIT_FAILURE;
    }
};

} //namespace test

/* Test Anything Protocol for C++11
 *
 * Example:
 *
 *    #include "taptests.hpp"
 *
 *    int main ()
 *    {
 *        test::simple t (2);
 *        // test::simple t; // no_plan
 *        t.ok (true, "description");
 *        t.ok (2 + 2 == 4, "a to z");
 *        t.diag ("it is something to be wrong, it will.");
 *        return t.done_testing ();
 *    }
 *
 * License: The BSD 3-Clause
 *
 * Copyright (c) 2015, MIZUTANI Tociyuki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

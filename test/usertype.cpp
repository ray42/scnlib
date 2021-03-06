// Copyright 2017-2019 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

struct user_type {
    int val1{}, val2{};
};
struct user_type2 {
    int val1{}, val2{};
};

namespace scn {
    template <typename CharT>
    struct scanner<CharT, user_type> : public scn::empty_parser {
        template <typename Context>
        error scan(user_type& val, Context& ctx)
        {
            auto r = scn::scan(ctx.range(), "[{}, {}]", val.val1, val.val2);
            ctx.range().advance_to(r.begin());
            if (r) {
                return {};
            }
            return r.error();
        }
    };
    template <typename CharT>
    struct scanner<CharT, user_type2> : public scn::empty_parser {
        template <typename Context>
        error scan(user_type2& val, Context& ctx)
        {
            using pctx_type =
                basic_parse_context<typename Context::locale_type>;
            int i, j;
            auto args = make_args<Context, pctx_type>(i, j);
            auto newctx = Context(ctx.range());
            auto pctx = pctx_type("[{}, {}]", newctx);
            auto ret = vscan(newctx, pctx, {args});
            ctx.range().advance_to(ret.begin());
            if (ret) {
                val.val1 = i;
                val.val2 = j;
                return {};
            }
            return ret.error();
        }
    };
}  // namespace scn

TEST_CASE_TEMPLATE_DEFINE("user type", T, user_type_test)
{
    T ut{};

    SUBCASE("regular")
    {
        auto ret = scn::scan("[4, 20]", "{}", ut);
        CHECK(ret);
        CHECK(ut.val1 == 4);
        CHECK(ut.val2 == 20);
    }
    SUBCASE("format string error")
    {
        auto ret = scn::scan("[4, 20]", "{", ut);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::invalid_format_string);

        ret = scn::scan(ret.range(), "{:a}", ut);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::invalid_format_string);
    }
    SUBCASE("mixed")
    {
        int i, j;
        auto ret = scn::scan("123 [4, 20] 456", "{} {} {}", i, ut, j);
        CHECK(ret);
        CHECK(i == 123);
        CHECK(ut.val1 == 4);
        CHECK(ut.val2 == 20);
        CHECK(j == 456);
    }
}
TYPE_TO_STRING(user_type);
TYPE_TO_STRING(user_type2);
TEST_CASE_TEMPLATE_INSTANTIATE(user_type_test, user_type, user_type2);

struct non_default_construct {
    non_default_construct() = delete;

    non_default_construct(int val) : value(val) {}

    int value{};
};

namespace scn {
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")
    template <typename CharT>
    struct scanner<CharT, wrap_default<non_default_construct>>
        : public scanner<CharT, int> {
        template <typename Context>
        error scan(wrap_default<non_default_construct>& val, Context& ctx)
        {
            int tmp{};
            auto ret = scanner<CharT, int>::scan(tmp, ctx);
            if (!ret) {
                return ret;
            }
            val = non_default_construct(tmp);
            return {};
        }
    };
    SCN_CLANG_POP
}  // namespace scn

TEST_CASE("non_default_construct")
{
    scn::wrap_default<non_default_construct> val;
    auto ret = scn::scan("42", "{}", val);

    CHECK(ret);

    REQUIRE(val);
    CHECK(val->value == 42);
}

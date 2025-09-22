// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <libhal-util/streams.hpp>

#include <libhal-util/as_bytes.hpp>
#include <libhal-util/timeout.hpp>

#include <boost/ut.hpp>

namespace hal {
namespace {
struct example_stream
{
  work_state state()
  {
    return m_state;
  }

  work_state m_state;
};

std::span<hal::byte const> operator|(
  std::span<hal::byte const> const& p_input_data,
  [[maybe_unused]] example_stream& p_self)
{
  return p_input_data;
}
}  // namespace

boost::ut::suite<"stream_terminated_test"> stream_terminated_test = [] {
  using namespace boost::ut;
  using namespace std::literals;

  // NOTE: this is here to bypass the following error in CLANG.
  //       see https://bugs.llvm.org/show_bug.cgi?id=33068
  //       without this, you will get the following error:
  //
  //          function 'operator|' is not needed and will not be emitted
  //          [-Werror,-Wunneeded-internal-declaration]
  //
  example_stream compiler_error_bypass_error;
  std::span<hal::byte const>() | compiler_error_bypass_error;

  "hal::terminate(byte_stream) -> true with finished state"_test = []() {
    // Setup
    example_stream stream;
    stream.m_state = work_state::finished;

    // Exercise
    bool result = hal::terminated(stream);

    // Verify
    expect(result) << "Should return true as the work_state is 'finished'!";
  };

  "hal::terminate(byte_stream) -> true with failed state"_test = []() {
    // Setup
    example_stream stream;
    stream.m_state = work_state::failed;

    // Exercise
    bool result = hal::terminated(stream);

    // Verify
    expect(result) << "Should return true as the work_state is 'failed'!";
  };

  "hal::terminate(byte_stream) -> false with in_progress state"_test = []() {
    // Setup
    example_stream stream;
    stream.m_state = work_state::in_progress;

    // Exercise
    bool result = hal::terminated(stream);

    // Verify
    expect(!result)
      << "Should return false as the work_state is 'in_progress'!";
  };
};

// =============================================================================
//
//                              |  parse<T> Stream  |
//
// =============================================================================
boost::ut::suite<"parse_stream_test"> parse_stream_test = [] {
  using namespace boost::ut;
  using namespace std::literals;

  "[parse<std::uint32_t>] normal usage"_test = []() {
    // Setup
    std::string_view digits_in_between = "abcd1234x---";
    auto digits_span = hal::as_bytes(digits_in_between);
    hal::stream_parse<std::uint32_t> parse_int;

    // Exercise
    auto remaining = digits_span | parse_int;

    expect(that % work_state::finished == parse_int.state());
    expect(that % 1234 == parse_int.value());
    expect(that % digits_span.subspan(digits_in_between.find('x')).size() ==
           remaining.size());
    expect(that % digits_span.subspan(digits_in_between.find('x')).data() ==
           remaining.data());
  };

  "[parse<std::uint64_t>] normal usage"_test = []() {
    // Setup
    std::string_view digits_in_between = "abcd12356789101234x---";
    auto digits_span = hal::as_bytes(digits_in_between);
    hal::stream_parse<std::uint64_t> parse_int;

    // Exercise
    auto remaining = digits_span | parse_int;

    expect(that % work_state::finished == parse_int.state());
    expect(that % 12'356'789'101'234ULL == parse_int.value());
    expect(that % digits_span.subspan(digits_in_between.find('x')).size() ==
           remaining.size());
    expect(that % digits_span.subspan(digits_in_between.find('x')).data() ==
           remaining.data());
  };

  "[parse<std::uint32_t>] empty span"_test = []() {
    // Setup
    hal::stream_parse<std::uint32_t> parse_int;

    // Exercise
    auto remaining = std::span<hal::byte const>() | parse_int;

    expect(that % work_state::in_progress == parse_int.state());
    expect(that % 0 == parse_int.value());
    expect(that % 0 == remaining.size());
    expect(that % nullptr == remaining.data());
  };

  "[parse<std::uint32_t>] no digits"_test = []() {
    // Setup
    std::string_view digits_in_between = "abcd?efghx-[--]/";
    auto digits_span = hal::as_bytes(digits_in_between);
    hal::stream_parse<std::uint32_t> parse_int;

    // Exercise
    auto remaining = digits_span | parse_int;

    expect(that % work_state::in_progress == parse_int.state());
    expect(that % 0 == parse_int.value());
    expect(that % 0 == remaining.size());
    expect(that % &(*digits_span.end()) == remaining.data());
  };

  "[parse<std::uint32_t>] two blocks"_test = []() {
    // Setup
    std::array<std::string_view, 2> halves = { "abc12", "98ce" };
    auto span0 = hal::as_bytes(halves[0]);
    auto span1 = hal::as_bytes(halves[1]);

    hal::stream_parse<std::uint32_t> parse_int;

    auto remaining0 = span0 | parse_int;
    auto remaining1 = span1 | parse_int;

    expect(that % work_state::finished == parse_int.state());
    expect(that % 1298 == parse_int.value());
    expect(that % 0 == remaining0.size());
    expect(that % &(*span0.end()) == remaining0.data());
    expect(that % span1.subspan(halves[1].find('c')).size() ==
           remaining1.size());
    expect(that % span1.subspan(halves[1].find('c')).data() ==
           remaining1.data());
  };

  "[parse<std::uint32_t>] three blocks"_test = []() {
    // Setup
    std::array<std::string_view, 3> halves = { "abc1", "23", "45ce" };
    auto span0 = hal::as_bytes(halves[0]);
    auto span1 = hal::as_bytes(halves[1]);
    auto span2 = hal::as_bytes(halves[2]);

    hal::stream_parse<std::uint32_t> parse_int;

    auto remaining0 = span0 | parse_int;
    auto remaining1 = span1 | parse_int;
    auto remaining2 = span2 | parse_int;

    expect(that % work_state::finished == parse_int.state());
    expect(that % 12345 == parse_int.value());
    expect(that % 0 == remaining0.size());
    expect(that % &(*span0.end()) == remaining0.data());
    expect(that % 0 == remaining1.size());
    expect(that % &(*span1.end()) == remaining1.data());
    expect(that % span2.subspan(halves[2].find('c')).size() ==
           remaining2.size());
    expect(that % span2.subspan(halves[2].find('c')).data() ==
           remaining2.data());
  };

  "[parse<std::uint32_t>] 2 blocks, 1 empty"_test = []() {
    // Setup
    std::array<std::string_view, 3> halves = { "abc12", "", "45ce" };
    auto span0 = hal::as_bytes(halves[0]);
    auto span1 = hal::as_bytes(halves[1]);
    auto span2 = hal::as_bytes(halves[2]);

    hal::stream_parse<std::uint32_t> parse_int;

    auto remaining0 = span0 | parse_int;
    auto remaining1 = span1 | parse_int;
    auto remaining2 = span2 | parse_int;

    expect(that % work_state::finished == parse_int.state());
    expect(that % 1245 == parse_int.value());
    expect(that % 0 == remaining0.size());
    expect(that % &(*span0.end()) == remaining0.data());
    expect(that % 0 == remaining1.size());
    expect(that % &(*span1.end()) == remaining1.data());
    expect(that % span2.subspan(halves[2].find('c')).size() ==
           remaining2.size());
    expect(that % span2.subspan(halves[2].find('c')).data() ==
           remaining2.data());
  };

  "[parse<std::uint32_t>] chain of three"_test = []() {
    // Setup
    std::string_view three_numbers = "a123b456c789d";
    auto span = hal::as_bytes(three_numbers);

    hal::stream_parse<std::uint32_t> parse_int0;
    hal::stream_parse<std::uint32_t> parse_int1;
    hal::stream_parse<std::uint32_t> parse_int2;

    // Exercise
    auto remaining = span | parse_int0 | parse_int1 | parse_int2;

    expect(that % work_state::finished == parse_int0.state());
    expect(that % work_state::finished == parse_int1.state());
    expect(that % work_state::finished == parse_int2.state());
    expect(that % 123 == parse_int0.value());
    expect(that % 456 == parse_int1.value());
    expect(that % 789 == parse_int2.value());
    expect(that % 1 == remaining.size());
    // 'd' is left in the end
    expect(that % &span.back() == remaining.data());
  };
};

// =============================================================================
//
//                               |  Find Stream  |
//
// =============================================================================
boost::ut::suite<"find_stream_test"> find_stream_test = [] {
  // Setup
  using namespace boost::ut;
  using namespace std::literals;

  "[find] normal usage"_test = []() {
    // Setup
    std::string_view str = "Content-Length: 1023\r\n";
    auto span = hal::as_bytes(str);
    hal::stream_find finder(hal::as_bytes(": "sv));

    // Exercise
    auto remaining = span | finder;

    expect(that % work_state::finished == finder.state());
    expect(that % span.subspan(str.find(':') + 1).size() == remaining.size());
    expect(that % span.subspan(str.find(':') + 1).data() == remaining.data());
  };

  "[find] empty span"_test = []() {
    // Setup
    hal::stream_find finder(hal::as_bytes(": "sv));

    // Exercise
    auto remaining = std::span<hal::byte>() | finder;

    expect(that % work_state::in_progress == finder.state());
    expect(that % 0 == remaining.size());
    expect(that % nullptr == remaining.data());
  };

  "[find] nothing"_test = []() {
    // Setup
    std::string_view digits_in_between = "abcd??efghx-[--]/";
    auto digits_span = hal::as_bytes(digits_in_between);

    hal::stream_find finder(hal::as_bytes("????"sv));

    // Exercise
    auto remaining = digits_span | finder;

    expect(that % 0 == remaining.size());
    expect(that % &(*digits_span.end()) == remaining.data());
  };

  "[find] two blocks"_test = []() {
    // Setup
    std::array<std::string_view, 2> halves = { "abc12", "98ce" };
    auto span0 = hal::as_bytes(halves[0]);
    auto span1 = hal::as_bytes(halves[1]);

    hal::stream_find finder(hal::as_bytes("1298"sv));

    auto remaining0 = span0 | finder;
    auto remaining1 = span1 | finder;

    expect(that % 0 == remaining0.size());
    expect(that % &(*span0.end()) == remaining0.data());
    expect(that % span1.subspan(halves[1].find('8')).size() ==
           remaining1.size());
    expect(that % span1.subspan(halves[1].find('8')).data() ==
           remaining1.data());
  };

  "[find] three blocks"_test = []() {
    // Setup
    std::array<std::string_view, 3> halves = { "abc1", "23", "45ce" };
    auto span0 = hal::as_bytes(halves[0]);
    auto span1 = hal::as_bytes(halves[1]);
    auto span2 = hal::as_bytes(halves[2]);

    hal::stream_find finder(hal::as_bytes("345"sv));

    auto remaining0 = span0 | finder;
    auto remaining1 = span1 | finder;
    auto remaining2 = span2 | finder;

    expect(that % 0 == remaining0.size());
    expect(that % &(*span0.end()) == remaining0.data());
    expect(that % 0 == remaining1.size());
    expect(that % &(*span1.end()) == remaining1.data());
    expect(that % span2.subspan(halves[2].find('5')).size() ==
           remaining2.size());
    expect(that % span2.subspan(halves[2].find('5')).data() ==
           remaining2.data());
  };

  "[find] chain of three"_test = []() {
    // Setup
    std::string_view three_numbers = "----a---b---c";
    auto span = hal::as_bytes(three_numbers);

    hal::stream_find finder0(hal::as_bytes("a"sv));
    hal::stream_find finder1(hal::as_bytes("b"sv));
    hal::stream_find finder2(hal::as_bytes("c"sv));

    // Exercise
    auto remaining = span | finder0 | finder1 | finder2;

    expect(that % 1 == remaining.size());
    // 'd' is left in the end
    expect(that % &span.back() == remaining.data());
  };
};

// =============================================================================
//
//                             |  fill_upto Stream  |
//
// =============================================================================
boost::ut::suite<"fill_upto_stream_test"> fill_upto_stream_test = [] {
  // Setup
  using namespace boost::ut;
  using namespace std::literals;

  "[fill_upto] normal usage"_test = []() {
    // Setup
    std::string_view str = "some data#_$more data";
    auto span = hal::as_bytes(str);
    std::string_view expected_str = "some data#_$";
    auto expected = hal::as_bytes(expected_str);

    std::array<hal::byte, 128> buffer;
    auto target_string = "#_$"sv;
    auto target = hal::as_bytes(target_string);
    hal::stream_fill_upto filler(target, buffer);

    // Exercise
    auto remaining = span | filler;

    // Verify
    expect(that % work_state::finished == filler.state());
    expect(
      that %
        span.subspan(str.find(target_string) + target_string.size()).size() ==
      remaining.size());
    expect(
      that %
        span.subspan(str.find(target_string) + target_string.size()).data() ==
      remaining.data());
    expect(that %
             span.subspan(0, str.find(target_string) + target_string.size())
               .size() ==
           filler.span().size());
    expect(std::equal(expected.begin(),
                      expected.end(),
                      buffer.begin(),
                      buffer.begin() + expected.size()));
  };

  "[fill_upto] two blocks usage"_test = []() {
    // Setup
    std::array<std::string_view, 2> str = { "some data#"sv, "_$more data"sv };
    std::string_view expected_str = "some data#_$";
    auto expected = hal::as_bytes(expected_str);
    std::array span{ hal::as_bytes(str[0]), hal::as_bytes(str[1]) };

    std::array<hal::byte, 128> buffer;
    hal::stream_fill_upto filler(hal::as_bytes("#_$"sv), buffer);

    // Exercise
    auto remaining0 = span[0] | filler;
    auto remaining1 = span[1] | filler;

    // Verify
    expect(that % work_state::finished == filler.state());
    expect(that % 0 == remaining0.size());
    expect(that % &(*span[0].end()) == remaining0.data());
    expect(that % (span[1].size() - 2) == remaining1.size());
    expect(that % (span[1].data() + 2) == remaining1.data());
    expect(std::equal(expected.begin(),
                      expected.end(),
                      buffer.begin(),
                      buffer.begin() + expected.size()));
  };
};

// =============================================================================
//
//                               |  Multi Stream Test  |
//
// =============================================================================
boost::ut::suite<"multi_stream_test"> multi_stream_test = [] {
  using namespace boost::ut;
  using namespace std::literals;

  "[✨ multi ✨] http request"_test = []() {
    std::string_view str = "HTTP/1.1 200 OK\r\n"
                           "Content-Encoding: gzip\r\n"
                           "Accept-Ranges: bytes\r\n"
                           "Age: 501138\r\n"
                           "Cache-Control: max-age=604800\r\n"
                           "Content-Type: text/html; charset=UTF-8\r\n"
                           "Date: Thu, 17 Nov 2022 06:19:47 GMT\r\n"
                           "Etag: \" 3147526947 \"\r\n"
                           "Expires: Thu, 24 Nov 2022 06:19:47 GMT\r\n"
                           "Last-Modified: Thu, 17 Oct 2019 07:18:26 GMT\r\n"
                           "Server: ECS (sab/56CE)\r\n"
                           "Vary: Accept-Encoding\r\n"
                           "X-Cache: HIT\r\n"
                           "Content-Length: 47\r\n"
                           "\r\n"
                           "<html><body><h1>Hello, World</h1></body></html>";

    auto input_data = hal::as_bytes(str);
    std::array<hal::byte, 1024> response_buffer;
    response_buffer.fill('.');

    hal::stream_find find_content_length(hal::as_bytes("Content-Length: "sv));
    hal::stream_parse<std::uint32_t> parse_body_length;
    hal::stream_fill_upto find_end_of_header(hal::as_bytes("\r\n\r\n"sv),
                                             response_buffer);

    [[maybe_unused]] auto start_of_body =
      input_data | find_content_length | parse_body_length;

    [[maybe_unused]] auto remaining = input_data | find_end_of_header;

    /*
    if (hal::terminated(find_end_of_header.state())) {
      hal::stream_fill fill_buffer(find_end_of_header.unfilled(),
                                    parse_body_length.value());
      remaining = remaining | fill_buffer;
    }

    printf("[Content-Length]: %u, %d\n",
           static_cast<int>(parse_body_length.value()),
           static_cast<int>(response_buffer.size() - remaining.size()));

    printf("response_buffer:\r\n%.*s\n",
           static_cast<int>(response_buffer.size() - remaining.size()),
           response_buffer.data());
    printf("remaining = %.*s\n",
           static_cast<int>(remaining.size()),
           remaining.data());
    */
  };
};
}  // namespace hal

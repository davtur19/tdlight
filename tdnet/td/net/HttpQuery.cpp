//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/net/HttpQuery.h"

#include "td/utils/misc.h"

#include <algorithm>

namespace td {

Slice HttpQuery::header(Slice key) const {
  auto it = std::find_if(headers_.begin(), headers_.end(),
                         [&key](const std::pair<MutableSlice, MutableSlice> &s) { return s.first == key; });
  return it == headers_.end() ? Slice() : it->second;
}

MutableSlice HttpQuery::arg(Slice key) const {
  auto it = std::find_if(args_.begin(), args_.end(),
                         [&key](const std::pair<MutableSlice, MutableSlice> &s) { return s.first == key; });
  return it == args_.end() ? MutableSlice() : it->second;
}

std::vector<std::pair<string, string>> HttpQuery::string_args() const {
  std::vector<std::pair<string, string>> res;
  for (auto &it : args_) {
    res.push_back(std::make_pair(it.first.str(), it.second.str()));
  }
  return res;
}

int HttpQuery::get_retry_after() const {
  auto value = header("retry-after");
  if (value.empty()) {
    return 0;
  }
  auto r_retry_after = to_integer_safe<int>(value);
  if (r_retry_after.is_error()) {
    return 0;
  }

  return td::max(0, r_retry_after.ok());
}

StringBuilder &operator<<(StringBuilder &sb, const HttpQuery &q) {
  switch (q.type_) {
    case HttpQuery::Type::EMPTY:
      sb << "EMPTY";
      return sb;
    case HttpQuery::Type::GET:
      sb << "GET";
      break;
    case HttpQuery::Type::POST:
      sb << "POST";
      break;
    case HttpQuery::Type::RESPONSE:
      sb << "RESPONSE";
      break;
  }
  if (q.type_ == HttpQuery::Type::RESPONSE) {
    sb << ":" << q.code_ << ":" << q.reason_;
  } else {
    sb << ":" << q.url_path_;
    for (auto &key_value : q.args_) {
      sb << ":[" << key_value.first << ":" << key_value.second << "]";
    }
  }
  if (q.keep_alive_) {
    sb << ":keep-alive";
  }
  sb << "\n";
  for (auto &key_value : q.headers_) {
    sb << key_value.first << "=" << key_value.second << "\n";
  }
  sb << "BEGIN CONTENT\n";
  sb << q.content_;
  sb << "END CONTENT\n";

  return sb;
}

}  // namespace td

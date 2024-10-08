#include "endpoint.hpp"
#include <boost/algorithm/string.hpp>

namespace keep_my_journal {
std::optional<endpoint_t::rule_iterator>
endpoint_t::get_rules(std::string const &target) {
  auto iter = m_endpoints.find(target);
  if (iter == m_endpoints.end())
    return std::nullopt;
  return iter;
}

std::optional<endpoint_t::rule_iterator>
endpoint_t::get_rules(boost::string_view const &target) {
  return get_rules(target.to_string());
}

void endpoint_t::construct_special_placeholder(
    endpoint_t::special_placeholders_t &placeholder, std::string const &route) {
  size_t from_pos = 0;
  auto index = route.find_first_of('{', from_pos);
  if (index == std::string::npos || index == 0)
    throw std::runtime_error{"A special route must have a placeholder"};

  std::string const prefix = route.substr(0, index);
  if (auto const str = utils::trimCopy(prefix); str.empty() || str == "/")
    throw std::runtime_error("A special placeholder must have a valid prefix");

  size_t end_of_placeholder;

  do {
    end_of_placeholder = route.find_first_of('}', index);
    if (end_of_placeholder == std::string::npos)
      throw std::runtime_error("end of placeholder not found");
    auto const str_length = end_of_placeholder - index - 1;
    auto const name = utils::trimCopy(route.substr(index + 1, str_length));

    if (name.empty())
      throw std::runtime_error("empty placeholder name is not allowed");

    placeholder.placeholders.push_back({name});
    from_pos = end_of_placeholder + 1;
    if (from_pos >= route.size())
      break;

    index = route.find_first_of('{', from_pos);
    // ensure the character before this is a '/'
    if (index != std::string::npos && route[index - 1] != '/') {
      throw std::runtime_error(
          "special placeholders should be separated by '/'");
    }
  } while (index != std::string ::npos);

  if (auto const str_length = route.length() - end_of_placeholder - 1;
      str_length > 0) {
    placeholder.suffix = route.substr(end_of_placeholder + 1, str_length);
    while (placeholder.suffix.back() == '/')
      placeholder.suffix.pop_back();
  }

  if (m_specialEndpoints.find(prefix) != m_specialEndpoints.end())
    throw std::runtime_error("the prefix '" + prefix + "' already exist");
  m_specialEndpoints[prefix] = std::move(placeholder);
}

std::optional<endpoint_t::special_placeholders_t>
endpoint_t::get_special_rules(std::string this_target) {
  while (this_target.back() == '/')
    this_target.pop_back();

  for (auto const &[prefix, placeholder] : m_specialEndpoints) {
    // check if the prefix match
    auto target_copy = this_target;
    if (!boost::starts_with(target_copy, prefix))
      continue;

    boost::erase_first(target_copy, prefix);

    // if there's a suffix, check if they match
    if (!placeholder.suffix.empty()) {
      if (!boost::ends_with(target_copy, placeholder.suffix))
        continue;
      boost::erase_last(target_copy, placeholder.suffix);
    }

    std::vector<std::string> split_result{};
    boost::split(split_result, target_copy,
                 [](auto const ch) { return ch == '/'; });
    if (split_result.size() != placeholder.placeholders.size())
      continue;
    special_placeholders_t res{placeholder};
    for (size_t i = 0; i < res.placeholders.size(); ++i)
      res.placeholders[i].value = split_result[i];
    return res;
  }

  return std::nullopt;
}

} // namespace keep_my_journal
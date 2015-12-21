/**
 *
 **/
#ifndef __EZTEMP_H__
#define __EZTEMP_H__

#include <boost/variant.hpp>
#include <boost/regex.hpp>

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <cctype>
#include <functional>

#include <eztemp_export.h>

namespace ez {

/**
 * @brief EZ Node
 * A variant type holding manipulated values.
 */
using node = boost::make_recursive_variant<
    std::nullptr_t, std::string, int, double, bool,
    std::vector<boost::recursive_variant_>,
    std::map<const std::string, boost::recursive_variant_>>::type;
/**
 * @brief EZ Array
 * A list (vector) of nodes.
 */
using array = std::vector<ez::node>;

/**
 * @brief EZ Dict
 **/
using dict = std::map<const std::string, node>;

class render_node_visitor: public boost::static_visitor<std::string>
{
public:
    render_node_visitor(const std::vector<std::string> & keys, int level = 1): boost::static_visitor<std::string>(), m_keys(keys), m_level(level) {}
    std::string operator()(std::nullptr_t) const
    {
        return "null";
    }
    std::string operator()(int val) const
    {
        return std::to_string(val);
    }
    std::string operator()(double val) const
    {
        return std::to_string(val);
    }
    std::string operator()(bool val) const
    {
        return val ? "true" : "false";
    }
    std::string operator()(const std::string & val) const
    {
        return val;
    }
    std::string operator ()(const std::map<const std::string,ez::node> & map) const
    {
        return boost::apply_visitor(render_node_visitor(m_keys, m_level + 1), map.at(m_keys.at(m_level)));
    }
    std::string operator ()(const node & var) const
    {
        return "";
    }
    std::string operator ()(const std::map<const std::string, boost::recursive_variant_> & var) const
    {
        return "";
    }
    template <typename T, typename U>
    std::string operator()( const T &, const U & ) const
    {
        return "";
    }
    template <typename T>
    std::string operator()( const T & lhs, const T & rhs ) const
    {
        return "";
    }

    /*
    std::string operator()(const std::map<const std::string, boost::recursive_variant_> & val) const
    {
        return "Cannot render dict";//return boost::apply_visitor(render_node_visitor(), val);
    }*/
private:
    std::vector<std::string> m_keys;
    int m_level;
};

using delim_type = std::pair<std::string, std::string>;

class token
{
public:
    enum class type {
        text, render, section,
    };
    token(token::type _type): m_type(_type) {}
    inline const token::type token_type() { return m_type; }
    virtual std::string render(const dict & context) = 0;
private:
    type m_type;
};

inline std::vector<std::string> split(const std::string &text, char sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

class text_token : public token
{
public:
    text_token(const std::string & text): token(token::type::text), m_text(text) {}
    std::string render(const dict & context) override { return m_text; }
private:
    std::string m_text;
};

class render_token : public token
{
public:
    render_token(const std::string & content):
        token(token::type::render),
        m_content(content.substr(m_start_tag.size()))
    {}
    std::string render(const dict& context) override;
    static bool is_token_start(const std::string & text) {
        return text.substr(0, m_start_tag.size()) == m_start_tag;
    }
    static bool is_token_end(const std::string & text) {
        return text.substr(0, m_end_tag.size()) == m_end_tag;
    }
    static inline const std::string & start_tag() { return m_start_tag; }
    static inline const std::string & end_tag() { return m_end_tag; }
private:
    std::string m_content;
    static std::string m_start_tag;
    static std::string m_end_tag;
};

class section_token : public token
{
public:
    section_token(const std::string & content):
        token(token::type::section),
        m_content(content.substr(m_start_tag.size()))
    {}
    std::string render(const dict& context) override { return ""; }
    std::vector<std::string> content() {
        std::vector<std::string> vec = split(m_content, ' ');
        std::for_each(vec.begin(), vec.end(), [](std::string & str){
            str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
        });
        vec.erase(std::remove(vec.begin(), vec.end(), ""), vec.end());
        return vec;
    }
    static bool is_token_start(const std::string & text) {
        return text.substr(0, m_start_tag.size()) == m_start_tag;
    }
    static bool is_token_end(const std::string & text) {
        return text.substr(0, m_end_tag.size()) == m_end_tag;
    }
    static inline const std::string & start_tag() { return m_start_tag; }
    static inline const std::string & end_tag() { return m_end_tag; }
private:
    std::string m_content;
    static std::string m_start_tag;
    static std::string m_end_tag;
};

using token_list = std::vector<std::shared_ptr<token>>;

class EZTEMP_EXPORT renderer
{
private:
    renderer();

    token_list make_token_list(const std::string & input);

public:
    static std::string render_json(const std::string & input, const std::string & context);

    static std::string render(const std::string & input, const dict & context);

    using render_function = std::function<std::string(const array)>;

    static std::map<std::string, render_function> m_functions;

    static void add_function(const std::string & key, const render_function & function)
    {
        m_functions[key] = function;
    }

    static std::string call_function(const std::string & key, const array & nodes)
    {
        return m_functions[key](nodes);
    }
};

inline void remove_whitespaces(std::string & str){
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
}

}

#endif // __EZTEMP_H__

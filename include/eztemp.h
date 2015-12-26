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
#include <fstream>

#include <eztemp_export.h>

namespace ez {

namespace temp {

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
using array = std::vector<ez::temp::node>;

/**
 * @brief EZ Dict
 **/
class dict: public std::map<const std::string, node>
{
public:
    dict(const std::initializer_list<std::pair<const std::string, node>> & init_lst)
    {
        for(auto & item: init_lst)
        {
            (*this)[item.first] = item.second;
        }
    }
    dict() {}
    static dict from_json(const std::string & json);
};

/**
 * @brief The token class
 **/
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

/**
 * @brief The text_token class
 **/
class text_token : public token
{
public:
    text_token(const std::string & text): token(token::type::text), m_text(text) {}
    std::string render(const dict & context) override { return m_text; }
private:
    std::string m_text;
};

/**
 * @brief The render_token class
 **/
class render_token : public token
{
public:
    render_token(const std::string & content);
    std::string render(const dict& context) override;
    static bool is_start(std::string::const_iterator start, const std::string::const_iterator & end);
    static bool is_end(std::string::const_iterator start, const std::string::const_iterator & end);
    static inline const std::string & start_tag() { return m_start_tag; }
    static inline const std::string & end_tag() { return m_end_tag; }
private:
    std::string m_content;
    static std::string m_start_tag;
    static std::string m_end_tag;
};

/**
 * @brief The section_token class
 **/
class section_token : public token
{
public:
    section_token(const std::string & content);
    std::string render(const dict& context) override { return ""; }
    inline const std::vector<std::string> & params() const { return m_params; }
    static bool is_start(std::string::const_iterator start, const std::string::const_iterator & end);
    static bool is_end(std::string::const_iterator start, const std::string::const_iterator & end);
    static inline const std::string & start_tag() { return m_start_tag; }
    static inline const std::string & end_tag() { return m_end_tag; }
private:
    std::string m_content;
    std::vector <std::string> m_params;
    static std::string m_start_tag;
    static std::string m_end_tag;
};

/**
 * @brief List of rendering tokens.
 **/
using compiled_template = std::vector<std::shared_ptr<token>>;

/**
 * @brief The renderer class.
 */
class EZTEMP_EXPORT renderer
{
public:
    /**
     * @brief Compile a template string.
     * @param input The input string.
     * @param path  The path to find extends templates.
     * @return The compiled template.
     **/
    static compiled_template compile(const std::string & input, const std::string & path = "");

    /**
     * @brief Compile a template file.
     * @param filepath  The path of the template file.
     * @return The compiled template.
     **/
    static compiled_template compile_file(const std::string & filepath);

    /**
     * @brief Render a template file.
     * @param filepath  The path of the template file.
     * @param context   The context as Json string.
     * @return The rendered template.
     **/
    static std::string render_file(const std::string & filepath, const std::string & context);

    /**
     * @brief Render a template string.
     * @param input     The input string.
     * @param context   The context dictionnary.
     * @return The rendered template.
     **/
    static std::string render(const std::string & input, const dict & context = dict());

    /**
     * @brief Render a template string.
     * @param input     The input string.
     * @param context   The context as Json string.
     * @return The rendered template.
     **/
    static std::string render(const std::string & input, const std::string & context);

    /**
     * @brief Render a compiled template.
     * @param input     The compiled template.
     * @param context   The context dictionnary.
     * @return The rendered template.
     */
    static std::string render(const ez::temp::compiled_template & input, const dict & context);

    /**
     * @brief Template rendering function definition.
     **/
    using render_function = std::function<std::string(const array)>;

    /**
     * @brief Add a template rendering function.
     * @param key       The key name of the given function.
     * @param function  The function to execute.
     **/
    static void add_function(const std::string & key, const render_function & function)
    {
        m_functions[key] = function;
    }

private:

    static std::string call_function(const std::string & key, const array & nodes)
    {
        return m_functions[key](nodes);
    }

    static std::map<std::string, render_function> m_functions;

    friend class render_token; // allow render_token to use call_function.
};

} // namespace temp

} // namespace ez

#endif // __EZTEMP_H__

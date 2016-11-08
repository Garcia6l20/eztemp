#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <functional>
#include <algorithm>
#include <memory>
#include <math.h>

#include <eztemp.h>
#include <ezexpr.h>

using namespace ez::temp;

#ifdef EZ_DEBUG
#define EZ_DPRINT(...)  fprintf(stderr, __VA_ARGS__)
#else
#define EZ_DPRINT(...)
#endif

std::map<std::string, renderer::render_function> renderer::m_functions;

std::string render_token::m_start_tag = "{{";
std::string render_token::m_end_tag = "}}";
std::string section_token::m_start_tag = "{%";
std::string section_token::m_end_tag = "%}";

/**
 * @brief The render_node_visitor class
 **/
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
    std::string operator ()(const std::map<const std::string, node> & map) const
    {
        try {
            return boost::apply_visitor(render_node_visitor(m_keys, m_level + 1), map.at(m_keys.at(m_level)));
        } catch (const std::out_of_range & e) {
            boost::cmatch what;
            if (boost::regex_match(m_keys.at(m_level).c_str(),
                                  what,
                                  boost::regex("(?<array>\\w{1,})\\[(?<num>\\d+)\\]")))
            {
                const std::string aname = what["array"].str();
                const ez::temp::array & a = boost::get<ez::temp::array>(map.at(aname));
                if (a.empty())
                {
                    throw renderer::render_exception(("Array: " + aname + " is empty.").c_str());
                }
                const node & n = a[atoi(what["num"].str().c_str())];
                return boost::apply_visitor(render_node_visitor({what["array"]}), n);
            }
            std::cerr << "key \"" << m_keys.at(m_level) << "\" not found" << std::endl;
            throw e;
        }
    }
    std::string operator ()(const array & var) const
    {
        throw renderer::render_exception("Tho shall not render an array !!!");
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
private:
    std::vector<std::string> m_keys;
    int m_level;
};


class get_type_node_visitor: public boost::static_visitor<std::string>
{
public:
    std::string operator()(std::nullptr_t) const
    {
        return "nullptr_t";
    }
    std::string operator()(int val) const
    {
        return "int";
    }
    std::string operator()(double val) const
    {
        return "double";
    }
    std::string operator()(bool val) const
    {
        return "bool";
    }
    std::string operator()(const std::string & val) const
    {
        return "string";
    }
    std::string operator ()(const std::map<const std::string, node> & map) const
    {
        return "dict";
    }
    std::string operator ()(const array & var) const
    {
        return "array";
    }
    /*std::string operator ()(const std::map<const std::string, boost::recursive_variant_> & var) const
    {
        return "";
    }*/
    template <typename T, typename U>
    std::string operator()( const T &, const U & ) const
    {
        return "unknown";
    }
    template <typename T>
    std::string operator()( const T & lhs, const T & rhs ) const
    {
        return "unknown";
    }
};

/**
 * @brief The bool_check_node_visitor class
 **/
class bool_check_node_visitor: public boost::static_visitor<bool>
{
public:
    bool_check_node_visitor(const std::vector<std::string> & keys, int level = 1): boost::static_visitor<bool>(), m_keys(keys), m_level(level) {}
    bool operator()(std::nullptr_t) const
    {
        return false;
    }
    bool operator()(int val) const
    {
        return val != 0;
    }
    bool operator()(double val) const
    {
        return val != 0.0;
    }
    bool operator()(bool val) const
    {
        return val;
    }
    bool operator()(const std::string & val) const
    {
        if(val == "true")
            return true;
        else if(val == "false")
            return false;
        else throw std::runtime_error("Not a boolean string");
    }
    bool operator ()(const std::map<const std::string, node> & map) const
    {
        /*if(m_keys.size() == 1)
        {
            // try to get the "value" key
            return boost::apply_visitor(bool_check_node_visitor(m_keys, m_level + 1), map.at("value"));
        }
        else
        {*/
            return boost::apply_visitor(bool_check_node_visitor(m_keys, m_level + 1), map.at(m_keys.at(m_level)));
        //}
    }
    bool operator ()(const node & var) const
    {
        throw std::runtime_error("Not a boolean");
    }
    bool operator ()(const std::map<const std::string, boost::recursive_variant_> & var) const
    {
        throw std::runtime_error("Not a boolean");
    }
    template <typename T, typename U>
    bool operator()( const T &, const U & ) const
    {
        throw std::runtime_error("Not a boolean");
    }
    template <typename T>
    bool operator()( const T & lhs, const T & rhs ) const
    {
        throw std::runtime_error("Not a boolean");
    }
private:
    std::vector<std::string> m_keys;
    int m_level;
};

/**
 * @brief remove_whitespaces
 * @param str
 */
inline void remove_whitespaces(std::string & str)
{
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
}

/**
 * @brief get_next_section
 * @param tokens
 * @param name
 * @param section
 * @param start_index
 * @return
 */
inline static
int get_next_section(const compiled_template & tokens, section_token::section_type_t section_type, std::shared_ptr<section_token> & section,int start_index = 0)
{
    for(int ii = start_index; ii < tokens.size(); ++ii)
    {
        if(tokens[ii]->token_type() == token::type::section)
        {
            section = std::dynamic_pointer_cast<section_token>(tokens[ii]);
            if(section->section_type() == section_type)
            {
                return ii;
            }
        }
    }
    return -1;
}

/**
 * @brief get_next_block
 * @param list
 * @param start_index
 * @param block_name
 * @return
 */
inline static
int get_next_block(compiled_template & list, int start_index, std::string & block_name)
{
    std::shared_ptr<section_token> section;
    int index;
    if((index = get_next_section(list, section_token::block_open, section, start_index)) != -1)
    {
        block_name = section->params()[1];
        return index;
    }
    else return -1;
}

/**
 * @brief get_next_endblock
 * @param list
 * @param start_index
 * @return
 */
inline static
int get_next_endblock(compiled_template & list, int start_index)
{
    std::shared_ptr<section_token> section;
    int index;
    if((index = get_next_section(list, section_token::block_close, section, start_index)) != -1)
    {
        return index;
    }
    else return -1;
}

/**
 * @brief get_named_block
 * @param list
 * @param block_name
 * @return
 */
inline static
int get_named_block (compiled_template & list,std::string block_name)
{
    std::shared_ptr<section_token> section;
    int index = 0;
    while((index = get_next_section(list, section_token::block_open, section, index)) != -1)
    {
        if(block_name == section->params()[1])
            return index;
        index += 1;
    }
    return -1;
}

// --------------------------------------------
// render_token stuff
//

render_token::render_token(const std::string &content):
    token(token::type::render),
    m_content(std::string(content.begin() + m_end_tag.size(), content.end() - m_end_tag.size()))
{}

std::string render_token::render(const dict & context){
   std::string full_key = m_content;
   remove_whitespaces(full_key);
   static const boost::regex expr("([a-zA-Z_]+)\\((.*)\\)$");
   boost::cmatch what;
   if(boost::regex_match(full_key.c_str(), what, expr))
   {
       array nl;
       std::string function_name = what[1];
       std::string params_raw = what[2];
       remove_whitespaces(params_raw);
       std::vector<std::string> params = ez::split(params_raw,',');
       for(const std::string & p: params)
       {
           if(!p.empty())
           {
               std::vector<std::string> keys = split(p,'.');

               node tmp_node = context.at(keys[0]);
               for (int ii = 1; ii < keys.size(); ++ii)
               {
                   std::map<const std::string, node> d = boost::get<std::map<const std::string, node>>(tmp_node);
                   tmp_node = d.at(keys[ii]);
               }

               nl.push_back(tmp_node);
           }
       }
       return renderer::call_function(function_name, nl);
   }
   else
   {
       const std::vector<std::string> keys = split(full_key, '.');
       try {
            return boost::apply_visitor(render_node_visitor(keys), context.at(keys.at(0)));
       } catch(const std::out_of_range &e) {
           /*if (boost::regex_match(keys,
                                  what,
                                  boost::regex("\\w{1,}\\[(?<num>\\d+)\\]")))
           {
               const ez::temp::array & a = boost::get<>
           }*/
           std::stringstream ss;
           ss << "ez::temp::render: no such key: ";
           ss << full_key;
           throw renderer::render_exception(ss.str().c_str());
       }
   }
}

bool render_token::is_start(std::string::const_iterator start, const std::string::const_iterator &end)
{
    for(std::string::const_iterator it = m_start_tag.begin(); it < m_start_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}
bool render_token::is_end(std::string::const_iterator start, const std::string::const_iterator &end)
{
    for(std::string::const_iterator it = m_end_tag.begin(); it < m_end_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}

// --------------------------------------------
// section_token stuff
//

section_token::section_token(const std::string & content):
    token(token::type::section),
    m_content(std::string(content.begin() + m_start_tag.size(), content.end() - m_end_tag.size())),
    m_section_type(unknown)
{
    m_params = split(m_content, ' ');

    // remove all white spaces
    std::for_each(m_params.begin(), m_params.end(), [](std::string & str){
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    });

    // remove all blank params
    m_params.erase(std::remove(m_params.begin(), m_params.end(), ""), m_params.end());

    if (m_params[0] == "if")
    {
        m_section_type = if_open;
    }
    else if (m_params[0] == "elif")
    {
        m_section_type = if_else_if;
    }
    else if (m_params[0] == "else")
    {
        m_section_type = if_else;
    }
    else if (m_params[0] == "endif")
    {
        m_section_type = if_close;
    }
    else if (m_params[0] == "for")
    {
        m_section_type = for_open;
    }
    else if (m_params[0] == "elsefor")
    {
        m_section_type = for_else;
    }
    else if (m_params[0] == "endfor")
    {
        m_section_type = for_close;
    }
    else if (m_params[0] == "block")
    {
        m_section_type = block_open;
    }
    else if (m_params[0] == "blockend")
    {
        m_section_type = block_close;
    }
    else if (m_params[0] == "extends")
    {
        m_section_type = extends;
    }
    else
    {
        std::stringstream ss;
        ss << "Unknown section tag: ";
        ss << m_params[0];
        throw renderer::render_exception(ss.str().c_str());
    }
}

bool section_token::is_start(std::string::const_iterator start, const std::string::const_iterator &end)
{
    for(std::string::const_iterator it = m_start_tag.begin(); it < m_start_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}

bool section_token::is_end(std::string::const_iterator start, const std::string::const_iterator &end)
{
    for(std::string::const_iterator it = m_end_tag.begin(); it < m_end_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}

// --------------------------------------------
// renderer stuff
//

dict renderer::make_dict(const std::string &json)
{
    dict context;

    std::stringstream ss;
    ss << json;
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    using boost::property_tree::ptree;
    std::function<void(const std::string&, ptree&, dict &, const std::string&)> parse_node;
    parse_node = [&parse_node](const std::string& key, ptree & pt, dict & context, const std::string& parent_key){
        if(pt.empty())
        {
            if(key.empty())
            {
                if(context.find(parent_key) != context.end())
                {
                    boost::get<std::vector<node>>(context[parent_key]).push_back(pt.data());
                }
                else
                {
                    context[parent_key] = std::vector<node>{pt.data()};
                }
            }
            else
            {
                context[key] = pt.data();
            }
        }
        else
        {
            for (ptree::iterator node = pt.begin(); node != pt.end(); ++node)
            {
                parse_node(node->first, node->second, context, key);
            }
        }
    };

    parse_node("", pt, context, "");
    return context;
}

token::ptr renderer::compile_file(const std::string &file_path)
{
    std::ifstream fs(file_path);
    std::string path = boost::filesystem::path(file_path).remove_filename().string();
    if(path.empty())
        path = ".";
    return compile(std::string(std::istreambuf_iterator<char>(fs),std::istreambuf_iterator<char>()), path);
}

inline void push_text_if_required(std::string::const_iterator begin, std::string::const_iterator end, token::ptr parent)
{
    if(begin < end)
    {
        token::ptr t(new text_token(std::string(begin, end)));
        parent->add_child(t);
    }
}


void _compile(std::string::const_iterator & it,
             const std::string::const_iterator & end,
             std::string::const_iterator text_tok_start,
             token::ptr parent)
{
    for (; it < end; ++it)
    {
        if(render_token::is_start(it, end))
        {
            std::string::const_iterator it_start = it;
            for(; it < end; ++it)
            {
                if(render_token::is_end(it, end))
                {
                    push_text_if_required(text_tok_start, it_start, parent);
                    it += render_token::end_tag().size();
                    token::ptr t(new render_token(std::string(it_start, it)));
                    parent->add_child(t);
                    text_tok_start = it;
                    --it;
                    break;
                }
            }
        }
        else if(section_token::is_start(it, end))
        {
            std::string::const_iterator it_start = it;
            for(; it < end; ++it)
            {
                if(section_token::is_end(it, end))
                {
                    // clean pre-section (remove spaces)

                    std::string str(text_tok_start, it_start);
                    int pos = std::distance(text_tok_start, it_start);
                    int prev_nl = str.rfind('\n', pos);
                    bool remove = false;
                    if(prev_nl != std::string::npos)
                    {
                        int dist = pos - prev_nl;
                        remove = true;
                        if(dist != 0)
                        {
                            for(std::string::const_iterator it = it_start - dist + 1; it < it_start; ++it)
                            {
                                if((*it) != ' ' && (*it) != '\t')
                                {
                                    remove = false;
                                    break;
                                }
                            }
                        }
                        else remove = false;
                        if(remove)
                        {
                            push_text_if_required(text_tok_start, it_start - dist, parent);
                        }
                        else
                        {
                            push_text_if_required(text_tok_start, it_start, parent);
                        }
                    }
                    else
                    {
                        push_text_if_required(text_tok_start, it_start, parent);
                    }


                    it += section_token::end_tag().size();
                    section_token * sec = new section_token(std::string(it_start, it));
                    token::ptr t(sec);

                    switch (sec->section_type())
                    {
                        case section_token::for_open:
                        case section_token::if_open:
                            parent->add_child(t);
                            text_tok_start = it;
                            _compile(it, end, text_tok_start, t);
                        break;
                        case section_token::for_else:
                        {
                            section_token * parent_sec = std::dynamic_pointer_cast<section_token>(parent).get();
                            if (!parent_sec || parent_sec->section_type() != section_token::for_open)
                            {
                                throw renderer::render_exception("ez::temp::compile: \"elsefor\" found without \"for\"");
                            }
                            else
                            {
                                parent_sec->parent()->add_child(t);
                                text_tok_start = it;
                                _compile(it, end, text_tok_start, t);
                                return;
                            }
                        }
                        case section_token::if_else:
                        {
                            section_token * parent_sec = std::dynamic_pointer_cast<section_token>(parent).get();
                            if (!parent_sec || parent_sec->section_type() != section_token::if_open)
                            {
                                throw renderer::render_exception("ez::temp::compile: \"else\" found without \"if\"");
                            }
                            else
                            {
                                parent_sec->parent()->add_child(t);
                                text_tok_start = it;
                                _compile(it, end, text_tok_start, t);
                                return;
                            }
                        }
                        break;
                        case section_token::for_close:
                        {
                            section_token * parent_sec = std::dynamic_pointer_cast<section_token>(parent).get();
                            if (!parent_sec || (
                                    parent_sec->section_type() != section_token::for_open &&
                                    parent_sec->section_type() != section_token::for_else))
                            {
                                throw renderer::render_exception("ez::temp::compile: \"endfor\" found without \"for\"");
                            }
                            else
                            {
                                parent_sec->parent()->add_child(t);
                                return;
                            }
                        }
                        break;
                        case section_token::if_close:
                        {
                            section_token * parent_sec = std::dynamic_pointer_cast<section_token>(parent).get();
                            if (!parent_sec || (
                                    parent_sec->section_type() != section_token::if_open &&
                                    parent_sec->section_type() != section_token::if_else_if &&
                                    parent_sec->section_type() != section_token::if_else))
                            {
                                throw renderer::render_exception("ez::temp::compile: \"endif\" found without \"if\"");
                            }
                            else
                            {
                                parent_sec->parent()->add_child(t);
                                return;
                            }
                        }
                        break;
                        default:
                            {
                                std::stringstream ss;
                                ss << "ez::temp::compile: unhandled tag: ";
                                ss << sec->section_type();
                                throw renderer::render_exception(ss.str().c_str());
                            }
                        break;
                    }

                    text_tok_start = it;
                    --it;
                    break;
                }
            }
        }
    }
    push_text_if_required(text_tok_start, end, parent);
}

void _print_tokens(const token::ptr &root, int level = 0)
{
    if (level)
    {
        for (int ii = 0; ii < level; ++ii)
            std::cout << "  ";
        std::cout << "|--> ";
    }
    switch (root->token_type())
    {
    case token::type::text:
        std::cout << "text token: " << std::dynamic_pointer_cast<text_token>(root)->content() << std::endl;
    break;
    case token::type::render:
        std::cout << "render token: " << std::dynamic_pointer_cast<render_token>(root)->content() << std::endl;
    break;
    case token::type::section:
        std::cout << "section token: " << std::dynamic_pointer_cast<section_token>(root)->content() << std::endl;
    break;
    case token::type::root:
        std::cout << "root token"  << std::endl;
    break;
    }
    for (const token::ptr & t: root->children())
    {
        _print_tokens(t, level + 1);
    }
}

token::ptr renderer::compile(const std::string &input, const std::string & path)
{
    compiled_template tokens;
    token::ptr root(new root_token);
    std::string::const_iterator start = input.begin();
    _compile(start, input.end(), start, root);
    _print_tokens(root);
    return root;
}

std::string renderer::render(const std::string & input, const std::string & context)
{
    return render(input, make_dict(context));
}

std::string renderer::render(const std::string & input, const dict & context)
{
    return render(renderer::compile(input), context);
}


std::string renderer::render_file(const std::string & input, const std::string & context)
{
    return render(compile_file(input), make_dict(context));
}

int process_tokens(const token::ptr & root, std::string & output, const dict & context, int ii_start, bool break_on_endfor, const dict & parent_loop_ctx)
{
    bool process_elsefor = false;

    const std::vector<token::ptr> & toks = root->children();

    for(int ii = 0; ii < toks.size(); ++ii)
    {
        switch(toks[ii]->token_type())
        {
        case token::type::section:
            {
                std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                if(open_sec->section_type() == section_token::for_open)
                {
                    EZ_DPRINT("ez::temp::render: opening section: \"%s\"\n", open_sec->content().c_str());

                    // process the for loop
                    dict for_context = context;
                    std::vector<std::string> container_id = ez::split(open_sec->params()[3], '.');

                    try
                    {
                        node tmp_node = for_context.at(container_id[0]);
                        for (int ii = 1; ii < container_id.size(); ++ii)
                        {
                            std::map<const std::string, node> d = boost::get<std::map<const std::string, node>>(tmp_node);
                            tmp_node = d.at(container_id[ii]);
                        }

                        ez::temp::array & array = boost::get<std::vector<node>>(tmp_node);
                        int index = 0;
                        int size = array.size();
                        dict loop_context = dict();
                        loop_context["length"] = size;
                        loop_context["parent"] = parent_loop_ctx;

                        if (array.size())
                        {
                            for(const node & _node: array)
                            {
                                loop_context["index"] = index + 1;
                                loop_context["index0"] = index;
                                loop_context["first"] = index == 0;
                                loop_context["last"] = index == size - 1;
                                loop_context["revindex"] = size - index;
                                loop_context["revindex0"] = size - index - 1;
                                for_context["loop"] = loop_context;
                                for_context[open_sec->params()[1]] = _node;

                                ++index;
                                //ii_last = process_tokens(toks[ii], output, for_context, ii + 1, true, loop_context);
                                process_tokens(toks[ii], output, for_context, ii + 1, true, loop_context);
                            }
                        }
                        else
                        {
                            EZ_DPRINT("ez::temp::render: section end: \"%s\" (empty), ", open_sec->content().c_str());
                            // jump to elsefor/endfor section
                            std::shared_ptr<section_token> tmp_sec;
                            int else_index = get_next_section(toks, section_token::for_else, tmp_sec, ii + 1);
                            int for_close_index = get_next_section(toks, section_token::for_close, tmp_sec, ii + 1);
                            if(else_index != -1 && else_index < for_close_index)
                            {
                                EZ_DPRINT("jumping to elsefor\n");
                                ii = else_index;
                                process_tokens(toks[ii], output, for_context, ii + 1, true, loop_context);
                                ii = for_close_index;
                            }
                        }
                    } catch(const std::out_of_range& e) {
                        std::stringstream ss;
                        ss << "ez::temp::render: array not found:\"" << open_sec->params()[3] << "\"" << std::endl;
                        throw renderer::render_exception(ss.str().c_str());
                    } catch(const boost::bad_get& e) {
                        std::stringstream ss;
                        ss << "ez::temp::render: \"" << open_sec->params()[3] << "\" is not an array" << std::endl;
                        throw renderer::render_exception(ss.str().c_str());
                    }

                }
                else if (open_sec->section_type() == section_token::for_else)
                {
                    EZ_DPRINT("ez::temp::render: section elsefor\n");
                }
                else if (open_sec->section_type() == section_token::for_close)
                {
                    EZ_DPRINT("ez::temp::render: section endfor\n");
                }
                else if(open_sec->section_type() == section_token::if_open)
                {
                    EZ_DPRINT("ez::temp::render: section if: \"%s\"\n", open_sec->content().c_str());
                    int ii_param = 1;
                    bool check_request = true;
                    if(open_sec->params()[1] == "not")
                    {
                        check_request = false;
                        ii_param++;
                    }

                    if (open_sec->params()[ii_param] == "class.flags")
                        std::cout << std::endl;

                    std::vector<std::string> keys = ez::split(open_sec->params()[ii_param], '.');

                    bool result = false;
                    std::vector<std::string> eval_elems(open_sec->params().begin() + ii_param, open_sec->params().end());
                    std::string eval_str = boost::join(eval_elems, " ");
                    result = ez::expr::eval<bool>(eval_str, context);
                    //result = boost::apply_visitor(bool_check_node_visitor(keys), context.at(keys.at(0)));
                    if(result != check_request)
                    {
                        // jump to else section
                        std::shared_ptr<section_token> tmp_sec;

                        int else_index = get_next_section(toks, section_token::if_else, tmp_sec, ii + 1);
                        int if_close_index = get_next_section(toks, section_token::if_close, tmp_sec, ii + 1);
                        if (else_index != -1 && else_index < if_close_index)
                        {
                            process_tokens(toks[else_index], output, context, ii, false, parent_loop_ctx);
                            ii = if_close_index;
                        }
                    }
                    else
                    {
                        process_tokens(toks[ii], output, context, ii, false, parent_loop_ctx);
                    }
                }
                else if (open_sec->section_type() == section_token::if_else)
                {
                    EZ_DPRINT("ez::temp::render: section else\n");
                }
                else if (open_sec->section_type() == section_token::if_close)
                {
                    EZ_DPRINT("ez::temp::render: section endif\n");
                }
                else
                {
                    std::stringstream ss;
                    ss << "Unknown tag: ";
                    ss << open_sec->params()[0];
                    throw renderer::render_exception(ss.str().c_str());
                }
            }
            break;
        default:
            {
                EZ_DPRINT("ez::temp::render: rendering: \"%s\"\n", toks[ii]->render(context).c_str());
                output += toks[ii]->render(context);
            }
        }
    }
    return 0;
}

std::string renderer::render(const token::ptr & root, const dict & context)
{
    std::string output;
    process_tokens(root, output, context, 0, false, dict());
    return output;
}


// -----------------------------------------
// renderer functions registration
//

static bool register_functions()
{
    renderer::add_function("date",[](array args) -> std::string{
       namespace pt = boost::posix_time;
       namespace gr = boost::gregorian;
       pt::ptime todayUtc(gr::day_clock::universal_day(), pt::second_clock::universal_time().time_of_day());
       return pt::to_simple_string(todayUtc);
    });

    renderer::add_function("toupper",[](array args) -> std::string{
        std::string str = boost::get<std::string>(args[0]);
        boost::to_upper(str);
        return str;
    });

    return true;
}

static bool function_registered = register_functions();

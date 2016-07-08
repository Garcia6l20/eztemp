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
#include <math.h>

#include <eztemp.h>

using namespace ez::temp;

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
        return boost::apply_visitor(render_node_visitor(m_keys, m_level + 1), map.at(m_keys.at(m_level)));
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
int get_next_section(const compiled_template & tokens, const std::string & name, std::shared_ptr<section_token> & section,int start_index = 0)
{
    for(int ii = start_index; ii < tokens.size(); ++ii)
    {
        if(tokens[ii]->token_type() == token::type::section)
        {
            section = std::dynamic_pointer_cast<section_token>(tokens[ii]);
            if(section->params()[0] == name)
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
    if((index = get_next_section(list, "block", section, start_index)) != -1)
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
    if((index = get_next_section(list, "endblock", section, start_index)) != -1)
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
    while((index = get_next_section(list, "block", section, index)) != -1)
    {
        if(block_name == section->params()[1])
            return index;
        index += 1;
    }
    return -1;
}

// --------------------------------------------
// dict stuff
//

dict dict::from_json(const std::string &json)
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

// --------------------------------------------
// render_token stuff
//

render_token::render_token(const std::string & content):
    token(token::type::render),
    m_content(std::string(content.begin() + m_start_tag.size(), content.end() - m_end_tag.size()))
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
       std::vector<std::string> params = split(params_raw,',');
       for(const std::string & p: params)
       {
           if(!p.empty())
           {
               const node & _n = context.at(p);
               nl.push_back(_n);
           }
       }
       return renderer::call_function(function_name, nl);
   }
   else
   {
       const std::vector<std::string> keys = split(full_key, '.');
       return boost::apply_visitor(render_node_visitor(keys), context.at(keys.at(0)));
   }
}

bool render_token::is_start(std::string::const_iterator start, const std::string::const_iterator & end)
{
    for(std::string::const_iterator it = m_start_tag.begin(); it < m_start_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}
bool render_token::is_end(std::string::const_iterator start, const std::string::const_iterator & end)
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
    m_content(std::string(content.begin() + m_start_tag.size(), content.end() - m_end_tag.size()))
{
    m_params = split(m_content, ' ');

    // remove all white spaces
    std::for_each(m_params.begin(), m_params.end(), [](std::string & str){
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    });

    // remove all blank params
    m_params.erase(std::remove(m_params.begin(), m_params.end(), ""), m_params.end());
}

bool section_token::is_start(std::string::const_iterator start, const std::string::const_iterator & end)
{
    for(std::string::const_iterator it = m_start_tag.begin(); it < m_start_tag.end(); ++it, ++start)
    {
        if((*it) != *(start))
            return false;
    }
    return true;
}

bool section_token::is_end(std::string::const_iterator start, const std::string::const_iterator & end)
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

compiled_template renderer::compile_file(const std::string &file_path)
{
    std::ifstream fs(file_path);
    std::string path = boost::filesystem::path(file_path).remove_filename().string();
    if(path.empty())
        path = ".";
    return compile(std::string(std::istreambuf_iterator<char>(fs),std::istreambuf_iterator<char>()), path);
}

compiled_template renderer::compile(const std::string &input, const std::string & path)
{
    compiled_template tokens;

    auto push_text_if_required = [&input](compiled_template & list, std::string::const_iterator begin, std::string::const_iterator end){
        if(begin != end)
        {
            list.push_back(std::shared_ptr<token>(new text_token(std::string(begin, end))));
        }
    };

    std::string::const_iterator last_it = input.begin();
    std::string::const_iterator it = input.begin();
    for(; it < input.end(); ++it)
    {
        if(render_token::is_start(it, input.end()))
        {
            std::string::const_iterator it_start = it;
            for(; it < input.end(); ++it)
            {
                if(render_token::is_end(it, input.end()))
                {
                    push_text_if_required(tokens, last_it, it_start);
                    it += render_token::end_tag().size();
                    tokens.push_back(std::shared_ptr<token>(new render_token(std::string(it_start, it))));
                    last_it = it;
                    --it;
                    break;
                }
            }
        }
        else if(section_token::is_start(it, input.end()))
        {
            std::string::const_iterator it_start = it;
            for(; it < input.end(); ++it)
            {
                if(section_token::is_end(it, input.end()))
                {
                    // clean pre-section (remove spaces)
                    int pos = std::distance(input.begin(), it_start);
                    int prev_nl = input.rfind('\n', pos);
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
                            push_text_if_required(tokens, last_it, it_start - dist);
                        }
                        else
                        {
                            push_text_if_required(tokens, last_it, it_start);
                        }
                    }
                    else
                    {
                        push_text_if_required(tokens, last_it, it_start);
                    }

                    it += section_token::end_tag().size();
                    tokens.push_back(std::shared_ptr<token>(new section_token(std::string(it_start, it))));
                    last_it = it;

                    --it;
                    break;
                }
            }
        }
    }
    push_text_if_required(tokens, last_it, it);

    // check for extends
    std::string extending_base;

    std::shared_ptr<section_token> section;
    int index;
    if((index = get_next_section(tokens, "extends", section, 0)) >= 0)
    {
        // extending ...

        extending_base = section->params()[1];
        compiled_template base_tokens = compile_file(path + "/" + extending_base + ".ez" );

        std::string block_name;

        int index_base = 0;
        int index = 0;
        while((index_base = get_next_block(base_tokens, index_base, block_name)) != -1)
        {
            int index_endblock_base = get_next_endblock(base_tokens, index_base);

            if((index = get_named_block(tokens, block_name)) != -1)
            {
                // erase base content
                base_tokens.erase(base_tokens.begin() + index_base, base_tokens.begin() + index_endblock_base);

                int index_endblock = get_next_endblock(tokens, index);
                base_tokens.insert(base_tokens.begin() + index_base, tokens.begin() + index + 1, tokens.begin() + index_endblock);
                index = 0;
            }
            else
            {
                base_tokens.erase(base_tokens.begin() + index_endblock_base);   // remove the endblock section
                base_tokens.erase(base_tokens.begin() + index_base);            // remove the block section
            }
        }
        tokens = base_tokens;
    }

    return tokens;
}

std::string renderer::render(const std::string & input, const std::string & context)
{
    return render(input, dict::from_json(context));
}

std::string renderer::render(const std::string & input, const dict & context)
{
    return render(renderer::compile(input), context);
}


std::string renderer::render_file(const std::string & input, const std::string & context)
{
    return render(compile_file(input), dict::from_json(context));
}

int process_tokens(const compiled_template & toks, std::string & output, const dict & context, int ii_start, bool break_on_endfor, const dict & parent_loop_ctx)
{
    bool loop_done = false;
    int ii;
    int ii_last = ii_start;

    for(ii = ii_start; ii < toks.size(); ++ii)
    {
        switch(toks[ii]->token_type())
        {
        case token::type::section:
            {
                std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                if(open_sec->params()[0] == "for")
                {
                    // process the for loop
                    dict for_context = context;
                    std::vector<std::string> container_id = split(open_sec->params()[3], '.');

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
                            ii_last = process_tokens(toks, output, for_context, ii + 1, true, loop_context);
                        }
                        ii = ii_last;
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
                else if(open_sec->params()[0] == "endfor")
                {
                    ii_last = ii;
                    loop_done = true;
                    return ii_last;
                }
                else if(open_sec->params()[0] == "if")
                {
                    int ii_param = 1;
                    bool check_request = true;
                    if(open_sec->params()[1] == "not")
                    {
                        check_request = false;
                        ii_param++;
                    }

                    std::vector<std::string> keys = split(open_sec->params()[ii_param], '.');

                    bool result = boost::apply_visitor(bool_check_node_visitor(keys), context.at(keys.at(0)));
                    if(result != check_request)
                    {
                        // jump to else section
                        std::shared_ptr<section_token> tmp_sec;
                        int endif_index = get_next_section(toks, "endif", tmp_sec, ii);
                        int else_index = get_next_section(toks, "else", tmp_sec, ii);
                        if(else_index != -1 && else_index < endif_index)
                            ii = else_index;
                        else ii = endif_index;
                    }
                }
                else if(open_sec->params()[0] == "else")
                {
                    // jump to endif section
                    std::shared_ptr<section_token> endif_sec;
                    ii = get_next_section(toks, "endif", endif_sec, ii);
                }
            }
            break;
        default:
            {
                output += toks[ii]->render(context);
            }
        }
    }
    return ii_last;
}

std::string renderer::render(const compiled_template & toks, const dict & context)
{
    std::string output;
    process_tokens(toks, output, context, 0, false, dict());
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

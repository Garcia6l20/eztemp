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
std::string section_token::m_start_tag = "{#";
std::string section_token::m_end_tag = "#}";

std::string render_token::render(const dict & context){
   std::string full_key = m_content;
   remove_whitespaces(full_key);
   static const boost::regex expr("([a-zA-Z]+)\\((.*)\\)$");
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
       std::vector<std::string> keys = split(full_key, '.');
       return boost::apply_visitor(render_node_visitor(keys), context.at(keys.at(0)));
   }
}

compiled_template renderer::compile_file(const std::string &file_path)
{
    std::ifstream fs(file_path);
    return compile(std::string(std::istreambuf_iterator<char>(fs),std::istreambuf_iterator<char>()), boost::filesystem::path(file_path).remove_filename().string());
}

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

compiled_template renderer::compile(const std::string &input, const std::string & path)
{
    compiled_template tokens;
    int prev_pos = 0;
    int curr_pos = 0;

    auto push_text_token_if_required = [&curr_pos, &prev_pos, &input](compiled_template & list){
        if(curr_pos != prev_pos)
        {
            list.push_back(std::shared_ptr<token>(new text_token(input.substr(prev_pos,curr_pos-prev_pos))));
        }
    };
    char * p = (char*)input.c_str();
    for(; curr_pos < input.size(); ++curr_pos)
    {
        p = (char*)input.c_str() + curr_pos;

        if(render_token::is_token_start(input.substr(curr_pos)))
        {
            for(int jj = 0; jj < input.size()-curr_pos; ++jj)
            {
                if(render_token::is_token_end(input.substr(curr_pos+jj)))
                {
                    push_text_token_if_required(tokens);
                    tokens.push_back(std::shared_ptr<token>(new render_token(input.substr(curr_pos,jj))));
                    curr_pos += jj + render_token::end_tag().size();
                    prev_pos = curr_pos;
                    --curr_pos;
                    break;
                }
            }
        }
        else if(section_token::is_token_start(input.substr(curr_pos)))
        {
            for(int jj = 0; jj < input.size()-curr_pos; ++jj)
            {
                if(section_token::is_token_end(input.substr(curr_pos+jj)))
                {
                    push_text_token_if_required(tokens);
                    tokens.push_back(std::shared_ptr<token>(new section_token(input.substr(curr_pos,jj))));
                    curr_pos += jj + section_token::end_tag().size();
                    prev_pos = curr_pos;
                    --curr_pos;
                    break;
                }
            }
        }
    }
    push_text_token_if_required(tokens);

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
                base_tokens.erase(base_tokens.begin() + index_base);            // remove the block section
                base_tokens.erase(base_tokens.begin() + index_endblock_base);   // remove the endblock section
            }
        }
        tokens = base_tokens;
    }

    return tokens;
}

std::string renderer::render(const std::string & input, const std::string & context)
{
    std::stringstream ss;
    ss << context;
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);

    dict real_context;

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

    parse_node("", pt, real_context, "");

    return render(input, real_context);
}

std::string renderer::render(const std::string & input, const dict & context)
{
    return render(renderer::compile(input), context);
}


std::string renderer::render(const compiled_template & toks, const dict & context)
{
    std::string output;

    std::function<void(const compiled_template & toks, const dict & context, const std::vector<std::string> &, int &, std::string &)> process_for_loop;
    process_for_loop = [&output, &process_for_loop](const compiled_template & toks, const dict & context, const std::vector<std::string> & params, int & index, std::string & output){
        dict for_context = context;
        std::vector<node> & array = boost::get<std::vector<node>>(for_context.at(params[3]));
        int ii_last = index + 1;
        for(const node & _node: array)
        {
            for_context[params[1]] = _node;

            bool loop_done = false;
            int ii;
            for(ii = index + 1; ii < toks.size() && !loop_done; ++ii)
            {
                int for_index;
                std::shared_ptr<section_token> section;

                switch(toks[ii]->token_type())
                {
                case token::type::section:
                    {
                        std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                        if(open_sec->params()[0] == "for")
                        {
                            process_for_loop(toks, for_context, open_sec->params(), ii, output);
                        }
                        else if(open_sec->params()[0] == "endfor")
                        {
                            ii_last = ii;
                            loop_done = true;
                            break;
                        }
                    }
                    break;
                default:
                    output += toks[ii]->render(for_context);
                }
            }
        }
        index = ii_last;
    };

    auto process_tokens = [&context, &process_for_loop](const compiled_template & toks, std::string & output){
        for(int ii = 0; ii < toks.size(); ++ii)
        {
            switch(toks[ii]->token_type())
            {
            case token::type::section:
                {
                    std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                    if(open_sec->params()[0] == "for")
                    {
                        process_for_loop(toks, context, open_sec->params(), ii, output);
                    }
                }
                break;
            default:
                output += toks[ii]->render(context);
            }
        }
    };
    process_tokens(toks, output);
    return output;
}


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

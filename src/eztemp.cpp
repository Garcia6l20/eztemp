#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <functional>
#include <algorithm>
#include <math.h>

#include <eztemp.h>

using namespace ez;

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

renderer::renderer()
{
}

token_list renderer::make_token_list(const std::string &input)
{
    token_list list;
    int prev_pos = 0;
    int curr_pos = 0;

    auto push_text_token_if_required = [&curr_pos, &prev_pos, &input](token_list & list){
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
                    push_text_token_if_required(list);
                    list.push_back(std::shared_ptr<token>(new render_token(input.substr(curr_pos,jj))));
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
                    push_text_token_if_required(list);
                    list.push_back(std::shared_ptr<token>(new section_token(input.substr(curr_pos,jj))));
                    curr_pos += jj + section_token::end_tag().size();
                    prev_pos = curr_pos;
                    --curr_pos;
                    break;
                }
            }
        }
    }
    push_text_token_if_required(list);

    return list;
}

std::string renderer::render_json(const std::string & input, const std::string & context)
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
                    boost::get<std::vector<ez::node>>(context[parent_key]).push_back(pt.data());
                }
                else
                {
                    context[parent_key] = std::vector<ez::node>{pt.data()};
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
    std::string output;
    renderer render;
    token_list toks = render.make_token_list(input);

    std::function<void(token_list & toks, const dict & context, const std::vector<std::string> &, int &, std::string &)> process_for_loop;
    process_for_loop = [&output, &process_for_loop](token_list & toks, const dict & context, const std::vector<std::string> & params, int & index, std::string & output){
        dict for_context = context;
        std::vector<ez::node> & array = boost::get<std::vector<ez::node>>(for_context.at(params[3]));
        int ii_last = index + 1;
        for(const ez::node & _node: array)
        {
            for_context[params[1]] = _node;
            bool loop_done = false;
            int ii;
            for(ii = index + 1; ii < toks.size() && !loop_done; ++ii)
            {
                switch(toks[ii]->token_type())
                {
                case token::type::section:
                    {
                        std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                        std::vector<std::string> params = open_sec->content();
                        if(params[0] == "for")
                        {
                            process_for_loop(toks, for_context, params, ii, output);
                        }
                        else if(params[0] == "endfor")
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

    auto process_tokens = [&context, &process_for_loop](token_list & toks, std::string & output){
        for(int ii = 0; ii < toks.size(); ++ii)
        {
            switch(toks[ii]->token_type())
            {
            case token::type::section:
                {
                    std::shared_ptr<section_token> open_sec = std::dynamic_pointer_cast<section_token>(toks[ii]);
                    std::vector<std::string> params = open_sec->content();
                    if(params[0] == "for")
                    {
                        process_for_loop(toks, context, params, ii, output);
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
    ez::renderer::add_function("date",[](ez::array args) -> std::string{
       namespace pt = boost::posix_time;
       namespace gr = boost::gregorian;
       pt::ptime todayUtc(gr::day_clock::universal_day(), pt::second_clock::universal_time().time_of_day());
       return pt::to_simple_string(todayUtc);
    });

    ez::renderer::add_function("toupper",[](ez::array args) -> std::string{
        std::string str = boost::get<std::string>(args[0]);
        boost::to_upper(str);
        return str;
    });

    return true;
}

static bool function_registered = register_functions();

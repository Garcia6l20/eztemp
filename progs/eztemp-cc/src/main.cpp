#include <eztemp.h>

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace po = boost::program_options;

std::string unescape(const std::string& s)
{
  std::string res;
  std::string::const_iterator it = s.begin();
  while (it != s.end())
  {
    char c = *it++;
    if (c == '\\' && it != s.end())
    {
      switch (*it++) {
      case '\\': c = '\\'; break;
      case 'n': c = '\n'; break;
      case 't': c = '\t'; break;
      default:
        continue;
      }
    }
    res += c;
  }

  return res;
}

int main(int argc, char ** argv)
{
    try
    {
        std::string input;
        std::string params = "{}";
        std::ostream * out = &std::cout;
        std::ofstream fout;
        bool verbose = false;

        po::options_description desc("Options");

        po::positional_options_description p;
        p.add("input", 1);
        p.add("output", 1);


        desc.add_options()
            ("help", "produce help message")
            ("verbose,v", "Let me talk !")
            ("input", po::value(&input), "Input (filename or string)")
            ("output", po::value<std::string>(), "Output <filename>")
            ("params,p", po::value(&params), "Json parameters (filename or string)")
        ;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help") || !vm.count("input")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if(vm.count("verbose"))
        {
            verbose = true;
        }

        if(vm.count("output"))
        {
            fout.open(vm["output"].as<std::string>());
            out = &fout;
        }

        input = unescape(vm["input"].as<std::string>());

        if(vm.count("params"))
        {
            params = unescape(vm["params"].as<std::string>());
            if(boost::ends_with(params, ".json"))
            {
                // load it
                std::ifstream fs(params);
                params = std::string(std::istreambuf_iterator<char>(fs),std::istreambuf_iterator<char>());
            }
        }

        std::chrono::time_point<std::chrono::system_clock> start, end;

        if(verbose)
        {
            start = std::chrono::system_clock::now();
        }

        if(boost::ends_with(input, ".ez"))
        {
            *out << ez::temp::renderer::render_file(input, params);
        }
        else
        {
            *out << ez::temp::renderer::render(input, params);
        }

        if(verbose)
        {
            end = std::chrono::system_clock::now();
            double elapsed_milliseconds = std::chrono::duration_cast<std::chrono::microseconds>
                                     (end-start).count()/1000.;
            std::cout << "Generated in " << elapsed_milliseconds << " milliseconds." << std::endl;
        }

        return 0;
    }
    catch(std::exception & e)
    {
        std::cerr << "Compilation error: " << e.what() << std::endl;
    }

    return -1;
}


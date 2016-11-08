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

static void help(int argc, char ** argv)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << argv[0] << " [options] INPUT_FILENAME_OR_STRING" << std::endl;
    std::cout << "Option:" << std::endl;
    std::cout << "  --help/-h                           Show this message and exit." << std::endl;
    std::cout << "  --verbose/-v                        Let me talk about me." << std::endl;
    std::cout << "  --params/-p  [FILENAME OR STRING]   Json parameters." << std::endl;
}

using _arg = std::pair<std::string, std::string>;

inline bool operator ==(const std::string &str, const _arg & values)
{
    return str == "--" + values.first || str == "-" + values.second;
}

int main(int argc, char ** argv)
{
    try
    {
        std::string input;
        std::string params = "{}";
        std::ostream * out = &std::cout;
        bool verbose = false;

        for (int ii = 1; ii < argc; ++ii)
        {
            std::string arg = argv[ii];
            if (arg == _arg{"help", "h"})
            {
                help(argc, argv);
                exit(0);
            }
            else if (arg == _arg{"verbose", "v"})
            {
                verbose = true;
            }
            else if (arg == _arg{"params", "p"})
            {
                params = unescape(argv[++ii]);
            }
            else if (ii == argc - 1)
            {
                input = unescape(argv[ii]);
            }
            else
            {
                std::cerr << "Unknown argument: " << arg << std::endl;
                help(argc, argv);
                exit(-1);
            }
        }

        if(boost::ends_with(params, ".json"))
        {
            // load it
            std::ifstream fs(params);
            params = std::string(std::istreambuf_iterator<char>(fs),std::istreambuf_iterator<char>());
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

#include <eztemp.h>
#include <ezexpr.h>

#ifdef _WIN32
#include <Windows.h>
#define usleep(__us) Sleep(__us/1000)
#else
#include <unistd.h>
#endif

#include <iostream>

class test_visitor: public boost::static_visitor<std::string>
{
public:
    std::string operator()(int val) const { return std::to_string(val); }
    std::string operator()(std::string val) const { return val; }
};

int main(int argc, char ** argv)
{
    // eval example
    std::cout << ez::expr::eval<double>("10+num", {{"num", double{2}}}) << std::endl;

    // render context example
    std::cout <<
    ez::renderer::render("Hello {{ you }} !!\n"
                         "Today is {{ date() }}\n"
                         "{# for guy in guys #}"
                         "{{ guy.name }} aged {{ guy.age }}.\n"
                         "{# endfor #}"
                         "{# for name in names #}"
                         "{# for say in says #}"
                         "- {{ toupper(name) }} says: {{ say }} !\n"
                         "{# endfor #}"
                         "{# endfor #}",
    {
        {"you", std::string("World")},
        {"guys", ez::array{
            ez::dict{
                {"name", std::string{"riri"}},
                {"age", int{2}}
            },
            ez::dict{
                {"name", std::string{"fifi"}},
                {"age", int{3}}
            },
            ez::dict{
                {"name", std::string{"loulou"}},
                {"age", int{4}}
            }
            }
        },
        {"names", ez::array{
             std::string("toto"),
             std::string("titi"),
             std::string("tata")
         }},
         {"says", ez::array{
              std::string("Hello"),
              std::string("Bye"),
          }},
    })
    << std::endl;

    // reder json example
    std::cout <<
    ez::renderer::render_json("Hello {{ you }} !!\n"
                         "{# for name in names #}"
                         "{# for say in says #}"
                         "- {{ name }} says: {{ say }} !\n"
                         "{# endfor #}"
                         "{# endfor #}",
                         "{ \"you\": \"World\","
                         "  \"names\": [ \"toto\", \"titi\", \"tata\" ],"
                         "  \"says\": [ \"hello\", \"bye\" ]"
                         "}"
    )
    << std::endl;
    return 0;
}

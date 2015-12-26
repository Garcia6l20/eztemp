#include <eztemp.h>
#include <ezexpr.h>

#include <iostream>

int main(int argc, char ** argv)
{
    // eval example
    std::cout << ez::expr::eval<double>("10+num", {{"num", double{2}}}) << std::endl;

    // custom function example
    ez::temp::renderer::add_function("say_my_name", [](const ez::temp::array & unsused){ return "My Name !"; });
    std::cout <<
    ez::temp::renderer::render("{{ say_my_name() }}")
    << std::endl;

    // render context example
    std::cout <<
    ez::temp::renderer::render("Hello {{ you }} !!\n"
                         "Today is {{ date() }}\n"
                         "{% for guy in guys %}"
                         "\n{{ guy.name }} aged {{ guy.age }}."
                         "{% endfor %}"
                         "{% for name in names %}"
                         "{% for say in says %}"
                         "\n- {{ toupper(name) }} says: {{ say }} !\n"
                         "{% endfor %}"
                         "{% endfor %}",
    ez::temp::dict{
        {"you", std::string{"World"}},
        {"guys", ez::temp::array{
            ez::temp::dict{
                {"name", std::string{"riri"}},
                {"age", int{2}}
            },
            ez::temp::dict{
                {"name", std::string{"fifi"}},
                {"age", int{3}}
            },
            ez::temp::dict{
                {"name", std::string{"loulou"}},
                {"age", int{4}}
            }
            }
        },
        {"names", ez::temp::array{
             std::string{"toto"},
             std::string{"titi"},
             std::string{"tata"}
         }},
         {"says", ez::temp::array{
              std::string{"Hello"},
              std::string{"Bye"}
          }},
    })
    << std::endl;

    // render json example
    std::cout <<
    ez::temp::renderer::render("Hello {{ you }} !!\n"
                         "{% for name in names %}"
                         "{% for say in says %}"
                         "\n- {{ name }} says: {{ say }}{% if loop.parent.last %} !!! {% else %} !{% endif %}"
                         "{% endfor %}"
                         "{% endfor %}",
                         "{ \"you\": \"World\","
                         "  \"names\": [ \"toto\", \"titi\", \"tata\" ],"
                         "  \"says\": [ \"hello\", \"bye\" ]"
                         "}"
    )
    << std::endl;
    return 0;
}


/**
 * @file ezexpr.h
 * @author Sylvain Garcia <garcia.sylvain@gmail.com>
 * @date 20/12/2015
 **/
#ifndef __EZ_EXPR_H__
#define __EZ_EXPR_H__

#include <eztemp.h>

#include <cmath>
#include <limits>
#include <iterator>

#include <boost/math/constants/constants.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace ez {

namespace expr {

struct lazy_pow_
{
    template <typename X, typename Y>
    struct result { typedef X type; };

    template <typename X, typename Y>
    X operator()(X x, Y y) const
    {
        return std::pow(x, y);
    }
};

struct lazy_ufunc_
{
    template <typename F, typename A1>
    struct result { typedef A1 type; };

    template <typename F, typename A1>
    A1 operator()(F f, A1 a1) const
    {
        return f(a1);
    }
};

struct lazy_bfunc_
{
    template <typename F, typename A1, typename A2>
    struct result { typedef A1 type; };

    template <typename F, typename A1, typename A2>
    A1 operator()(F f, A1 a1, A2 a2) const
    {
        return f(a1, a2);
    }
};

template <class T>
T max_by_value ( const T a, const T b ) {
    return std::max(a, b);
}

template <class T>
T min_by_value ( const T a, const T b ) {
    return std::min(a, b);
}

template <typename FPT, typename Iterator>
struct grammar
    : boost::spirit::qi::grammar<
            Iterator, FPT(), boost::spirit::ascii::space_type
        >
{

    // symbol table for constants like "pi"
    struct constant_
        : boost::spirit::qi::symbols<
                typename std::iterator_traits<Iterator>::value_type,
                FPT
            >
    {
        constant_()
        {
            this->add
                ("digits",   std::numeric_limits<FPT>::digits    )
                ("digits10", std::numeric_limits<FPT>::digits10  )
                ("e" ,       boost::math::constants::e<FPT>()    )
                ("epsilon",  std::numeric_limits<FPT>::epsilon() )
                ("pi",       boost::math::constants::pi<FPT>()   )
            ;
        }
    } constant;

    struct locals_
        : boost::spirit::qi::symbols<
                typename std::iterator_traits<Iterator>::value_type,
                FPT
            >
    {

    } locals;

    // symbol table for unary functions like "abs"
    struct ufunc_
        : boost::spirit::qi::symbols<
                typename std::iterator_traits<Iterator>::value_type,
                FPT (*)(FPT)
            >
    {
        ufunc_()
        {
            this->add
                ("abs"  , static_cast<FPT (*)(FPT)>(&std::abs  ))
                ("acos" , static_cast<FPT (*)(FPT)>(&std::acos ))
                ("asin" , static_cast<FPT (*)(FPT)>(&std::asin ))
                ("atan" , static_cast<FPT (*)(FPT)>(&std::atan ))
                ("ceil" , static_cast<FPT (*)(FPT)>(&std::ceil ))
                ("cos"  , static_cast<FPT (*)(FPT)>(&std::cos  ))
                ("cosh" , static_cast<FPT (*)(FPT)>(&std::cosh ))
                ("exp"  , static_cast<FPT (*)(FPT)>(&std::exp  ))
                ("floor", static_cast<FPT (*)(FPT)>(&std::floor))
                ("log10", static_cast<FPT (*)(FPT)>(&std::log10))
                ("log"  , static_cast<FPT (*)(FPT)>(&std::log  ))
                ("sin"  , static_cast<FPT (*)(FPT)>(&std::sin  ))
                ("sinh" , static_cast<FPT (*)(FPT)>(&std::sinh ))
                ("sqrt" , static_cast<FPT (*)(FPT)>(&std::sqrt ))
                ("tan"  , static_cast<FPT (*)(FPT)>(&std::tan  ))
                ("tanh" , static_cast<FPT (*)(FPT)>(&std::tanh ))
            ;
        }
    } ufunc;

    // symbol table for binary functions like "pow"
    struct bfunc_
        : boost::spirit::qi::symbols<
                typename std::iterator_traits<Iterator>::value_type,
                FPT (*)(FPT, FPT)
            >
    {
        bfunc_()
        {
            this->add
                ("atan2", static_cast<FPT (*)(FPT, FPT)>(&std::atan2  ))
                ("max"  , static_cast<FPT (*)(FPT, FPT)>(&max_by_value))
                ("min"  , static_cast<FPT (*)(FPT, FPT)>(&min_by_value))
                ("pow"  , static_cast<FPT (*)(FPT, FPT)>(&std::pow    ))
            ;
        }
    } bfunc;

    boost::spirit::qi::rule<
            Iterator, FPT(), boost::spirit::ascii::space_type
        > equation, expression, term, factor, primary;

    grammar() : grammar::base_type(equation)
    {
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::real_policies;
        real_parser<FPT,real_policies<FPT> > real;

        using boost::spirit::qi::_1;
        using boost::spirit::qi::_2;
        using boost::spirit::qi::_3;
        using boost::spirit::qi::no_case;
        using boost::spirit::qi::_val;

        boost::phoenix::function<lazy_pow_>   lazy_pow;
        boost::phoenix::function<lazy_ufunc_> lazy_ufunc;
        boost::phoenix::function<lazy_bfunc_> lazy_bfunc;

        equation = (
                        ( (expression >> "==" >> expression) [ _val = _1 == _2 ] )
                      | ( (expression >> "!=" >> expression) [ _val = _1 != _2 ] )
                      | ( (expression) [ _val = _1 ] )
                    );

        expression =
            term                   [_val =  _1]
            >> *(  ('+' >> term    [_val += _1])
                |  ('-' >> term    [_val -= _1])
                )
            ;

        term =
            factor                 [_val =  _1]
            >> *(  ('*' >> factor  [_val *= _1])
                |  ('/' >> factor  [_val /= _1])
                )
            ;

        factor =
            primary                [_val =  _1]
            >> *(  ("**" >> factor [_val = lazy_pow(_val, _1)])
                )
            ;

        primary =
            real                   [_val =  _1]
            |   '(' >> expression  [_val =  _1] >> ')'
            |   ('-' >> primary    [_val = -_1])
            |   ('+' >> primary    [_val =  _1])
            |   (no_case[ufunc] >> '(' >> expression >> ')')
                                   [_val = lazy_ufunc(_1, _2)]
            |   (no_case[bfunc] >> '(' >> expression >> ','
                                       >> expression >> ')')
                                   [_val = lazy_bfunc(_1, _2, _3)]
            |   no_case[constant]  [_val =  _1]
            |   no_case[locals]    [_val =  _1]
            ;

    }
};

template <typename FPT, typename Iterator>
bool parse(Iterator &iter,
           Iterator end,
           const grammar<FPT,Iterator> &g,
           FPT &result)
{
    return boost::spirit::qi::phrase_parse(
                iter, end, g, boost::spirit::ascii::space, result);
}

class _eval_key_value_visitor: public boost::static_visitor<std::map<std::string, double>>
{
public:
    _eval_key_value_visitor(const std::string & key):
        m_key(key)
    {
    }

    std::map<std::string, double> operator()(nullptr_t value) const
    {
        throw std::runtime_error("ez::eval: nullptr detected");
    }
    std::map<std::string, double> operator()(const std::string &value) const
    {
        double v = -DBL_MAX;
        try
        {
            v = std::stod(value);
        }
        catch(const std::exception & e){}
        return {{m_key, v}};
    }
    std::map<std::string, double> operator()(const double &value) const
    {
        return {{m_key, value}};
    }
    std::map<std::string, double> operator()(const int &value) const
    {
        return {{m_key, (double)value}};
    }
    std::map<std::string, double> operator()(const bool &value) const
    {
        return {{m_key, value == true ? 1.0 : 0.0}};
    }
    std::map<std::string, double> operator ()(const std::map<const std::string, ez::temp::node> & dict) const
    {
        std::map<std::string, double> list;
        for (const std::pair<std::string, ez::temp::node> & ii: dict)
        {
            std::map<std::string, double> child_list = boost::apply_visitor(_eval_key_value_visitor(m_key + "." + ii.first), ii.second);
            list.insert(child_list.begin(), child_list.end());
        }
        return list;
    }
    std::map<std::string, double> operator ()(const ez::temp::array & array) const
    {
        std::map<std::string, double> list;
        int ii = 0;
        for (const ez::temp::node & n: array)
        {
            std::map<std::string, double> child_list = boost::apply_visitor(_eval_key_value_visitor(m_key + "[" + std::to_string(ii++) + "]"), n);
            list.insert(child_list.begin(), child_list.end());
        }
        return list;
    }
private:
    std::string m_key;
    std::map<std::string, double> m_list;
};

static double _eval(const std::string & input, const ez::temp::dict & context)
{
    double result = 0.0;
    grammar<double, std::string::const_iterator> eg;
    std::for_each(context.begin(), context.end(), [&eg](const std::pair<std::string, ez::temp::node> & item){
        try
        {
            std::map<std::string, double> kv = boost::apply_visitor(_eval_key_value_visitor(item.first), item.second);
            for (const std::pair<std::string, double> & ii: kv)
                eg.locals.add(ii.first, ii.second);
        }
        catch(std::exception & e)
        {
            std::cerr << "ez::expr::eval: warning: unhandled data type (" << item.second.type().name() << "): " << e.what() << std::endl;
        }
    });
    std::string::const_iterator ii = input.begin();
    std::string::const_iterator end = input.end();
    parse(ii, end, eg, result);
    return result;
}

template <typename T = double>
static T eval(const std::string & input, const ez::temp::dict & context)
{
    return (T) _eval(input, context);
}

template <>
bool eval<bool>(const std::string & input, const ez::temp::dict & context)
{
    return _eval(input, context) != 0.0;
}

} // namespace expr

} // namespace ez

#endif // __EZ_EXPR_H__

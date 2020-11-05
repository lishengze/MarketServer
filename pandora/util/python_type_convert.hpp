/**
 * Define Type Convert utils here.
 * @Author daniel (daniel.bian@outlook.com)
 * @since  September, 2018
 */
#pragma once
#include "../pandora_declare.h"

#include <vector>
#include <boost/python/object.hpp>
#include <boost/locale.hpp>
#include <boost/python/stl_iterator.hpp>

PANDORA_NAMESPACE_START

/**
 * transform python list into vector with template
 */
template<typename T>
inline vector <T> py_list_to_std_vector(const boost::python::object &iterable)
{
    return vector<T>(boost::python::stl_input_iterator<T>(iterable),
                          boost::python::stl_input_iterator<T>());
}

/**
 * transform vector with template into python list
 */
template<class T>
inline boost::python::list std_vector_to_py_list(const vector <T> &vector)
{
    typename vector<T>::const_iterator iter;
    boost::python::list list;
    for (iter = vector.begin(); iter != vector.end(); ++iter)
    {
        list.append(*iter);
    }
    return list;
}

/**
 * extract int from python dict with key, store in value
 */
inline void getInt(const boost::python::dict &d, const string &key, int *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<int> x(o);
        if (x.check())
        {
            *value = x();
        }
    }
}

/**
 * extract long from python dict with key, store in value
 */
inline void getLong(const boost::python::dict &d, const string &key, long *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<int> x(o);
        if (x.check())
        {
            *value = x();
        }
    }
}

/**
 * extract short from python dict with key, store in value
 */
inline void getShort(const boost::python::dict &d, const string &key, short *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<int> x(o);
        if (x.check())
        {
            *value = x();
        }
    }
}

/**
 * extract double from python dict with key, store in value
 */
inline void getDouble(const boost::python::dict &d, const string &key, double *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<double> x(o);
        if (x.check())
        {
            *value = x();
        }
    }
}

/**
 * extract char from python dict with key, store in value
 */
inline void getChar(const boost::python::dict &d, const string &key, char *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<string> x(o);
        if (x.check())
        {
            string s = x();
            const char *buffer = s.c_str();
            *value = *buffer;
        }
    }
}

/**
 * extract string(char*) from python dict with key, store in value
 */
inline void getString(const boost::python::dict &d, const string &key, char *value)
{
    if (d.has_key(key))
    {
        boost::python::object o = d[key];
        boost::python::extract<string> x(o);
        if (x.check())
        {
            string s = x();
            const char *buffer = s.c_str();
            strcpy(value, buffer);
        }
    }
}

/**
 * convert gbk to utf8
 */
inline string gbk2utf8(const string& str)
{
    return boost::locale::conv::between(str, "UTF-8", "GBK");
}

PANDORA_NAMESPACE_END

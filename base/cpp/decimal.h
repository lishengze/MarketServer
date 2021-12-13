#pragma once

#include "basic.h"

const double G_PRECISION = 0.000000001;

#define CALC_BASE(x) (uint64(pow(10, (x))))

template<typename T>
inline T labs( const T & x ){return x<0?-x:x;}
 
template<typename T>
inline int64 sgn( const T & x ){return x<0?-1:(x?1:0);}

inline int64 d_round( const double & x ){return (int64)(sgn(x)*(labs(x)+0.50001));}

inline int64 str_to_int64(const char* s) {
#ifdef WIN32
    return _atoi64(s);
#else
    char *endptr = NULL;
    return strtoll(s, &endptr, 10);
#endif
}

#pragma pack(1)
struct SDecimal {
    union{
        struct{
            mutable uint64 tag_:1;
            mutable uint64 value_:55;
            mutable uint64 prec_:8;
        }real_;
        mutable uint64 value_;
    }data_;

    SDecimal() {
        data_.value_ = 0;
    }

    SDecimal(const SDecimal&& src):data_{std::move(src.data_)}
    {

    }

    SDecimal(const SDecimal& src):data_{src.data_}
    {

    }    

    SDecimal& operator= (const SDecimal& src)
    {
        data_ = src.data_;
        return *this;
    }   

    SDecimal(const string& v) {
        from(v);
    }

    SDecimal(const double v, int precise = -1, bool ceiling = false) {
        from(v, precise, ceiling);
    }

    explicit SDecimal(const string& s, int precise = -1, bool ceiling = false) {
        from(s, precise, ceiling);
    }

    static SDecimal parse(const string& s, int precise = -1, bool ceiling = false) {
        SDecimal ret;
        ret.from(s, precise, ceiling);
        return ret;
    }

    static SDecimal parse(double v, int precise = -1, bool ceiling = false) {
        SDecimal ret;
        ret.from(v, precise, ceiling);
        return ret;
    }
    
    static SDecimal parse_by_raw(uint64 base, uint64 prec) {
        SDecimal ret;
        ret.from_raw(base, prec);
        return ret;
    }

    uint64 get_raw() const {
        return data_.value_;
    }

    static SDecimal max_decimal() {
        SDecimal ret;
        ret.data_.value_ = ULLONG_MAX;
        return ret;
    }

    static SDecimal min_decimal() {
        SDecimal ret;
        return ret;
    }

    bool is_zero() const {
        return data_.real_.value_ == 0;
    }
    
    void from_raw(uint64 base, uint64 prec) {
        data_.real_.value_ = base;
        data_.real_.prec_ = prec;
    }

    void from(double v, int precise = -1, bool ceiling = false) {
        if( precise == -1 ) {
            char value[1024];
            sprintf(value, "%f", v);
            return from(value, precise, ceiling);
        }
        data_.real_.prec_ = precise;
        data_.real_.value_ = d_round(v * CALC_BASE(precise));
        if( ceiling && get_value() < v )
            data_.real_.value_ += 1;
    }

    void scale(int precise, bool ceiling = false) 
    {
        if( precise < 0 || data_.real_.prec_ == precise )
            return;

        if( precise > data_.real_.prec_ ) { // 放大
            uint64 v = CALC_BASE(precise - data_.real_.prec_);
            data_.real_.value_ *= v;
            data_.real_.prec_ = precise;
        } else { // 缩小
            uint64 v = CALC_BASE(data_.real_.prec_ - precise);
            uint64 remain = data_.real_.value_ % v;
            data_.real_.prec_ = precise;
            data_.real_.value_ /= v;
            if( ceiling && remain > 0 )
                data_.real_.value_ += 1;
        }
    }

    const SDecimal multiple(double factor, bool ceiling = false) const
    {
        SDecimal ret = *this;
        double tmp = (double)ret.data_.real_.value_ * factor;
        ret.data_.real_.value_ = tmp;
        if( ceiling && tmp > ret.data_.real_.value_) {
            ret.data_.real_.value_ += 1;
        }
        return ret;
    }

    const SDecimal add(double factor, bool ceiling = false) const
    {
        SDecimal ret = *this;
        double tmp = (double)ret.data_.real_.value_ + factor * CALC_BASE(ret.data_.real_.prec_);
        ret.data_.real_.value_ = tmp;
        if( ceiling && tmp > ret.data_.real_.value_ ) {
            ret.data_.real_.value_ += 1;
        }
        return ret;
    }

    void from(const string& data, int precise = -1, bool ceiling = false) {
        std::string::size_type pos = data.find(".");
        if( pos == string::npos ) { // 没有小数
            data_.real_.prec_ = 0;
            data_.real_.value_ = str_to_int64(data.c_str());
        } else { // 有小数
            int point = data.length() - pos - 1;
            data_.real_.prec_ = point;
            string newData = data.substr(0, pos) + data.substr(pos+1, data.length()- pos - 1);
            data_.real_.value_ = str_to_int64(newData.c_str());
        }

        // 缩放
        if( precise != -1 ) {
            scale(precise, ceiling);
        }
    }

    void from(const SDecimal& data, int precise = -1, bool ceiling = false) {
        if( precise == -1 || data.data_.real_.prec_ <= precise ) {
            data_.real_.value_ = data.data_.real_.value_;
            data_.real_.prec_ = data.data_.real_.prec_;
            return;
        }

        from(data.get_str_value(), precise, ceiling);
    }

    double get_value() const {
        return data_.real_.value_ * 1.0 / CALC_BASE(data_.real_.prec_);
    }

    string get_str_value() const {
        if( data_.real_.value_ == 0 )
            return "0";

        char precise[25];
        sprintf(precise, "%d", data_.real_.prec_+1);

        char holder[1024];
        string fmt = "%0" + string(precise) + "lu";
        sprintf(holder, fmt.c_str(), data_.real_.value_);
        string ret = holder;
        ret.insert(ret.begin() + ret.length() - data_.real_.prec_, '.');
        return ret;
    }

    bool operator <(const SDecimal& d) const {
        if( data_.real_.prec_ > d.data_.real_.prec_ ) {
            return data_.real_.value_ < d.data_.real_.value_ * CALC_BASE(data_.real_.prec_-d.data_.real_.prec_);
        } else {
            return data_.real_.value_ * CALC_BASE(d.data_.real_.prec_-data_.real_.prec_) < d.data_.real_.value_;
        }
    }
    bool operator >(const SDecimal& d) const {
        if( data_.real_.prec_ > d.data_.real_.prec_ ) {
            return data_.real_.value_ > d.data_.real_.value_ * CALC_BASE(data_.real_.prec_-d.data_.real_.prec_);
        } else {
            return data_.real_.value_ * CALC_BASE(d.data_.real_.prec_-data_.real_.prec_) > d.data_.real_.value_;
        }
    }
    bool operator ==(const SDecimal& d) const {
        if( data_.real_.prec_ > d.data_.real_.prec_ ) {
            return data_.real_.value_ == d.data_.real_.value_ * CALC_BASE(data_.real_.prec_-d.data_.real_.prec_);
        } else {
            return data_.real_.value_ * CALC_BASE(d.data_.real_.prec_-data_.real_.prec_) == d.data_.real_.value_;
        }
    }
    bool operator <=(const SDecimal& d) const {
        if( data_.real_.prec_ > d.data_.real_.prec_ ) {
            return data_.real_.value_ <= d.data_.real_.value_ * CALC_BASE(data_.real_.prec_-d.data_.real_.prec_);
        } else {
            return data_.real_.value_ * CALC_BASE(d.data_.real_.prec_-data_.real_.prec_) <= d.data_.real_.value_;
        }
    }

    bool operator < (const double& d) const {
        return get_value() < d;
    }

    bool operator <=(const double& d) const {
        double d_value = get_value();
        return d_value < d || fabs(d_value-d) < G_PRECISION; 
    }

    bool operator >=(const SDecimal& d) const {
        if( data_.real_.prec_ > d.data_.real_.prec_ ) {
            return data_.real_.value_ >= d.data_.real_.value_ * CALC_BASE(data_.real_.prec_-d.data_.real_.prec_);
        } else {
            return data_.real_.value_ * CALC_BASE(d.data_.real_.prec_-data_.real_.prec_) >= d.data_.real_.value_;
        }
    }

    const SDecimal operator + (const SDecimal &rhs) const {
        SDecimal ret = *this;
        if( ret.data_.real_.prec_ > rhs.data_.real_.prec_ ) {
            ret.data_.real_.value_ = ret.data_.real_.value_ + rhs.data_.real_.value_ * CALC_BASE(ret.data_.real_.prec_ - rhs.data_.real_.prec_);
        } else {
            ret.data_.real_.value_ = ret.data_.real_.value_ * CALC_BASE(rhs.data_.real_.prec_ - ret.data_.real_.prec_) + rhs.data_.real_.value_;
            ret.data_.real_.prec_ = rhs.data_.real_.prec_;
        }
        return ret;
    }

    SDecimal & operator+=(const SDecimal &rhs) {
        if( data_.real_.prec_ > rhs.data_.real_.prec_ ) {
            data_.real_.value_ = data_.real_.value_ + rhs.data_.real_.value_ * CALC_BASE(data_.real_.prec_ - rhs.data_.real_.prec_);
        } else {
            data_.real_.value_ = data_.real_.value_ * CALC_BASE(rhs.data_.real_.prec_ - data_.real_.prec_) + rhs.data_.real_.value_;
            data_.real_.prec_ = rhs.data_.real_.prec_;
        }
        return *this;
    }

    const SDecimal operator-(const SDecimal &rhs) const {
        SDecimal ret = *this;
        if( ret.data_.real_.prec_ > rhs.data_.real_.prec_ ) {
            ret.data_.real_.value_ = ret.data_.real_.value_ - rhs.data_.real_.value_ * CALC_BASE(ret.data_.real_.prec_ - rhs.data_.real_.prec_);
        } else {
            ret.data_.real_.value_ = ret.data_.real_.value_ * CALC_BASE(rhs.data_.real_.prec_ - ret.data_.real_.prec_) - rhs.data_.real_.value_;
            ret.data_.real_.prec_ = rhs.data_.real_.prec_;
        }
        return ret;
    }

    SDecimal & operator-=(const SDecimal &rhs) {
        if( data_.real_.prec_ > rhs.data_.real_.prec_ ) {
            data_.real_.value_ = data_.real_.value_ - rhs.data_.real_.value_ * CALC_BASE(data_.real_.prec_-rhs.data_.real_.prec_);
        } else {
            data_.real_.value_ = data_.real_.value_ * CALC_BASE(rhs.data_.real_.prec_-data_.real_.prec_) - rhs.data_.real_.value_;
            data_.real_.prec_ = rhs.data_.real_.prec_;
        }
        return *this;
    }

    const SDecimal operator / (const double &d) const {
        SDecimal ret = *this;
        ret.data_.real_.value_ /= d;
        return ret;
    }

    SDecimal & operator/=(const double &rhs) {
        data_.real_.value_ /= rhs;
        return *this;
    }

    const SDecimal operator * (const double &d) const {
        SDecimal ret = *this;
        ret.data_.real_.value_ *= d;
        return ret;
    }

    SDecimal & operator*=(const double &rhs) {
        data_.real_.value_ *= rhs;
        return *this;
    }
};
#pragma pack()

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <boost/shared_ptr.hpp>

// fields that store in the package define base structure here
struct FieldDef {
    FieldDef(boost::shared_ptr<char> field, int len, long fid) {
        Field = field;
        FieldLength = len;
        FieldID = fid;
    }

    FieldDef(int len, long fid) {
        Field = boost::shared_ptr<char>{ new char[len],
                                         std::default_delete<char[]>{}};
        memset(Field.get(), 0, len);
        FieldLength = len;
        FieldID = fid;
    }

    bool operator == (const FieldDef& rhs) const
    {
        return rhs.FieldID == this->FieldID;
    }

    ~FieldDef() {
        FieldLength = 0;
        FieldID = 0;
    }

    // field attribute
    // field
    boost::shared_ptr<char> Field;
    // field length
    int FieldLength;
    // field id
    long FieldID;
};

// read and write channel for read or write data
class IChannel {
public:
    virtual void send(int data, bool is_last = true) {
        send(std::to_string(data), is_last);
    }
    virtual void send(const long& data, bool is_last = true) {
        send(std::to_string(data), is_last);
    }
    virtual void send(const unsigned int& data, bool is_last = true) {
        send(std::to_string(data), is_last);
    }
    virtual void send(const std::string&, bool is_last = true) = 0;
    // virtual void send(void* data, int& len, long& fid, bool is_last=true)=0;
    virtual void send(std::vector<FieldDef>& fields) = 0;

    // virtual void read(char*& data, int& len, long& fid)=0;
    virtual void read(std::vector<FieldDef>& fields) = 0;
    virtual void read(std::string&) = 0;
    virtual void read(unsigned int&) = 0;
    virtual void read(unsigned char&) = 0;
    virtual void read(long&) = 0;
    virtual void read(long long&) = 0;
    // virtual void read(char* data) = 0;
    // return value: how much data has been sent, -1 for error
    virtual int send(const char* data, size_t size, bool is_last = true) = 0;
};

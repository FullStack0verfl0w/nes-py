#pragma once
#include <cstdint>
#include <stack>
#include <ostream>
#include <istream>
#include <iostream>
#include "log.hpp"

/* ──────────────────────────────────────────
 * Minimal RIFF‑style helper: <id><len><data>
 * id  = 4‑byte ASCII
 * len = little‑endian uint32 length of data
 * ────────────────────────────────────────── */
namespace NES {

class StateWriter {
    std::ostream& out_;
    std::stack<std::streampos> len_pos_;
    void put_u32(uint32_t v) { out_.write(reinterpret_cast<char*>(&v), 4); }
public:
    explicit StateWriter(std::ostream& o) : out_(o) {}

    void begin(std::string id) {
        out_.write(id.data(), 4);               // <id>
        len_pos_.push(out_.tellp());
        put_u32(0);                       // reserve space for <len>
    }
    template<class T> void write(const T& v) {
        auto size = sizeof(T);
        LOG(Info) << "Writing " << size << " bytes" << std::endl;
        out_.write(reinterpret_cast<const char*>(&v), size);
    }
    void write_block(const void* p, size_t n) {
        LOG(Info) << "Writing block of " << n << " bytes" << std::endl;
        out_.write(reinterpret_cast<const char*>(p), n);
    }

    void end() {
        const std::streampos here = out_.tellp();
        const std::streampos lenp = len_pos_.top(); len_pos_.pop();
        const uint32_t len = static_cast<uint32_t>(here - lenp - 4);
        out_.seekp(lenp);
        put_u32(len);                     // patch <len>
        out_.seekp(here);
    }
};

class StateReader {
    std::istream& in_;
    std::stack<uint32_t> remaining_;
    uint32_t get_u32() { uint32_t v; in_.read(reinterpret_cast<char*>(&v), 4); return v; }
public:
    explicit StateReader(std::istream& i) : in_(i) {}

    bool next(std::string& id, uint32_t& len)
    {
        char cid[4];
        if (!in_.read(cid, 4))
            return false;

        id.assign(cid, 4);          // <id>
        len = get_u32();            // <len>
        remaining_.push(len);
        return true;
    }
    template<class T> void read(T& v) {
        auto size = sizeof(T);
        LOG(Info) << "Reading " << size << " bytes" << std::endl;
        in_.read(reinterpret_cast<char*>(&v), size); remaining_.top() -= size;
    }
    void read_block(void* p, size_t n) {
        LOG(Info) << "Reading block of " << n << " bytes" << std::endl;
        in_.read(reinterpret_cast<char*>(p), n); remaining_.top() -= n;
    }

    void skip_remainder() { in_.seekg(remaining_.top(), std::ios::cur); remaining_.pop(); }
};

}

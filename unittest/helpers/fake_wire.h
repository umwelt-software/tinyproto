/*
    Copyright 2017 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _FAKE_WIRE_H_
#define _FAKE_WIRE_H_

#include <stdint.h>
#include <mutex>
#include <list>
#include <thread>
#include <atomic>

class FakeWire
{
public:
    FakeWire(int readbuf_size  = 1024, int writebuf_size = 1024);
    ~FakeWire();
    int read(uint8_t * data, int length, int timeout = 1000);
    int write(const uint8_t * data, int length, int timeout = 1000);
    void generate_error_every_n_byte(int n) { m_errors.push_back( ErrorDesc{0, n, -1} ); };
    void generate_single_error(int position) { m_errors.push_back( ErrorDesc{position, position, 1} ); };
    void generate_error(int first, int period, int count = -1) { m_errors.push_back( ErrorDesc{first, period, count} ); };
    void reset();
    void disable() { m_enabled = false; }
    void enable()  { m_enabled = true; }
    void flush();
    int lostBytes() { return m_lostBytes; }
private:
    typedef struct
    {
        int first;
        int period;
        int count;
    } ErrorDesc;
    int          m_writeptr = 0;
    int          m_readptr = 0;
    std::mutex   m_readmutex{};
    std::mutex   m_writemutex{};
    uint8_t      *m_readbuf = nullptr;
    uint8_t      *m_writebuf = nullptr;
    int          m_readbuf_size = 0;
    int          m_writebuf_size = 0;
    int          m_readlen = 0;
    int          m_writelen = 0;
    std::list<ErrorDesc> m_errors;
    int          m_byte_counter = 0;
    bool         m_enabled = true;
    std::atomic<int> m_lostBytes{ 0 };

    void         TransferData(int num_bytes);

    friend class FakeConnection;
};


#endif /* _FAKE_WIRE_H_ */

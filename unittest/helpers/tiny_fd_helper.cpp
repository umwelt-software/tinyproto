/*
    Copyright 2017-2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include "tiny_fd_helper.h"
#include <unistd.h>

TinyHelperFd::TinyHelperFd(FakeEndpoint *endpoint, int rxBufferSize,
                           const std::function<void(uint8_t *, int)> &onRxFrameCb, int window_frames, int timeout)
    : IBaseHelper(endpoint, rxBufferSize)
    , m_onRxFrameCb(onRxFrameCb)
    , m_rxBufferSize(rxBufferSize)
    , m_window(window_frames)
    , m_timeout(timeout)
{
    init();
}

TinyHelperFd::TinyHelperFd(FakeEndpoint *endpoint, int rxBufferSize, uint8_t mode,
                           const std::function<void(uint8_t *, int)> &onRxFrameCb)
    : IBaseHelper(endpoint, rxBufferSize)
    , m_onRxFrameCb(onRxFrameCb)
    , m_rxBufferSize(rxBufferSize)
    , m_window(7)
    , m_timeout(-1)
{
    m_mode = mode;
}

void TinyHelperFd::setTimeout(int timeout)
{
    m_timeout = timeout;
}

void TinyHelperFd::setAddress(uint8_t address)
{
    m_addr = address;
}

void TinyHelperFd::setPeersCount(uint8_t count)
{
    m_peersCount = count;
}

int TinyHelperFd::init()
{
    tiny_fd_init_t init{};
    init.pdata = this;
    init.on_frame_cb = onRxFrame;
    init.on_sent_cb = onTxFrame;
    init.on_connect_event_cb = onConnect;
    init.buffer = m_buffer;
    init.buffer_size = m_rxBufferSize;
    init.window_frames = m_window ? m_window : 7;
    init.send_timeout = m_timeout < 0 ? 2000 : m_timeout;
    init.retry_timeout = init.send_timeout ? (init.send_timeout / 2) : 200;
    init.retries = 2;
    init.mode = m_mode;
    init.peers_count = m_peersCount;
    init.addr = m_addr;
    init.crc_type = HDLC_CRC_16;

    return tiny_fd_init(&m_handle, &init);
}

void TinyHelperFd::set_connect_cb(const std::function<void(uint8_t, bool)> &onConnectCb)
{
    m_onConnectCb = onConnectCb;
}

int TinyHelperFd::registerPeer(uint8_t address)
{
    return tiny_fd_register_peer(m_handle, address);
}

int TinyHelperFd::send(uint8_t *buf, int len)
{
    return tiny_fd_send_packet(m_handle, buf, len);
}

int TinyHelperFd::sendto(uint8_t address, uint8_t *buf, int len)
{
    return tiny_fd_send_packet_to(m_handle, address, buf, len);
}

void TinyHelperFd::MessageSender(TinyHelperFd *helper, int count, std::string msg)
{
    while ( count-- && !helper->m_stop_sender )
    {
        helper->send((uint8_t *)msg.c_str(), msg.size());
    }
}

int TinyHelperFd::send(const std::string &message)
{
    return send((uint8_t *)message.c_str(), message.size());
}

int TinyHelperFd::send(int count, const std::string &msg)
{
    if ( m_message_sender )
    {
        m_stop_sender = true;
        m_message_sender->join();
        delete m_message_sender;
        m_message_sender = nullptr;
    }
    if ( count != 0 )
    {
        m_stop_sender = false;
        m_message_sender = new std::thread(MessageSender, this, count, msg);
    }
    return 0;
}

int TinyHelperFd::run_tx()
{
    uint8_t buf[16];
    int len = tiny_fd_get_tx_data(m_handle, buf, sizeof(buf));
    uint8_t *ptr = buf;
    while ( len > 0 )
    {
        int i = write_data(this, ptr, len);
        if ( i > 0 )
        {
            len -= i;
            ptr += i;
        }
    }
    return 0;
}

int TinyHelperFd::run_rx()
{
    uint8_t buffer[16];
    int len = read_data(this, buffer, sizeof(buffer));
    return tiny_fd_on_rx_data(m_handle, buffer, len);
}

void TinyHelperFd::wait_until_rx_count(int count, uint32_t timeout)
{
    while ( m_rx_count != count && timeout-- )
        usleep(1000);
}

void TinyHelperFd::onRxFrame(void *handle, uint8_t *buf, int len)
{
    TinyHelperFd *helper = reinterpret_cast<TinyHelperFd *>(handle);
    helper->m_rx_count++;
    if ( helper->m_onRxFrameCb )
    {
        helper->m_onRxFrameCb(buf, len);
    }
}

void TinyHelperFd::onTxFrame(void *handle, uint8_t *buf, int len)
{
    TinyHelperFd *helper = reinterpret_cast<TinyHelperFd *>(handle);
    helper->m_tx_count++;
}

void TinyHelperFd::onConnect(void *handle, uint8_t addr, bool connected)
{
    TinyHelperFd *helper = reinterpret_cast<TinyHelperFd *>(handle);
    if ( helper->m_onConnectCb )
    {
        helper->m_onConnectCb(addr, connected);
    }
}

TinyHelperFd::~TinyHelperFd()
{
    // stop sender thread
    send(0, "");
    stop();
    tiny_fd_close(m_handle);
}

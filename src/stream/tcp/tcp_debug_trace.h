//--------------------------------------------------------------------------
// Copyright (C) 2014-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------

// tcp_debug_trace.h author davis mcpherson <davmcphe@cisco.com>
// Created on: Aug 5, 2015

#ifndef TCP_DEBUG_TRACE_H
#define TCP_DEBUG_TRACE_H

#include "utils/stats.h"

#include "tcp_reassemblers.h"

#ifndef REG_TEST
#define S5TraceTCP(tsd)
#else
#define LCL(p, x) ((p).x() - (p).get_iss())
#define RMT(p, x, q) ((p).x - (q).get_iss())

static const char* const statext[] =
{
    "LST", "SYS", "SYR", "EST", "FW1", "FW2", "CLW",
    "CLG", "LAK", "TWT", "CLD", "NON"
};

static const char* const flushxt[] = { "IGN", "FPR", "PRE", "PRO", "PAF" };

static THREAD_LOCAL int s5_trace_enabled = -1;  // FIXIT-L should use module trace feature

inline void TraceEvent(const TcpSegmentDescriptor& tsd, uint32_t txd, uint32_t rxd)
{
    char flags[7] = "UAPRSF";
    const snort::tcp::TCPHdr* h = tsd.get_tcph();
    const char* order = "";
    const char* meta_ack_marker = tsd.is_meta_ack_packet() ? "M" : " ";

    if (!h)
        return;

    for (int i = 0; i < 6; i++)
        if (!((1 << (5 - i)) & h->th_flags))
            flags[i] = '-';

    // force relative ack to zero if not conveyed
    if (flags[1] != 'A')
        rxd = tsd.get_ack();   // FIXIT-L SYN's seen with ack > 0 and ACK flag not set...

    if ( tsd.are_packet_flags_set(PKT_STREAM_ORDER_OK) )
        order = " (ins)";
    else if ( tsd.are_packet_flags_set(PKT_STREAM_ORDER_BAD) )
        order = " (oos)";

    uint32_t rseq = ( txd ) ? tsd.get_seq() - txd : tsd.get_seq();
    uint32_t rack = ( rxd ) ? tsd.get_ack() - rxd : tsd.get_ack();
    fprintf(stdout, "\n" FMTu64("-3") " %s %s=0x%02x Seq=%-4u Ack=%-4u Win=%-4u Len=%-4hu%s\n",
        tsd.get_packet_number(), meta_ack_marker, flags, h->th_flags, rseq, rack, tsd.get_wnd(),
        tsd.get_len(), order);
}

inline void TraceSession(const snort::Flow* flow)
{
    fprintf(stdout, "      LWS: ST=0x%x SF=0x%x CP=%hu SP=%hu\n", (unsigned)flow->session_state,
        flow->ssn_state.session_flags, flow->client_port, flow->server_port);
}

inline void TraceState(TcpStreamTracker& a, TcpStreamTracker& b, const char* s)
{
    uint32_t ua = a.get_snd_una() ? LCL(a, get_snd_una) : 0;
    uint32_t ns = a.get_snd_nxt() ? LCL(a, get_snd_nxt) : 0;

    fprintf(stdout,
        "      %s ST=%s      UA=%-4u NS=%-4u LW=%-5u RN=%-4u RW=%-4u ISS=%-4u IRS=%-4u ",
        s, statext[a.get_tcp_state()], ua, ns, a.get_snd_wnd( ),
        RMT(a, rcv_nxt, b), RMT(a, r_win_base, b), a.get_iss(), a.get_irs());

    fprintf(stdout, "\n");
    unsigned paf = ( a.is_splitter_paf() ) ? 2 : 0;
    unsigned fpt = a.get_flush_policy() ? 192 : 0;

    fprintf(stdout, "           FP=%s:%-4u SC=%-4u FL=%-4u SL=%-5u BS=%-4u",
        flushxt[a.get_flush_policy() + paf], fpt,
        a.reassembler.get_seg_count(), a.reassembler.get_flush_count(),
        a.reassembler.get_seg_bytes_logical(),
        a.reassembler.get_seglist_base_seq() - b.get_iss());

    if ( s5_trace_enabled == 2 )
        a.reassembler.trace_segments();

    fprintf(stdout, "\n");
}

inline void TraceTCP(const TcpSegmentDescriptor& tsd)
{
    TcpSession* ssn = (TcpSession*)tsd.get_flow()->session;
    assert(ssn);
    TcpStreamTracker& srv = ssn->server;
    TcpStreamTracker& cli = ssn->client;

    const char* cdir = "?", * sdir = "?";
    uint32_t txd = 0, rxd = 0;

    if ( tsd.is_packet_from_client() )
    {
        sdir = "SRV<";
        cdir = "CLI>";
        txd = cli.get_iss();
        rxd = cli.get_irs();
    }
    else
    {
        sdir = "SRV>";
        cdir = "CLI<";
        txd = srv.get_iss();
        rxd = srv.get_irs();
    }

    TraceEvent(tsd, txd, rxd);

    if ( ssn->lws_init )
        TraceSession(tsd.get_flow());

    TraceState(cli, srv, cdir);
    TraceState(srv, cli, sdir);
}

inline void S5TraceTCP(const TcpSegmentDescriptor& tsd)
{
    if ( !s5_trace_enabled )
        return;

    if ( s5_trace_enabled < 0 )
    {
        const char* s5t = getenv("S5_TRACE");

        if ( !s5t )
        {
            s5_trace_enabled = 0;
            return;
        }

        // no error checking required - atoi() is sufficient
        s5_trace_enabled = atoi(s5t);
    }

    TraceTCP(tsd);
}
#endif  // REG_TEST

#endif


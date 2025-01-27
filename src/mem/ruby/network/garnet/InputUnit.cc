/*
 * Copyright (c) 2020 Inria
 * Copyright (c) 2016 Georgia Institute of Technology
 * Copyright (c) 2008 Princeton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet/InputUnit.hh"

#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet/Credit.hh"
#include "mem/ruby/network/garnet/Router.hh"

namespace gem5
{

namespace ruby
{

namespace garnet
{

InputUnit::InputUnit(int id, PortDirection direction, Router *router)
  : Consumer(router), m_router(router), m_id(id), m_direction(direction),
    m_vc_per_vnet(m_router->get_vc_per_vnet())
{
    const int m_num_vcs = m_router->get_num_vcs();
    m_num_buffer_reads.resize(m_num_vcs/m_vc_per_vnet);
    m_num_buffer_writes.resize(m_num_vcs/m_vc_per_vnet);
    for (int i = 0; i < m_num_buffer_reads.size(); i++) {
        m_num_buffer_reads[i] = 0;
        m_num_buffer_writes[i] = 0;
    }

    // Instantiating the virtual channels
    virtualChannels.reserve(m_num_vcs);
    for (int i=0; i < m_num_vcs; i++) {
        virtualChannels.emplace_back();
    }
}

/*
 * The InputUnit wakeup function reads the input flit from its input link.
 * Each flit arrives with an input VC.
 * For HEAD/HEAD_TAIL flits, performs route computation,
 * and updates route in the input VC.
 * The flit is buffered for (m_latency - 1) cycles in the input VC
 * and marked as valid for SwitchAllocation starting that cycle.
 *
 */

void InputUnit::handleLocalReply(flit* t_flit){
    // std::cout << "Handling Local Reply, will try to get the flit out of the VC !" << std::endl;
    // How to check if the flit has the data we are looking for ?
    // Lets say the flit has a msg_ptr and the msg_ptr type is RequestMsg
    // It has a DataBlock
    // Now we need to check if the address that we had is present in this Datablock ?
    Addr req = t_flit->get_msg_ptr()->get_physical_address();

    for(auto vc : m_vc_as_cache){
        if(vc >= virtualChannels.size()) continue;

        flit* vc_flit = virtualChannels[vc].getTopFlit();
        if(vc_flit == nullptr){
            continue;
        }
        MsgPtr& vc_msg = vc_flit->get_msg_ptr();
        std::cout << "Finding in VC ( " << vc << " )" << std::endl;
        if(req >= vc_msg->get_physical_address() && req <= vc_msg->get_physical_address() + (Addr)128){
            std::cout << "Found the required address [" << req << "] inside the VC(" << vc << ") having address [" << vc_msg->get_physical_address() << "] and block size - " << 128 << std::endl;
            return;
        }
    }
}

void
InputUnit::wakeup()
{
    flit *t_flit;
    // Consume the incoming link
    // Now you have the flit for the peket, and you need to store this flit into the VC
    // and just disable the SA and VA for this particular flit so that it keeps there in the 
    // same router till the SA and VA are started again.
    if(m_vc_per_vnet == m_vc_as_cache.size()){
        // what is the logic? can i do all this in one curTick() ?
        // this will only activate when some message is recieved, need to do something about it !!
        std::cout << "All VC's of this router are full, emptying them" << std::endl;
        for(auto vc : m_vc_as_cache){
            assert(virtualChannels[vc].get_state() == BUSY_STORE_);
            set_vc_active(vc, curTick());
            assert(virtualChannels[vc].get_state() == ACTIVE_);

            if(virtualChannels[vc].isReady(curTick())){
                t_flit = virtualChannels[vc].getTopFlit();

                assert(t_flit != nullptr);
                assert(t_flit->m_isStore == true);

                Cycles pipe_stages = m_router->get_pipe_stages();
                if (pipe_stages == 1) {
                    // 1-cycle router
                    // Flit goes for SA directly
                    t_flit->advance_stage(SA_, curTick());
                } else {
                    assert(pipe_stages > 1);
                    // Router delay is modeled by making flit wait in buffer for
                    // (pipe_stages cycles - 1) cycles before going for SA

                    Cycles wait_time = pipe_stages - Cycles(1);
                    t_flit->advance_stage(SA_, m_router->clockEdge(wait_time));

                    // Wakeup the router in that cycle to perform SA
                    m_router->schedule_wakeup(Cycles(wait_time));
                }
                t_flit->m_isStore = false;
                std::cout << "Packet [ " << t_flit->getPacketID() << " ] removed from ( " << vc << " ) [ Router : " << m_router->get_id() << " ] and leaving for it's desired location !" << std::endl;
            }
        }
        m_vc_as_cache.clear();
        if (m_in_link->isReady(curTick())) {
            m_router->schedule_wakeup(Cycles(1));
        }
        return;
    }

    if (m_in_link->isReady(curTick())) {
        t_flit = m_in_link->consumeLink();
        
        // if(t_flit->m_isReadReq || t_flit->m_isWriteReq){
        //     handleLocalReply(t_flit);
        // }

        DPRINTF(RubyNetwork, "Router[%d] Consuming:%s Width: %d Flit:%s\n",
        m_router->get_id(), m_in_link->name(),
        m_router->getBitWidth(), *t_flit);

        assert(t_flit->m_width == m_router->getBitWidth());
        int vc = t_flit->get_vc();
        t_flit->increment_hops(); // for stats

        if ((t_flit->get_type() == HEAD_) ||
            (t_flit->get_type() == HEAD_TAIL_)) {

            assert(virtualChannels[vc].get_state() == IDLE_);
            set_vc_active(vc, curTick());

            // Here the srfd unit will be active and check for the store flag
            // and if the store flag is true, it wont get any outport


            // Route computation for this vc
            int outport = m_router->route_compute(t_flit->get_route(),
                m_id, m_direction);

            // Update output port in VC
            // All flits in this packet will use this output port
            // The output port field in the flit is updated after it wins SA
            grant_outport(vc, outport);

        } else {
            assert(virtualChannels[vc].get_state() == ACTIVE_);
        }


        // Buffer the flit
        virtualChannels[vc].insertFlit(t_flit);

        int vnet = vc/m_vc_per_vnet;
        // number of writes same as reads
        // any flit that is written will be read only once
        m_num_buffer_writes[vnet]++;
        m_num_buffer_reads[vnet]++;

        Cycles pipe_stages = m_router->get_pipe_stages();
        // If the srfd unit says, then dont advance it for the further steps right ?

        // And also this has to be seen only when it is a data packet and nothing else
        // how do i check if this is a data packet.
        if(t_flit->m_isStore == false || t_flit->get_route().src_router != m_router->get_id() || m_vc_as_cache.size() == m_vc_per_vnet-1){
            // It is not to be stored, hence continue with the next pipeline stage.
            if (pipe_stages == 1) {
                // 1-cycle router
                // Flit goes for SA directly
                t_flit->advance_stage(SA_, curTick());
            } else {
                assert(pipe_stages > 1);
                // Router delay is modeled by making flit wait in buffer for
                // (pipe_stages cycles - 1) cycles before going for SA

                Cycles wait_time = pipe_stages - Cycles(1);
                t_flit->advance_stage(SA_, m_router->clockEdge(wait_time));

                // Wakeup the router in that cycle to perform SA
                m_router->schedule_wakeup(Cycles(wait_time));
            }
        }else{
            m_vc_as_cache.insert(vc);
            set_vc_busy_store(vc, curTick());
            std::cout << "Packet [ " << t_flit->getPacketID() << " ] kept inside the VC ( " << vc << " ) [ Router : " << m_router->get_id() << " ] and wont go anywhere untill otherwise stated !" << std::endl;
        }

        if (m_in_link->isReady(curTick())) {
            m_router->schedule_wakeup(Cycles(1));
        }
    }
}

// Send a credit back to upstream router for this VC.
// Called by SwitchAllocator when the flit in this VC wins the Switch.
void
InputUnit::increment_credit(int in_vc, bool free_signal, Tick curTime)
{
    DPRINTF(RubyNetwork, "Router[%d]: Sending a credit vc:%d free:%d to %s\n",
    m_router->get_id(), in_vc, free_signal, m_credit_link->name());
    Credit *t_credit = new Credit(in_vc, free_signal, curTime);
    creditQueue.insert(t_credit);
    m_credit_link->scheduleEventAbsolute(m_router->clockEdge(Cycles(1)));
}

bool
InputUnit::functionalRead(Packet *pkt, WriteMask &mask)
{
    bool read = false;
    for (auto& virtual_channel : virtualChannels) {
        if (virtual_channel.functionalRead(pkt, mask))
            read = true;
    }

    return read;
}

uint32_t
InputUnit::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;
    for (auto& virtual_channel : virtualChannels) {
        num_functional_writes += virtual_channel.functionalWrite(pkt);
    }

    return num_functional_writes;
}

void
InputUnit::resetStats()
{
    for (int j = 0; j < m_num_buffer_reads.size(); j++) {
        m_num_buffer_reads[j] = 0;
        m_num_buffer_writes[j] = 0;
    }
}

} // namespace garnet
} // namespace ruby
} // namespace gem5

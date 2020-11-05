/**
 * @file src/metax/router.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::router
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "router.hpp"
//#include "link_peer.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/ThreadPool.h>

// Headers from standard libraries
#include <sstream>

leviathan::metax::ID32
leviathan::metax::router::
get_request_id()
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = m_request_counter;
        ++m_request_counter;
        m_request_counter = (m_request_counter & ROUTER_REQUEST_UPPER_BOUND) |
                                        ROUTER_REQUEST_LOWER_BOUND;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return id;
}

leviathan::metax::UUID
leviathan::metax::router::
get_tender_id()
{
        METAX_TRACE(__FUNCTION__);
        Poco::UUIDGenerator& generator =
                Poco::UUIDGenerator::defaultGenerator();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return generator.createRandom();
}

void
leviathan::metax::router::
onTimer(Poco::Timer&)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Mutex::ScopedLock lock(m_timeout_mutex);
        m_router_timeout = true;
        wake_up();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_config_input()
{
        if (! config_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_config_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        config_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
configure_router()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const router_cfg_package& in = (*config_rx);
        poco_assert(metax::router_config == in.cmd);
        MAX_HOPS = in.max_hops;
        RESPONSE_TIME = in.peer_response_wait_time * 1000000;
        m_is_configured = true;
        METAX_INFO("Received max hop count from config: "
                        + platform::utils::to_string(MAX_HOPS));
        METAX_INFO("Received hop response time value: "
                        + platform::utils::to_string(RESPONSE_TIME));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_config_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        switch ((*config_rx).cmd) {
                case metax::router_config: {
                        configure_router();
                        break;
                } default: {
                        METAX_WARNING("This case is not handled yet!!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

unsigned int leviathan::metax::router::
get_command_rating(metax::command cmd)
{
        switch (cmd) {
                case metax::on_update_register:
                case metax::on_update_unregister:
                case metax::save:
                case metax::save_stream:
                        return 50;
                case metax::del:
                case metax::update:
                        return 5;
                case metax::get:
                        return 10;
                default:
                        return 0;
        }
        poco_assert("Shouldn't reach here" == 0);
        return 0;
}

void leviathan::metax::router::
post_link_kernel_tender(tender& t, const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        Poco::Timestamp now;
        METAX_INFO("test_log!sending_tender!id!" +
                        t.tender_id.toString() + "!time!" +
                        platform::utils::to_string(
                                now.epochMicroseconds()) + '!');
        router_data data;
        t.write_router_data(data);
        --data.hop_id;
        data.set_payload(in);
        router_link_package& out = *link_tx;
        out.cmd_rating = get_command_rating(t.cmd);
        out.should_block_by_sync = metax::get == t.cmd ? true : false;
        out.cmd = metax::broadcast;
        data.serialize(out);
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_get()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::get == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_DEBUG("Constructing tender for kernel get: tender id - " +
                        t_id.toString() + ", uuid - " + t.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_save_stream()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::save_stream == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_find_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::find_peer == in.cmd
                        || metax::notify_update == in.cmd
                        || metax::notify_delete == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.data_version = in.data_version;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_tender_data(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        router_data data;
        t.write_router_data(data);
        --data.hop_id;
        data.cmd = in.cmd;
        data.set_payload(in); /// in.payload is assigned to data's payload
        router_link_package& out = *link_tx;
        out.cmd = metax::send;
        poco_assert(! t.prs.empty());
        out.peers.insert(*(t.prs.begin()));
        //if (metax::livestream_content != in.cmd
        //                && metax::get_livestream_content != in.cmd) {
        //        t.prs.erase(t.prs.begin());
        //}
        data.serialize(out); // serialize router_data as payload
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_tender_data(tender& t, metax::command cmd, platform::default_package p)
{
        METAX_TRACE(__FUNCTION__);
        router_data data;
        t.write_router_data(data);
        --data.hop_id;
        data.cmd = cmd;
        data.set_payload(p);
        router_link_package& out = *link_tx;
        out.cmd = metax::send;
        poco_assert(! t.prs.empty());
        out.peers.insert(*(t.prs.begin()));
        data.serialize(out);
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_offer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return; // notify kernel that request is not commited.
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_tenders.find(t_id);
        if (m_tenders.end() == j) {
                return; // notify kernel that request is not commited.
        }
        tender& t = j->second;
        if (is_first_command_of_multiwinner_tender(t.cmd)) {
                handle_kernel_offer_for_multiwinner_tender(t);
        } else {
                handle_kernel_data_transfer();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_offer_for_multiwinner_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        if (tender::announced != t.st) {
                METAX_INFO("Tender already expired:  " + t.tender_id.toString());
                return;
        }
        poco_assert(tender::remote == t.typ);
        t.hop_id = MAX_HOPS;
        response res(LINK_REQUEST_UPPER_BOUND);
        res.offers.push_back(0);
        m_responses[t.tender_id].push_back(res);
        t.time.update();
        METAX_INFO("Kernel sent offer or auth request for:  "
                        + t.tender_id.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_accept_for_multiwinner_tender()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator ri = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == ri) {
                return; // notify kernel that request is not commited.
        }
        const UUID& t_id = ri->second;
        tenders::iterator ti = m_tenders.find(t_id);
        if (m_tenders.end() == ti) {
                return; // notify kernel that request is not commited.
        }
        tender& t = ti->second;
        if (is_first_command_of_multiwinner_tender(t.cmd)) {
                send_accept_to_winner_peers(t);
                t.idling_interval = t.hop_id * RESPONSE_TIME;
                Poco::Timestamp now;
                METAX_INFO("test_log!sending_tender!id!" +
                                t.tender_id.toString() + "!time!" +
                                platform::utils::to_string(
                                        now.epochMicroseconds()) + '!');
                t.time.update();
        } else {
                handle_kernel_data_transfer();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_confirm_for_multiwinner_tender()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return; // notify kernel that request is not commited.
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_tenders.find(t_id);
        if (m_tenders.end() == j) {
                return; // notify kernel that request is not commited.
        }
        tender& t = j->second;
        //poco_assert(tender::confirmed == t.st || tender::transporting == t.st);
        //poco_assert(tender::remote == t.typ || tender::internal == t.typ);
        poco_assert(tender::remote == t.typ);
        t.hop_id = MAX_HOPS;
        send_tender_data(t);
        t.time.update();
        METAX_DEBUG("Passing tender to link:  " + t_id.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_data_transfer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return; // notify kernel that request is not commited.
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_tenders.find(t_id);
        if (m_tenders.end() == j) {
                return; // notify kernel that request is not commited.
        }
        tender& t = j->second;
        poco_assert(tender::confirmed == t.st || tender::received == t.st);
        t.st = tender::confirmed;
        poco_assert(tender::remote == t.typ || tender::internal == t.typ);
        t.hop_id = MAX_HOPS;
        if (metax::recording_started == in.cmd) {
                t.uuid = in.uuid;
        }
        if (metax::livestream_content == in.cmd) {
                t.data_version = in.data_version;
        }
        send_tender_data(t);
        t.time.update();
        METAX_DEBUG("Passing tender to link:  " + t_id.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_storage_data_transfer(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return; // notify kernel that request is not commited.
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_tenders.find(t_id);
        if (m_tenders.end() == j) {
                return; // notify kernel that request is not commited.
        }
        tender& t = j->second;
        poco_assert(tender::confirmed == t.st || tender::received == t.st);
        t.st = tender::confirmed;
        poco_assert(tender::remote == t.typ || tender::internal == t.typ);
        t.hop_id = MAX_HOPS;
        send_tender_data(t, in.cmd, in);
        t.time.update();
        METAX_DEBUG("Passing tender to link:  " + t_id.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_sync_data_transfer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return;
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_sync_tenders.find(t_id);
        if (m_sync_tenders.end() == j) {
                return;
        }
        tender& t = j->second;
        if (metax::send_uuids == in.cmd) {
                post_sync_started_request_to_link_peer(t);
        }
        send_tender_data(t);
        // Update sync tender ideling interval
        t.idling_interval = SYNC_TENDER_RESPONSE_TIME;
        t.time.update();
        METAX_DEBUG("Passing sync tender to link:  " + t_id.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_get_uuid_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        request_tender_map::const_iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return;
        }
        tenders::iterator j = m_tenders.find(i->second);
        if (m_tenders.end() == j) {
                return;
        }
        tender& t = j->second;
        t.data_version = in.data_version;
        t.last_updated = in.last_updated;
        t.expire_date = in.expire_date;
        t.uuid = in.uuid;
        send_tender_data(t);
        // Update sync tender idling interval
        t.idling_interval = SYNC_TENDER_RESPONSE_TIME;
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_save_request(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(is_first_command_of_multiwinner_tender(in.cmd));
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.data_version = in.data_version;
        t.last_updated = in.last_updated;
        t.expire_date = in.expire_date;
        METAX_INFO("received save request for uuid: " + in.uuid.toString());
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_on_update_register_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::on_update_register == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_DEBUG("Constructing tender for kernel on update register: tender id - " +
                        t_id.toString() + ", uuid - " + t.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_recording_start()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::recording_start == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_DEBUG("Constructing tender for kernel recording start: tender id - " +
                        t_id.toString() + ", uuid - " + t.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_recording_stop()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::recording_stop == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_DEBUG("Constructing tender for kernel recording stop: tender id - " +
                        t_id.toString() + ", uuid - " + t.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_on_update_unregister_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::on_update_unregister == in.cmd);
        poco_assert(m_id_tenders.end() == m_id_tenders.find(in.request_id));
        UUID t_id = get_tender_id();
        poco_assert(m_tenders.end() == m_tenders.find(t_id));
        tender& t = m_tenders[t_id];
        t.max_response_cnt = in.max_response_cnt;
        t.request_id = in.request_id;
        t.tender_id = t_id;
        t.cmd = in.cmd;
        t.uuid = in.uuid;
        t.typ = tender::internal;
        t.st = tender::announced;
        t.hop_id = MAX_HOPS;
        t.idling_interval = t.hop_id * RESPONSE_TIME; // for round trip
        m_tenders_timeline.push_back(&t);
        m_id_tenders.insert(std::make_pair(in.request_id, t_id));
        post_link_kernel_tender(t, in);
        t.time.update();
        METAX_DEBUG("Constructing tender for kernel on update register: tender id - " +
                        t_id.toString() + ", uuid - " + t.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
broadcast_tender_to_peers(tender& t, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        router_link_package& out = *link_tx;
        router_data data;
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        t.write_router_data(data);
        --data.hop_id;
        data.set_payload(pkg);
        data.serialize(out);
        out.cmd_rating = get_command_rating(t.cmd);
        out.should_block_by_sync = metax::get == t.cmd ? true : false;
        out.cmd = metax::broadcast_except;
        poco_assert(1 == t.prs.size());
        out.peers = t.prs;
        //t.src = *(t.prs.begin());
        link_tx.commit();
        //poco_assert(tender::received == t.st || tender::confirmed == t.st);
        poco_assert(tender::remote == t.typ);
        //t.st = tender::announced;
        t.request_id = UINT32_MAX;
        t.time.update();
        //t.prs.erase(t.prs.begin());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_accept_to_winner_peers(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        std::map<ID32, std::vector<ID32>> wm;
        for (size_t i = 0; i < t.winners.size(); ++i) {
                wm[t.winners[i].first].push_back(t.winners[i].second);
        }
        for (auto wi = wm.begin(); wi != wm.end(); ++wi) {
                METAX_DEBUG("Passing tender to winner peer:  "
                                                + t.tender_id.toString());
                                                //+ wi->first->address());
                poco_assert(tender::confirmed == t.st ||
                                tender::received == t.st);
                t.st = tender::confirmed;
                poco_assert(tender::remote == t.typ ||
                                tender::internal == t.typ);
                t.hop_id = MAX_HOPS;
                router_data data;
                t.write_router_data(data);
                --data.hop_id;
                data.cmd = in.cmd;
                data.winners_id = wi->second;
                data.set_payload(in); /// in.payload is assigned to data's payload
                router_link_package& out = *link_tx;
                out.cmd = metax::send;
                out.peers.insert(wi->first);
                data.serialize(out); // serialize router_data as payload
                link_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
forward_remote_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        router_link_package& out = *link_tx;
        router_data data;
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        t.write_router_data(data);
        --data.hop_id;
        data.extract_payload(*link_rx, m_router_data.header_size());
        data.serialize(out);
        out.cmd_rating = get_command_rating(t.cmd);
        out.should_block_by_sync = metax::get == t.cmd ? true : false;
        out.cmd = metax::broadcast_except;
        poco_assert(1 == t.prs.size());
        out.peers = t.prs;
        //t.src = *(t.prs.begin());
        link_tx.commit();
        //poco_assert(tender::announced == t.st);
        poco_assert(tender::remote == t.typ);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_refuse(ID32 rid, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        request_tender_map::iterator i = m_id_tenders.find(rid);
        if (m_id_tenders.end() == i) {
                return;
        }
        const UUID& t_id = i->second;
        tenders::iterator j = m_tenders.find(t_id);
        if (m_tenders.end() == j) {
                return;
        }
        tender& t = j->second;
        if (1 == t.hop_id) {
                clean_up_tender(t);
                return;
        }
        METAX_DEBUG("Tender refused, broadcasting to peers:  "
                        + t.tender_id.toString());
        broadcast_tender_to_peers(t, pkg);
        m_id_tenders.erase(i);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_backup_input()
{
        if (! backup_rx.has_data()) {
                return;
        }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_backup_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        backup_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_backup_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(backup_rx.has_data());
        const kernel_router_package& in = *backup_rx;
        switch(in.cmd) {
                case metax::update:
                case metax::save: {
                        handle_kernel_save_request(in);
                        break;
                } default: {
                        METAX_ERROR("Unexpected command");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_input()
{
        if (! kernel_rx.has_data()) {
                return; }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_kernel_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        kernel_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_kernel_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        switch(in.cmd) {
                case metax::get: {
                        handle_kernel_get();
                        break;
                } case metax::update:
                  case metax::del:
                  case metax::copy:
                  case metax::save: {
                        handle_kernel_save_request(in);
                        break;
                } case metax::on_update_register: {
                        handle_kernel_on_update_register_request();
                        break;
                } case metax::on_update_unregister: {
                        handle_kernel_on_update_unregister_request();
                        break;
                } case metax::save_stream: {
                        handle_kernel_save_stream();
                        break;
                } case metax::recording_start: {
                        handle_kernel_recording_start();
                        break;
                } case metax::recording_stop: {
                        handle_kernel_recording_stop();
                        break;
                } case metax::notify_update:
                  case metax::notify_delete:
                  case metax::find_peer: {
                        handle_kernel_find_peer();
                        break;
                } case metax::refuse: {
                        handle_refuse(in.request_id, in);
                        break;
                } case metax::clear_request: {
                        handle_kernel_clear_request();
                        break;
                } case metax::auth:
                  case metax::save_offer: {
                        handle_kernel_offer();
                        break;
                } case metax::auth_data:
                  case metax::save_data: {
                        handle_kernel_accept_for_multiwinner_tender();
                        break;
                } case metax::updated:
                  case metax::get_data:
                  case metax::deleted:
                  case metax::copy_succeeded:
                  case metax::save_confirm: {
                        handle_kernel_confirm_for_multiwinner_tender();
                        break;
                } case metax::send_journal_info:
                  case metax::send_uuids: {
                        handle_sync_data_transfer();
                        break;
                } case metax::sync_finished: {
                        handle_kernel_sync_finished();
                        break;
                } case metax::get_uuid: {
                        handle_kernel_get_uuid_request();
                        break;
                } case metax::peer_found:
                  case metax::send_to_peer:
                  case metax::send_to_peer_confirm:
                  case metax::deliver_fail:
                  case metax::no_permissions:
                  case metax::save_stream_offer:
                  case metax::stream_auth:
                  case metax::livestream_content:
                  case metax::get_livestream_content:
                  case metax::cancel_livestream_get:
                  case metax::recording_started:
                  case metax::recording_stopped:
                  case metax::recording_fail:
                  case metax::get_journal_info:
                  case metax::on_update_registered:
                  case metax::on_update_register_fail:
                  case metax::on_update_unregister_fail:
                  case metax::on_update_unregistered: {
                        handle_kernel_data_transfer();
                        break;
                } default: {
                        METAX_WARNING("Unexpected command");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_kernel_clear_request()
{
        const kernel_router_package& in = *kernel_rx;
        poco_assert(kernel_rx.has_data());
        request_tender_map::iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return;
        }
        const UUID& t_id = i->second;
        METAX_DEBUG("Processing kernel clear request:  " + t_id.toString());
        tenders::iterator j = m_tenders.find(t_id);
        poco_assert(m_tenders.end() != j);
        tender& t = j->second;
        t.is_canceled = true;
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        t.time.update();
        //clean_up_tender_info(t);
}

void leviathan::metax::router::
handle_kernel_sync_finished()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(kernel_rx.has_data());
        const kernel_router_package& in = *kernel_rx;
        poco_assert(metax::sync_finished == in.cmd);
        request_tender_map::iterator i = m_id_tenders.find(in.request_id);
        if (m_id_tenders.end() == i) {
                return;
        }
        const UUID& t_id = i->second;
        METAX_DEBUG("Processing sync finish request from kernel:  " +
                        t_id.toString());
        tenders::iterator j = m_sync_tenders.find(t_id);
        if (m_sync_tenders.end() == j) {
                return;
        }
        tender& t = j->second;
        post_sync_finished_request_to_link_peer(t);
        send_tender_data(t);
        clean_up_sync_tender_info(t);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
post_sync_finished_request_to_link_peer(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        router_link_package& out = *link_tx;
        out.cmd = metax::sync_finished;
        poco_assert(! t.prs.empty());
        out.peers.insert(*(t.prs.begin()));
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
post_sync_started_request_to_link_peer(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        router_link_package& out = *link_tx;
        out.cmd = metax::sync_started;
        poco_assert(! t.prs.empty());
        out.peers.insert(*(t.prs.begin()));
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
forward_data_to_winner_peer(tender& t, ID32 p,
                                const std::vector<ID32>& w)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(tender::confirmed == t.st || tender::received == t.st);
        poco_assert(tender::remote == t.typ || tender::internal == t.typ);
        poco_assert(LINK_REQUEST_UPPER_BOUND != p);
        router_data data;
        t.write_router_data(data);
        --data.hop_id;
        data.cmd = m_router_data.cmd;
        data.winners_id = w;
        data.extract_payload(*link_rx, m_router_data.header_size());
        router_link_package& out = *link_tx;
        out.cmd = metax::send;
        out.peers.insert(p);
        data.serialize(out); // serialize router_data as payload
        link_tx.commit();
        METAX_INFO("forward tender data to winner peer: "
                        + platform::utils::to_string(p));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_peer_accept_for_multiwinner_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(is_accept_command_of_multiwinner_tender(
                                                m_router_data.cmd, t.cmd));
        poco_assert(is_accept_command_of_multiwinner_tender(
                                m_router_data.cmd, t.cmd));
        std::map<ID32, std::vector<ID32>> winners_by_peer;
        for (size_t i = 0; i < m_router_data.winners_id.size(); ++i) {
                size_t wid = m_router_data.winners_id[i];
                poco_assert(t.winners.size() > wid);
                winners_by_peer[t.winners[wid].first].push_back(t.winners[i].second);
        }
        for (auto wi = winners_by_peer.begin(); wi != winners_by_peer.end(); ++wi) {
                if (LINK_REQUEST_UPPER_BOUND == wi->first) {
                        send_incoming_tender_data_to_kernel(t);
                } else {
                        forward_data_to_winner_peer(t, wi->first, wi->second);

                }
        }
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
post_new_tender_to_kernel(ID32 r_id)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        kernel_router_package& out = *kernel_tx;
        out.request_id = r_id;
        out.cmd = m_router_data.cmd;
        out.uuid = m_router_data.uuid;
        out.data_version = m_router_data.data_version;
        out.last_updated = m_router_data.last_updated;
        out.expire_date = m_router_data.expire_date;
        out.extract_payload(*link_rx, m_router_data.header_size());
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::router::
is_invalid_new_tender(command cmd) const
{
        return metax::get != cmd &&
                metax::find_peer != cmd &&
                metax::save != cmd &&
                metax::update != cmd &&
                metax::save_stream != cmd &&
                metax::del != cmd &&
                metax::notify_update != cmd &&
                metax::notify_delete != cmd &&
                metax::copy != cmd &&
                metax::on_update_register != cmd &&
                metax::on_update_unregister != cmd &&
                metax::send_journal_info != cmd &&
                metax::recording_start != cmd &&
                metax::recording_stop != cmd;
}

void leviathan::metax::router::
handle_new_remote_tender()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        poco_assert(UUID::null() != m_router_data.tender_id);
        poco_assert(metax::max_command != m_router_data.cmd);
        poco_assert(m_tenders.end() == m_tenders.find(m_router_data.tender_id));
        if (is_invalid_new_tender(m_router_data.cmd)) {
                return;
        }
        METAX_INFO("New valid tender: " + m_router_data.tender_id.toString());
        tender& t = m_tenders[m_router_data.tender_id];
        process_new_tender(t);
        if (is_first_command_of_multiwinner_tender(m_router_data.cmd)) {
                //t.st = tender::announced;
                if (1 != t.hop_id) {
                        forward_remote_tender(t);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_new_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const router_link_package& in = *link_rx;
        t.read_router_data(m_router_data);
        t.prs.insert(in.peers.begin(), in.peers.end());
        t.request_id = get_request_id();
        t.st = tender::received;
        t.typ = tender::remote;
        t.idling_interval = KERNEL_RESPONSE_TIME;
        post_new_tender_to_kernel(t.request_id);
        m_id_tenders.insert(std::make_pair(t.request_id, t.tender_id));
        m_tenders_timeline.push_back(&t);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_new_sync_tender()
{
        METAX_TRACE(__FUNCTION__);
        if (metax::sync_finished == m_router_data.cmd
                        || metax::get_uuid == m_router_data.cmd) {
                METAX_INFO("Sync tender is already cleaned up: "
                                + m_router_data.tender_id.toString());
                return;
        }
        poco_assert(metax::get_journal_info == m_router_data.cmd
                        || metax::start_storage_sync == m_router_data.cmd);
        poco_assert(m_sync_tenders.end() ==
                        m_sync_tenders.find(m_router_data.tender_id));
        METAX_INFO("New valid sync tender: "
                        + m_router_data.tender_id.toString());
        tender& t = m_sync_tenders[m_router_data.tender_id];
        process_new_tender(t);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_sync_tenders()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(is_own_sync_tender(m_router_data.cmd));
        using U = platform::utils;
        Poco::Timestamp now;
        METAX_INFO("Received tender id=" + m_router_data.tender_id.toString() +
                        " time=" + U::to_string(now.epochMicroseconds())) +
                        " size=" + U::to_string(m_sync_tenders.size());
        tenders::iterator i = m_sync_tenders.find(m_router_data.tender_id);
        if (m_sync_tenders.end() == i) {
                handle_new_sync_tender();
        } else {
                process_existing_sync_tender(i->second);
        }
        m_router_data.reset();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_incoming_tender_data_to_kernel(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::max_command != m_router_data.cmd);
        METAX_DEBUG("Sending tender to kernel:  " + t.tender_id.toString());
        poco_assert(UINT32_MAX != t.request_id);
        t.st = tender::confirmed;
        t.prs.insert((*link_rx).peers.begin(), (*link_rx).peers.end());
        poco_assert(tender::confirmed == t.st);
        kernel_router_package& out = *kernel_tx;
        out.request_id = t.request_id;
        out.cmd = m_router_data.cmd;
        out.uuid = m_router_data.uuid;
        out.extract_payload(*link_rx, m_router_data.header_size());
        kernel_tx.commit();
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::router::
is_first_command_of_multiwinner_tender(metax::command cmd) const
{
        return metax::save == cmd
                || metax::update == cmd
                || metax::notify_update == cmd
                || metax::notify_delete == cmd
                || metax::del == cmd
                || metax::copy == cmd;
}

bool leviathan::metax::router::
is_confirm_command_of_multiwinner_tender(metax::command cmd) const
{
        return metax::save_confirm == cmd
                || metax::updated == cmd
                || metax::deleted == cmd
                || metax::copy_succeeded == cmd;
}

bool leviathan::metax::router::
is_accept_command_of_multiwinner_tender(metax::command rcmd,
                metax::command tcmd) const
{
        return metax::save_data == rcmd || (metax::auth_data == rcmd
                    && (metax::update == tcmd || metax::del == tcmd
                            || metax::copy == tcmd));

}

bool leviathan::metax::router::
is_offer_command_of_multiwinner_tender(metax::command rcmd,
                metax::command tcmd) const
{
        return metax::save_offer == rcmd || (metax::auth == rcmd
                    && (metax::del == tcmd || metax::update == tcmd
                            || metax::copy == tcmd));
}

void leviathan::metax::router::
process_remote_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        if (((metax::livestream_content == m_router_data.cmd
                        || metax::send_to_peer == m_router_data.cmd
                        || metax::cancel_livestream_get == m_router_data.cmd)
                        && tender::confirmed == t.st)
                                || metax::send_uuids == m_router_data.cmd) {
                send_response_to_kernel(&(*link_rx));
        } else {
                bypass_tender_response(&(*link_rx));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_existing_sync_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        t.st = tender::confirmed;
        t.idling_interval = KERNEL_RESPONSE_TIME;
        post_new_tender_to_kernel(t.request_id);
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_existing_tender_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_tenders.end() != m_tenders.find(m_router_data.tender_id));
        poco_assert(metax::max_command != m_router_data.cmd);
        poco_assert(UUID::null() != m_router_data.tender_id);
        tender& t = m_tenders[m_router_data.tender_id];
        if (t.is_canceled) {
                METAX_DEBUG("Received canceled tender: "
                                + t.tender_id.toString());
                return;
        }
        switch (t.typ) {
                case tender::remote: {
                        process_remote_tender(t);
                        break;
                } case tender::internal: {
                        send_response_to_kernel(&(*link_rx));
                        break;
                } default: {
                        METAX_ERROR("This tneder is not handled yet");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::router::
process_existing_tender()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_tenders.end()
                        != m_tenders.find(m_router_data.tender_id));
        poco_assert(metax::max_command != m_router_data.cmd);
        poco_assert(UUID::null() != m_router_data.tender_id);
        if (metax::get == m_router_data.cmd ||
            metax::save == m_router_data.cmd ||
            metax::update == m_router_data.cmd ||
            metax::del == m_router_data.cmd ||
            metax::copy == m_router_data.cmd ||
            metax::on_update_register == m_router_data.cmd ||
            metax::find_peer == m_router_data.cmd) {
                return;
        } else if (metax::sync_finished == m_router_data.cmd) {
                METAX_DEBUG("Received sync finished from peer for tender : "
                                + m_router_data.tender_id.toString());
                return clean_up_tender_info(m_tenders[m_router_data.tender_id]);
        }
        METAX_INFO("Valid existing tender: "
                        + m_router_data.tender_id.toString());
        poco_assert(tender::internal == m_tenders[m_router_data.tender_id].typ
                || tender::remote == m_tenders[m_router_data.tender_id].typ);
        process_existing_tender_data();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::router::
is_own_sync_tender(command cmd) const
{ 
        return metax::start_storage_sync == cmd
                || metax::get_journal_info == cmd
                || metax::get_uuid == cmd;
}

void leviathan::metax::router::
send_notification_to_kernel()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const router_link_package& in = *link_rx;
        poco_assert(metax::connected == in.cmd
                        || metax::disconnected == in.cmd);
        kernel_router_package& out = *kernel_tx;
        out.cmd = in.cmd;
        out = in;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_link_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        m_router_data.extract_header(*link_rx);
        if (UUID::null() == m_router_data.tender_id) {
                m_router_data.tender_id = get_tender_id();
        }
        if (is_own_sync_tender(m_router_data.cmd)) {
                return handle_sync_tenders();
        }
        poco_assert(Poco::UUID::null() != m_router_data.tender_id);
        poco_assert(metax::max_command != m_router_data.cmd);
        tenders::iterator i = m_tenders.find(m_router_data.tender_id);
        using U = platform::utils;
        METAX_INFO("Tenders size " + U::to_string(m_tenders.size()) +
                        ". Tender ID " + m_router_data.tender_id.toString());
        Poco::Timestamp now;
        METAX_INFO("test_log!received_tender!id!" +
                        m_router_data.tender_id.toString() + "!time!" +
                        U::to_string(now.epochMicroseconds()) + '!');
        if (m_tenders.end() == i) {
                handle_new_remote_tender();
        } else {
                process_existing_tender();
        }
        m_router_data.reset();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_link_nack()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        m_router_data.extract_header(*link_rx);
        poco_assert(UUID::null() != m_router_data.tender_id);
        if (m_tenders.end() != m_tenders.find(m_router_data.tender_id)) {
                tender& t = m_tenders[m_router_data.tender_id];
                clean_up_tender(t);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_link_input()
{
        if (! link_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_link_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        link_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_link_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(link_rx.has_data());
        const router_link_package& in = *link_rx;
        switch (in.cmd) {
                case metax::nack: {
                        handle_link_nack();
                        break;
                } case metax::received_data: {
                        handle_link_data();
                        break;
                } case metax::connected:
                  case metax::disconnected: {
                        send_notification_to_kernel();
                        break;
                } default : {
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_wallet_input()
{
        if (! wallet_rx.has_data()) {
                return;
        }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_wallet_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        wallet_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_wallet_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(wallet_rx.has_data());
        std::string s((*wallet_rx).message);
        METAX_INFO("Got message from wallet: " + s);
        (*wallet_tx).set_payload(std::string(name() + ": " + " end of connections"));
        wallet_tx.commit();
        wallet_tx.deactivate();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
cancel_request(ID32 r_id)
{
        METAX_TRACE(__FUNCTION__);
        /// Means that tender is not accosiated with request in kernel.
        if (UINT32_MAX == r_id) {
                return;
        }
        kernel_router_package& out = *kernel_tx;
        out.cmd = metax::cancel;
        out.request_id = r_id;
        kernel_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
clean_up_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        ID32 tid = t.request_id;
        bool canceled = t.is_canceled;
        if (metax::start_storage_sync == t.cmd
                        || metax::get_journal_info == t.cmd) {
                clean_up_sync_tender_info(t);
        } else {
                clean_up_tender_info(t);
        }
        if (! canceled) {
                cancel_request(tid);
        }
        METAX_DEBUG("m_tenders size = "
                        + platform::utils::to_string(m_tenders.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
print_tenders_debug_info()
{
        using U = platform::utils;
        METAX_DEBUG("m_tenders size = " + U::to_string(m_tenders.size()));
        METAX_DEBUG("m_responses size = " + U::to_string(m_responses.size()));
        METAX_DEBUG("m_sync_tenders size = " + 
                        U::to_string(m_sync_tenders.size()));
}

void leviathan::metax::router::
clean_up_tender_from_timeline(tender& t)
{
        tenders_timeline::iterator i = std::find(m_tenders_timeline.begin(),
                        m_tenders_timeline.end(), &t);
        poco_assert(m_tenders_timeline.end() != i);
        poco_assert(0 != *i);
        tenders_timeline::size_type j = 
                static_cast<tenders_timeline::size_type>(i -
                                m_tenders_timeline.begin());
        tenders_timeline::size_type size = m_tenders_timeline.size();
        poco_assert(m_tenders_timeline[j] == &t);      
        tender* tmp = m_tenders_timeline[size - 1];
        m_tenders_timeline[size - 1] = m_tenders_timeline[j];
        m_tenders_timeline[j] = tmp;
        m_tenders_timeline.pop_back();
}

void leviathan::metax::router::
clean_up_sync_tender_info(tender& t)
{
        poco_assert(metax::start_storage_sync == t.cmd
                        || metax::get_journal_info == t.cmd);
        clean_up_tender_from_timeline(t);
        m_id_tenders.erase(t.request_id);
        m_sync_tenders.erase(t.tender_id);
        if (m_sync_tenders.empty()) {
                (*kernel_tx).cmd = metax::sync_finished;
                kernel_tx.commit();
        }
        print_tenders_debug_info();
}

void leviathan::metax::router::
clean_up_tender_info(tender& t)
{
        /// Clean_up m_id_tenders
        // clean_up m_tenders
        // clean_up m_tenders_timeline
        METAX_TRACE(__FUNCTION__);
        clean_up_tender_from_timeline(t);
        m_id_tenders.erase(t.request_id);
        m_responses[t.tender_id].clear();
        m_responses.erase(t.tender_id);
        m_tenders.erase(t.tender_id);
        print_tenders_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
select_all_responses_as_winners(tender& t)
{
        auto& r = m_responses[t.tender_id];
        poco_assert(metax::del == t.cmd || metax::update == t.cmd
                        || metax::copy == t.cmd);
        for (size_t i = 0; i < r.size(); ++i) {
                for (size_t j = 0; j < r[i].offers.size(); ++j) {
                        t.winners.push_back(std::make_pair(r[i].peer,
                                                r[i].offers[j]));
                }
        }
}

void leviathan::metax::router::
decide_tender_winners_for_save_request(tender& t)
{
        poco_assert(metax::save == t.cmd);
        auto& r = m_responses[t.tender_id];
        size_t j = 0;
        size_t win_count = tender::internal == t.typ ? t.max_response_cnt : 3;
        for (size_t w = 0; w < win_count; ++w) {
                size_t i = 0;
                for (; i < r.size(); ++i) {
                        if (j < r[i].offers.size()) {
                                t.winners.push_back(std::make_pair(r[i].peer,
                                                        r[i].offers[j]));
                                if (win_count <= t.winners.size()) {
                                        return;
                                }
                        }
                }
                ++j;
        }
}

void leviathan::metax::router::
decide_tender_winners(tender& t)
{
        if (metax::del == t.cmd || metax::update == t.cmd
                        || metax::copy == t.cmd) {
                select_all_responses_as_winners(t);
        } else {
                poco_assert(metax::save == t.cmd);
                decide_tender_winners_for_save_request(t);
        }
        METAX_INFO("Winners count for " + t.tender_id.toString()
                        + " tender is "
                        + platform::utils::to_string(t.winners.size()));
}

void leviathan::metax::router::
send_response_to_kernel(const router_link_package* winner)
{
        METAX_TRACE(__FUNCTION__);
        router_data best;
        best.extract_header(*winner);
        best.extract_payload(*winner, best.header_size());
        poco_assert(m_tenders.end() != m_tenders.find(best.tender_id));
        tender& t = m_tenders[best.tender_id];
        if (metax::get_data == best.cmd && tender::confirmed == t.st) {
                METAX_DEBUG("Received response for confirmed get request");
                return;
        }
        t.prs.insert(winner->peers.begin(), winner->peers.end());
        t.st = tender::confirmed;
        t.idling_interval = KERNEL_RESPONSE_TIME;
        kernel_router_package& out = *kernel_tx;
        out.cmd = best.cmd;
        out.uuid = best.uuid;
        METAX_INFO("Sending tender to kernel - uuid: " + best.uuid.toString());
        poco_assert(UINT32_MAX != t.request_id);
        poco_assert(m_id_tenders.end() != m_id_tenders.find(t.request_id));
        out.request_id = t.request_id;
        out.data_version = best.data_version;
        out.set_payload(best);
        kernel_tx.commit();
        t.time.update();
        ++t.received_response_cnt;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
bypass_tender_response(const router_link_package* winner)
{
        METAX_TRACE(__FUNCTION__);
        router_data best;
        best.extract_header(*winner);
        poco_assert(m_tenders.end() != m_tenders.find(best.tender_id));
        tender& t = m_tenders[best.tender_id];
        METAX_INFO("Bypassing tender - uuid: " + best.tender_id.toString());
        poco_assert(UINT32_MAX == t.request_id ||
                        is_first_command_of_multiwinner_tender(t.cmd));
        //t.st = tender::transporting;
        t.hop_id = best.hop_id;
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        router_link_package& out = *link_tx;
        out.set_payload(*winner);
        router_data::update_hop_id(out.message.get(), ID16(t.hop_id - 1));
        out.peers.insert(t.prs.begin(), t.prs.end());
        out.cmd = metax::send;
        if (metax::save_stream_offer == m_router_data.cmd
                        || metax::peer_found == m_router_data.cmd
                        || metax::send_to_peer == m_router_data.cmd) {
                t.prs.clear();
                t.prs.insert(winner->peers.begin(), winner->peers.end());
        }
        link_tx.commit();
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_multiwinner_tender_offer_to_kernel(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        METAX_INFO("Winners decided, sending save offer to kernel: "
                        + t.tender_id.toString());
        kernel_router_package& out = *kernel_tx;
        out.request_id = t.request_id;
        out.cmd = metax::save == t.cmd ? metax::save_offer : metax::auth;
        out.uuid = t.uuid;
        out.winners_count = t.winners.size();
        kernel_tx.commit();
        t.time.update();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
send_multiwinner_tender_offer_to_announcer_peer(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        router_data data;
        t.write_router_data(data);
        data.cmd = metax::save == t.cmd ? metax::save_offer : metax::auth;
        METAX_INFO("Sending winners to announcer");
        for (size_t j = 0; j < t.winners.size(); ++j) {
                data.winners_id.push_back(j);
        }
        router_link_package& out = *link_tx;
        data.serialize(out); // serialize router_data as payload
        out.cmd = metax::send;
        poco_assert(! t.prs.empty());
        out.peers.insert(*(t.prs.begin()));
        t.idling_interval = t.hop_id * RESPONSE_TIME;
        t.time.update();
        Poco::Timestamp now;
        METAX_INFO("test_log!sending_tender!id!" +
                        t.tender_id.toString() + "!time!" +
                        platform::utils::to_string(
                                now.epochMicroseconds()) + '!');
        link_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_multiwinner_tender_responses(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(UINT32_MAX != t.request_id);
        t.st = tender::confirmed;
        decide_tender_winners(t);
        if (t.winners.empty()) {
                METAX_INFO("No winners for:  " + t.tender_id.toString());
                m_responses[t.tender_id].clear();
                m_responses.erase(t.tender_id);
                clean_up_tender(t);
        } else {
                if (tender::internal == t.typ) {
                        send_multiwinner_tender_offer_to_kernel(t);
                } else {
                        send_multiwinner_tender_offer_to_announcer_peer(t);
                }
                m_responses[t.tender_id].clear();
                m_responses.erase(t.tender_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_storage_input()
{
        if (! storage_rx.has_data()) {
                return;
        }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_storage_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        storage_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_storage_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        switch(in.cmd) {
                case metax::get_data:
                case metax::deleted: {
                        handle_storage_data_transfer(in);
                        break;
                } case metax::refuse: {
                        handle_refuse(in.request_id, in);
                        break;
                } default: {
                        METAX_WARNING("Unexpected command");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_storage_writer_input()
{
        if (! storage_writer_rx.has_data()) {
                return;
        }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_storage_writer_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        storage_writer_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_storage_writer_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        switch(in.cmd) {
                case metax::save_confirm:
                case metax::updated: {
                        handle_storage_data_transfer(in);
                        break;
                } default: {
                        METAX_WARNING("Unexpected command");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_cache_input()
{
        if (! cache_rx.has_data()) {
                return;
        }
        if (! m_is_configured) {
                return Poco::Thread::yield();
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_cache_input();
        } catch (const Poco::AssertionViolationException& e) {
                METAX_FATAL(e.displayText());
                abort();
        } catch (const Poco::Exception& e) {
                METAX_ERROR("Unhandled exception:"
                                + e.displayText());
        } catch (const std::exception& e) {
                std::string msg = "Unhandled exception:";
                METAX_ERROR(msg + e.what());
        } catch (...) {
                METAX_ERROR("Unhandled exception:");
        }
        cache_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_cache_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(cache_rx.has_data());
        const kernel_storage_package& in = *cache_rx;
        switch(in.cmd) {
                default: {
                        METAX_WARNING("Unexpected command");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
handle_expired_sync_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        if (! t.is_canceled) {
                cancel_request(t.request_id);
        }
        post_sync_finished_request_to_link_peer(t);
        clean_up_sync_tender_info(t);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
process_expired_tender(tender& t)
{
        METAX_TRACE(__FUNCTION__);
        //if (is_first_command_of_multiwinner_tender(t.cmd)
        //                && tender::announced == t.st) {
        //        handle_multiwinner_tender_responses(t);
        //} else {
                METAX_DEBUG("Cleaning up expired tender:  " + t.tender_id.toString());
                if (metax::start_storage_sync == t.cmd
                                || metax::get_journal_info == t.cmd) {
                        return handle_expired_sync_tender(t);
                }
                responses::iterator i = m_responses.find(t.tender_id);
                if (m_responses.end() != i) {
                        m_responses[t.tender_id].clear();
                        m_responses.erase(t.tender_id);
                        METAX_DEBUG("Tender had response, cleaning");
                }
                clean_up_tender(t);
        //}
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::router::tender*
leviathan::metax::router::
get_expired_tender()
{
        Poco::Mutex::ScopedLock lock(m_timeout_mutex);
        if (! m_router_timeout && ! m_expired_tender_exists) {
                return nullptr;
        } else if (m_tenders_timeline.empty()) {
                m_router_timeout = false;
                reset();
                return nullptr;
        }
        m_router_timeout = false;
        // reset task signal if it is set upon router timeout
        reset();
        std::make_heap(m_tenders_timeline.begin(), m_tenders_timeline.end(), tender::less_than);
        tender* t = (m_tenders_timeline.front());
        if (! t->is_expired()) {
                return nullptr;
        }
        return t;
}

void leviathan::metax::router::
handle_expired_tenders()
{
        tender* t = get_expired_tender();
        if (nullptr == t) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        process_expired_tender(*t);
        m_expired_tender_exists = false;
        if (! m_tenders_timeline.empty()) {
                std::make_heap(m_tenders_timeline.begin(),
                               m_tenders_timeline.end(), tender::less_than);
                tender* tf = m_tenders_timeline.front();
                poco_assert(nullptr != tf);
                m_expired_tender_exists = tf->is_expired();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::router::
wait_for_input_channels()
{
        while(true) {
                /// if expired tender exists go and process it.
                if (! m_expired_tender_exists) {
                        if (! wait()) { /// Wait for inputs/signal
                                break;
                        }
                }
                handle_config_input();
                handle_kernel_input();
                handle_storage_input();
                handle_storage_writer_input();
                handle_cache_input();
                handle_backup_input();
                handle_wallet_input();
                handle_link_input();
                handle_expired_tenders();
        }
}

void leviathan::metax::router::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        Poco::TimerCallback<router> onTimerCallback(*this, &router::onTimer);
        m_tenders_timer.start(onTimerCallback, Poco::ThreadPool::defaultPool());
        wait_for_input_channels();
        m_tenders_timer.stop();
        finish();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
} catch (const Poco::AssertionViolationException& e) {
        METAX_FATAL(e.displayText());
        abort();
} catch(const Poco::Exception& e) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL(e.displayText());
        std::terminate();
} catch(...) {
        std::cerr << "Fatal error." << std::endl;
        METAX_FATAL("Unhadled exception.");
        std::terminate();
}

leviathan::metax::router::
router()
        : leviathan::platform::csp::task("router", Poco::Logger::get("metax.router"))
        , kernel_rx(this)
        , config_rx(this)
        , wallet_rx(this)
        , link_rx(this)
        , backup_rx(this)
        , storage_rx(this)
        , storage_writer_rx(this)
        , cache_rx(this)
        , kernel_tx(this)
        , config_tx(this)
        , wallet_tx(this)
        , link_tx(this)
        , backup_tx(this)
        , m_tenders()
        , m_sync_tenders()
        , m_responses()
        , m_tenders_timeline()
        , m_id_tenders()
        , m_request_counter(ROUTER_REQUEST_LOWER_BOUND)
        , MAX_HOPS(3)
        , RESPONSE_TIME(30000000) // 30s
        , KERNEL_RESPONSE_TIME(10000000) // 10s
        , SYNC_TENDER_RESPONSE_TIME(30000000) // 15s
        // Bug - big delay on "get" request when any missing chunk
        // TicketID https://redmine.yerevak.com/issues/2759
        // giving lesser data transfer time makes get fail if getting big data
        // (e.g. ~750MB) from 2nd level peer
        // TODO- should depend from kernel load.
        //, DATA_TRANSFER_TIME(RESPONSE_TIME * 7) // 2m
        , m_tenders_timer(1000, std::min({
                                RESPONSE_TIME,
                                KERNEL_RESPONSE_TIME,
                                SYNC_TENDER_RESPONSE_TIME}) / 1000)
        , m_router_timeout(false)
        , m_expired_tender_exists(false)
        , m_timeout_mutex()
        , m_is_configured(false)
{
}

leviathan::metax::router::
~router()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:expandtab


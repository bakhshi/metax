
/**
 * @file src/metax/kernel.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::kernel
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "kernel.hpp"
#include "storage.hpp"

// Headers from other projects
#include <platform/utils.hpp>

// Headers from third party libraries
#include <Poco/JSON/Parser.h>
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
#include <Poco/String.h>

// Headers from standard libraries
#include <string>
#include <sstream>
#include <fstream>
#include <thread>

leviathan::metax::ID32
leviathan::metax::kernel::
get_request_id()
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = m_request_counter;
        ++m_request_counter;
        m_request_counter = (m_request_counter & KERNEL_REQUEST_UPPER_BOUND) |
                                        KERNEL_REQUEST_LOWER_BOUND;
        poco_assert(m_requests.end() == m_requests.find(id));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return id;
}

void leviathan::metax::kernel::
handle_key_manager_reencrypted_key_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::reencrypted_key == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
        r.aes_key = in.aes_key;
        r.aes_iv = in.aes_iv;
        send_ims_share_confirm(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_encrypted_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::encrypted == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        switch (r.cmd) {
                case metax::save: {
                        handle_save_request_after_encrypt(r);
                        break;
                } case metax::copy: {
                        handle_copy_of_encrypted_master_object(r);
                        break;
                } case metax::update: {
                        save_master_object(r, in);
                        break;
                } case metax::save_stream: {
                        handle_encrypted_save_stream(r);
                        break;
                } default: {
                        poco_assert(! (bool)"Not handled yet!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_get_aes_key()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::get_aes_key == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        switch (r.cmd) {
                case metax::update:
                case metax::save: {
                        process_save_request_after_key_generation(r);
                        break;
                } default: {
                        poco_assert(! (bool)"not handled yet!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_regenerate_user_keys_succeeded()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::regenerate_user_keys_succeeded == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        poco_assert(metax::regenerate_user_keys == r.cmd);
        send_dump_user_json_request(r.request_id, in.aes_key, in.aes_iv);
        send_dump_device_json_request(r.request_id, in.aes_key, in.aes_iv);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_keys_regeneration_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::failed == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        poco_assert(metax::regenerate_user_keys == r.cmd);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[r.request_id];
        out.cmd = metax::regenerate_user_keys_fail;
        std::string s = "Failed to regenerate user keys.";
        if (nullptr != in.message.get()) {
                s += " ";
                s += in.message.get();
        }
        out.set_payload(s);
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
encrypted_master_json_for_save(const kernel_key_manager_package& kmrx)
{
        return ! kmrx.aes_key.empty() && ! kmrx.aes_iv.empty();
}

void leviathan::metax::kernel::
handle_copy_of_encrypted_master_object(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        r.data = in;
        if (r.local) {
                post_storage_save_request(r.request_id, r.data, r.copy_uuid);
        } else {
                backup_data(r.request_id, r.copy_uuid, metax::save,
                               m_data_copy_count, r.data, r.data_version, r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_request_after_encrypt(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        poco_assert(metax::save == r.cmd);
        if (request::t_ims == r.typ) {
                save_master_object(r, *key_manager_rx);
        } else {
                handle_save_after_piece_encrypt(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_save_request_after_key_generation(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        poco_assert(metax::save == r.cmd || metax::update == r.cmd);
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::get_aes_key == in.cmd);
        poco_assert(request::t_ims == r.typ);
        poco_assert(! in.aes_key.empty() && ! in.aes_iv.empty());
        r.aes_key = in.aes_key;
        r.aes_iv = in.aes_iv;
        if (0 == r.master_json->size()) {
                start_master_object_save(r);
        } else {
                prepare_pieces_for_save(r);
                process_pending_save_requests();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_encrypted_save_stream(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        if (! in.aes_key.empty() && ! in.aes_iv.empty()) {
                r.aes_key = in.aes_key;
                r.aes_iv = in.aes_iv;
                add_shared_info_in_user_json(r, r.aes_key, r.aes_iv);
        } else {
                send_livestream_content_to_router(r, in, in.orig_size);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_livestream_content_to_router(request& r,
                platform::default_package pkg, uint32_t orig_size)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.cmd = metax::livestream_content;
        out.request_id = r.request_id;
        out = pkg;
        // sending the original size of the video chunk before encryption is
        // sent reusing data_version field as it has no meaning for livestream
        // content request
        out.data_version = orig_size;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_get_livestream_content_to_router(request& r,
                platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_router == r.typ);
        METAX_INFO("Forwarding livestream content to router: ");
        kernel_router_package& out = *router_tx;
        out.cmd = metax::get_livestream_content;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out = pkg;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_of_unencrypted_data(request& r)
{
        METAX_TRACE(__FUNCTION__);
        // In case of unencrypted update the old keys should be cleaned. This
        // is important while keys in user json are updated.
        if (metax::update == r.cmd && ! r.enc) {
                r.aes_key = r.aes_iv = "";
        }
        poco_assert(nullptr != r.master_json);
        if (0 == r.master_json->size()) {
                start_master_object_save(r);
        } else {
                prepare_pieces_for_save(r);
                process_pending_save_requests();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_after_piece_encrypt(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        auto uit = m_save_units.find(r.request_id);
        if (m_save_units.end() == uit) {
                return;
        }
        save_unit& u = uit->second;
        poco_assert(nullptr != u.main_request);
        poco_assert(u.main_request->enc && save_unit::piece == u.type);
        u.content = in;
        Poco::JSON::Object::Ptr pjo = u.main_request->master_json->
                                                getObject(u.uuid.toString());
        pjo->set("aes_iv", in.aes_iv);
        pjo->set("aes_key", in.aes_key);
        if (u.main_request->local) {
                post_storage_save_request(r.request_id, in, u.uuid);
        } else {
                backup_data(r.request_id, u.uuid, u.cmd, m_data_copy_count,
                                u.content, r.data_version, r);
        }
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
save_master_object(request& r, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        // TODO payload converts from pkg to string, then from string to pkg ???
        ID32 urid = prepare_unit_for_save(r, 0, pkg.size,
                                save_unit::master, r.uuid, pkg, ! r.local);
        if (r.local) {
                ++m_storage_input_cnt;
                post_storage_save_request(urid, pkg, r.uuid);
        } else {
                backup_data(urid, r.uuid, r.cmd, m_data_copy_count, pkg,
                                r.data_version, r);
                m_router_save_input_size += pkg.size;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::UInt64 leviathan::metax::kernel::
get_data_size_from_request_object(request& r)
{
        if (! r.file_path.empty()) {
                Poco::File f(r.file_path);
                return f.getSize();
        }
        return r.data.size;
}

Poco::JSON::Object::Ptr leviathan::metax::kernel::
generate_pieces_json(Poco::UInt64 s)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr pj = new Poco::JSON::Object();
        if (s <= max_chunk_size) {
                return pj;
        }
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        unsigned int b = 0;
        while (s > max_chunk_size) {
                UUID uuid = g.createRandom();
                pj->set(uuid.toString(), create_piece_json(b,
                                        max_chunk_size));
                b += max_chunk_size;
                s -= max_chunk_size;
        }
        if (0 < s) {
                UUID uuid = g.createRandom();
                pj->set(uuid.toString(), create_piece_json(b, s));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return pj;
}

void leviathan::metax::kernel::
prepare_pieces_for_save(request& pr)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != pr.master_json);
        auto b = pr.master_json->begin();
        for (; b != pr.master_json->end(); ++b) {
                Poco::UUID uuid(b->first);
                Poco::JSON::Object::Ptr p =
                        b->second.extract<Poco::JSON::Object::Ptr>();
                poco_assert(p->has("from") && p->has("size"));
                prepare_unit_for_save(pr, p->getValue<ID32>("from"),
                        p->getValue<ID32>("size"),
                        save_unit::piece, uuid, platform::default_package());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::ID32 leviathan::metax::kernel::
prepare_unit_for_save(request& r, unsigned int b, Poco::UInt64 s,
                              save_unit::unit_type t, const UUID& uuid,
                              platform::default_package c, bool backup)
{
        METAX_TRACE(__FUNCTION__);
        command cmd = save_unit::piece == t ? metax::save : r.cmd;
        save_unit u(&r, b, s, 0, t, uuid, c, cmd, backup);
        ID32 urid = get_request_id();
        // this map is for tracking the save process of file parts
        // (share, master, pieces)
        m_save_units.insert(std::make_pair(urid, u));
        // this set is for tracking whether all the pieces of the save
        // request are successfull with at least 1 copy
        r.units.insert(urid);
        // this set is for tracking whether all the back tenders are finished
        // after which the main request can be cleaned up
        // this set is a list of pending requests, e.g. when memory or
        // some other limit is over, the save requests will be waiting
        // in this map till there is a free resource to pick one from
        // here and proceed with.
        if (save_unit::piece == t) {
                r.backup_units.insert(urid);
                if (r.local) {
                        m_pending_save_requests.push_front(urid);
                } else {
                        m_pending_router_save_reqs.push_front(urid);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return urid;
}

void leviathan::metax::kernel::
handle_piece_save(ID32 id, platform::default_package pkg,
                                const Poco::UUID& uuid, request& pr)
{
        METAX_TRACE(__FUNCTION__);
        if (pr.enc) {
                poco_assert(! pr.aes_key.empty() && ! pr.aes_iv.empty());
                request& mr = create_request_for_unit_encrypt(id,
                                pkg, uuid, pr);
                post_key_manager_encrypt_request(mr, pkg, "", "");
        } else {
                post_storage_save_request(id, pkg, uuid);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
backup_data(ID32 id, const Poco::UUID& uuid, metax::command cmd,
                uint8_t copy_cnt, platform::default_package pkg,
                uint32_t version, const request& preq)
{
        METAX_TRACE(__FUNCTION__);
        // TODO - Fill tender requirements.
        // TODO - Change router to bakcup task
        kernel_router_package& out = *router_tx;
        out.request_id = id;
        out.cmd = cmd;
        out.uuid = uuid;
        out = pkg;
        out.data_version = version;
        out.last_updated = preq.last_updated;
        out.expire_date = preq.expire_date;
        out.max_response_cnt = copy_cnt;
        METAX_INFO("Asking router to save uuid: " + uuid.toString());
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
should_generate_new_key(request& r)
{
        if (metax::save == r.cmd) {
                return r.enc;
        }
        if (metax::update == r.cmd) {
                return r.enc && r.aes_key.empty() && r.aes_iv.empty();
        }
        return false;
}

void leviathan::metax::kernel::
process_ims_save_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == r.cmd || metax::save == r.cmd);
        Poco::UInt64 s = get_data_size_from_request_object(r);
        r.master_json = generate_pieces_json(s);
        r.backup_units.insert(r.request_id);
        if (should_generate_new_key(r)) {
                kernel_key_manager_package& out = *key_manager_tx;
                out.request_id = r.request_id;
                out.cmd = metax::gen_aes_key;
                key_manager_tx.commit();
        } else {
                handle_save_of_unencrypted_data(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::kernel::request& leviathan::metax::kernel::
create_request_for_unit_encrypt(ID32 id, platform::default_package pkg,
                                const Poco::UUID& uuid, request& pr)
{
        METAX_TRACE(__FUNCTION__);
        request& m = m_requests[id];
        m = request(id, uuid, request::requested, metax::save,
                        request::t_kernel);
        m.data = pkg;
        m.parent_req_id = pr.request_id;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return m;
}

leviathan::metax::kernel::request& leviathan::metax::kernel::
create_request_for_update_notification(const Poco::UUID& u, metax::command cmd)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == cmd || metax::del == cmd);
        ID32 id = get_request_id();
        request& m = m_requests[id];
        m = request(id, u, request::requested,
                        metax::send_to_peer, request::t_kernel);
        std::string ev = metax::update == cmd ? "updated" : "deleted";
        std::string s = R"({"event":")" + ev + R"(","uuid":")";
        s += u.toString();
        s +=R"("})";
        m.data.set_payload(s);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return m;
}

void leviathan::metax::kernel::
sort_streaming_chunks(std::vector<chunk_streaming_info>& v)
{
        METAX_TRACE(__FUNCTION__);
        using CSI = chunk_streaming_info;
        std::sort(v.begin(), v.end(),
                        [](const CSI& a, const CSI& c) {
                                return a.start < c.start;
                        }
                 );
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
update_pending_stream_requests(request& req)
{
        METAX_TRACE(__FUNCTION__);
        auto i = m_pending_ims_get_requests.find(req.uuid);
        poco_assert(m_pending_ims_get_requests.end() != i);
        for (Poco::UInt64 p : i->second) {
                request& r = m_requests[p];
                if (r.streaming_state.list.isNull()) {
                        set_streaming_state(req, r);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
set_encryption_keys_if_mls(request& req)
{
        METAX_TRACE(__FUNCTION__);
        if ("video/mls" == req.content_type) {
                req.content_type = "video/mp2t";
                poco_assert(req.master_json->has("livestream"));
                std::string lu =
                        req.master_json->getValue<std::string>("livestream");
                Poco::JSON::Object::Ptr p = get_master_info_from_user_json(lu);
                if (! p.isNull()) {
                        get_keys_from_master_info(req, p);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
construct_streaming_state_from_pieces(request& req)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr pieces = req.master_json->getObject("pieces");
        poco_assert(nullptr != pieces);
        req.streaming_state.list =
                new std::vector<chunk_streaming_info>(pieces->size());
        auto b = pieces->begin();
        auto& v = *req.streaming_state.list;
        for (Poco::UInt64 i = 0; i < pieces->size(); ++i, ++b) {
                v[i].uuid.parse(b->first);
                Poco::JSON::Object::Ptr p =
                        b->second.extract<Poco::JSON::Object::Ptr>();
                poco_assert(p->has("from") && p->has("size"));
                v[i].start = p->getValue<ID32>("from");
                v[i].size = p->getValue<ID32>("size");
        }
        sort_streaming_chunks(v);
        calculate_stream_current_chunk_from_range(req);
        process_stream_prepare(req);
        update_pending_stream_requests(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
construct_streaming_state_from_content(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json);
        poco_assert(req.master_json->has("content"));
        poco_assert(req.master_json->has("size"));
        req.streaming_state.list = new std::vector<chunk_streaming_info>(1);
        auto& v = *req.streaming_state.list;
        v[0].start = 0;
        v[0].size = req.master_json->getValue<ID32>("size");
        v[0].requested = true;
        v[0].available = true;
        std::ofstream ofs(v[0].file_path->path());
        std::string c = platform::utils::base64_decode(
                        req.master_json->getValue<std::string>("content"));
        ofs.write(c.c_str(), c.size());
        ofs.close();
        calculate_stream_current_chunk_from_range(req);
        process_stream_prepare(req);
        update_pending_stream_requests(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
construct_streaming_state(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json);
        if (req.master_json->has("content")) {
                construct_streaming_state_from_content(req);
                process_pending_get_streaming_requests(req, 0);
        } else {
                construct_streaming_state_from_pieces(req);
                post_get_request_for_streaming_pieces(req);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_stream_prepare(request& req, ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json);
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::prepare_streaming;
        out.uuid = req.uuid;
        out.content_type = req.content_type;
        out.request_id = id;
        out.chunk_count = req.streaming_state.last - req.streaming_state.curr;
        out.content_length = req.master_json->getValue<Poco::UInt64>("size");
        if (0 == req.get_range.first && (out.content_length - 1) ==
                        req.get_range.second) {
                out.get_range.first = out.get_range.second = -1;
        } else {
                out.get_range.first = req.get_range.first;
                out.get_range.second = req.get_range.second;
        }
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_stream_prepare(request& req)
{
        METAX_TRACE(__FUNCTION__);
        auto it = m_ims_req_ids.find(req.request_id);
        if (m_ims_req_ids.end() != it) {
                send_ims_stream_prepare(req, it->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_decrypted_master_node_for_get(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(is_valid_master_node(req.master_json));
        if (is_streaming_media(req)) {
                if ("video/mpl" == req.content_type) {
                        send_ims_playlist_for_recording(req);
                        return;
                }
                set_encryption_keys_if_mls(req);
                construct_streaming_state(req);
        } else {
                start_getting_pieces(req);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_playlist_for_recording(request& req)
{
        METAX_TRACE(__FUNCTION__);
        req.response_file_path =
                new Poco::TemporaryFile(platform::utils::tmp_dir);
        req.file_path = req.response_file_path->path();
        std::ofstream ofs(req.file_path, std::ios_base::binary);
        std::string d = platform::utils::to_string(
                        req.master_json->getValue<Poco::UInt32>("duration"));
        ofs << "#EXTM3U\n"
            << "#EXT-X-VERSION:3\n"
            << "#EXT-X-TARGETDURATION:" << d << '\n'
            << "#EXT-X-MEDIA-SEQUENCE:0\n"
            << "#EXTINF:" << d << ", no desc\n"
            << "/db/get?id=" + req.master_json->getValue<std::string>("recording") << '\n'
            << "#EXT-X-ENDLIST\n";
        ofs.close();
        req.content_type = "application/x-mpegurl";
        send_ims_requested_data(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_update_to_user_manager(command cmd, const Poco::UUID u,
                const std::string& iv, const std::string& key, int copy_cnt)
{
        METAX_TRACE(__FUNCTION__);
        kernel_user_manager_package& out = *user_manager_tx;
        out.cmd = cmd;
        out.uuid = u;
        out.iv = iv;
        out.key = key;
        out.copy_count = copy_cnt;
        user_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_shared_info_in_user_json(request& r, const std::string& key,
                const std::string& iv, unsigned int copy_count)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr s = new Poco::JSON::Object();
        Poco::UUID u = metax::copy == r.cmd ? r.copy_uuid : r.uuid;
        s->set("aes_key", key);
        s->set("aes_iv", iv);
        s->set("copy_count", copy_count);
        Poco::JSON::Object::Ptr fd = m_user_json->getObject("owned_data");
        poco_assert(! fd.isNull());
        // Will override if exists.
        fd->set(u.toString(), s);
        send_update_to_user_manager(metax::save, u, iv, key, copy_count);
        if (r.local) {
                add_new_uuid_in_device_json(r, u, r.data_version);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_decrypted_key(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::decrypted == in.cmd && metax::accept_share == r.cmd);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        add_shared_info_in_user_json(r, in.aes_key, in.aes_iv);
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::share_accepted;
        out.request_id = m_ims_req_ids[r.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_get_request_after_decrypt(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::decrypted == in.cmd && metax::get == r.cmd);
        r.data = in;
        if (request::t_ims == r.typ) {
                handle_unencrypted_master_object(r, in);
        } else {
                poco_assert(request::t_kernel == r.typ);
                if (m_requests.end() == m_requests.find(r.parent_req_id)) {
                        m_requests.erase(r.request_id);
                        return;
                }
                request& preq = m_requests[r.parent_req_id];
                process_unencrypted_piece(in, preq, r.uuid);
                m_requests.erase(r.request_id);
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_decryption_fail_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::decryption_fail == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        r.data = in;
        switch (r.cmd) {
                 case metax::accept_share: {
                         handle_key_decryption_fail(r);
                         break;
                 } case metax::update: {
                         send_ims_update_fail_response(r, in.message.get());
                         break;
                 } case metax::copy: {
                         send_ims_copy_failed_response(r, in.message.get());
                         break;
                 } case metax::get: {
                         handle_key_manager_failed_get_request(r);
                         break;
                 } case metax::del: {
                         handle_decryption_fail_for_delete_request(r);
                         break;
                 } default: {
                         std::string s = "Decryption fail request for ";
                         s += platform::utils::to_string(r.cmd);
                         METAX_WARNING(s + " is not handled yet !");
                 }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_decrypted_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        auto ri = m_requests.find(in.request_id);
        poco_assert(m_requests.end() != ri);
        request& r = ri->second;
        switch (r.cmd) {
                case metax::get: {
                        handle_get_request_after_decrypt(r);
                        break;
                } case metax::del:
                  case metax::copy:
                  case metax::update: {
                        handle_unencrypted_master_object(r, in);
                        break;
                } case metax::accept_share: {
                        handle_decrypted_key(r);
                        break;
                } default: {
                        METAX_WARNING(
                               "This command of request is unexpected.");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
is_valid_user_json()
{
        return (nullptr != m_user_json && m_user_json->has("protocol_version")
                                   && m_user_json->has("owned_data"));
}

void leviathan::metax::kernel::
process_unencrypted_piece(const platform::default_package pkg,
                                request& preq, const Poco::UUID& uuid)
{
        poco_assert(metax::get == preq.cmd);
        METAX_TRACE(__FUNCTION__);
        if (is_streaming_media(preq)) {
                handle_streaming_chunk(pkg, preq, uuid);
        } else {
                write_data_to_stream(pkg, preq, uuid);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_id_found_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        switch (r.cmd) {
                case metax::find_peer: {
                        post_router_peer_found(r);
                        break;
               } case metax::notify_update: {
                        send_update_notification_to_ims(r, "updated");
                        break;
               } case metax::notify_delete: {
                        send_update_notification_to_ims(r, "deleted");
                        break;
               } default: {
                       METAX_WARNING("Unexpected request type for id_found response");
               }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_peer_found(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.cmd = metax::peer_found;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_update_notification_to_ims(request& r, const std::string& e)
{
        METAX_TRACE(__FUNCTION__);
        if (r.uuid == m_user_json_uuid) {
                kernel_user_manager_package& out = *user_manager_tx;
                out.cmd = metax::update;
                out.data_version = r.data_version;
                user_manager_tx.commit();
                m_requests.erase(r.request_id);
        } else {
                std::string s = "{\"event\":\"" + e + "\",\"uuid\":\""
                        + r.uuid.toString() + "\"}";
                send_notification_to_ims(r.request_id, s);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_id_not_found_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        METAX_ERROR("ID NOT FOUND");
        if (metax::notify_update != r.cmd &&
                        metax::notify_delete != r.cmd) {
                kernel_router_package& out = *router_tx;
                out = r.data;
                out.request_id = in.request_id;
                out.uuid = r.uuid;
                out.cmd = metax::refuse;
                router_tx.commit();
        }
        m_requests.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_decryption_fail(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        poco_assert(request::t_ims == req.typ && metax::accept_share == req.cmd);
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::accept_share_fail;
        out.request_id = m_ims_req_ids[req.request_id];
        std::string s = "Could not decrypt key: ";
        out.set_payload(s + req.data.message.get());
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_decryption_fail_for_delete_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ims_kernel_package& out = (*ims_tx);
        out.uuid = req.uuid;
        std::string s = "Failed to delete. ";
        s += "Could not decrypt " + req.uuid.toString() + " uuid.";
        out.set_payload(s);
        out.cmd = metax::delete_fail;
        out.request_id = m_ims_req_ids[req.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_failed_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_requests.end() != m_requests.find(req.request_id));
        poco_assert(request::t_kernel == req.typ || request::t_ims == req.typ);
        ID32 i = req.request_id;
        if (request::t_kernel == req.typ) {
                i = req.parent_req_id;
                m_requests.erase(req.request_id);
        }
        if (m_requests.end() != m_requests.find(i)) {
                ims_kernel_package pkg;
                pkg.uuid = m_requests[i].uuid;
                std::string s = "Getting file failed: uuid="
                        + pkg.uuid.toString() + ". ";
                pkg.set_payload(s + req.data.message.get());
                pkg.cmd = metax::get_fail;
                send_ims_response(pkg);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_failed_share_request()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& req = m_requests[in.request_id];
        poco_assert(request::t_ims == req.typ);
        ID32 i = req.request_id;
        auto ii = m_ims_req_ids.find(i);
        if (m_requests.end() != m_requests.find(i) &&
                m_ims_req_ids.end() != ii) {
                poco_assert(key_manager_rx.has_data());
                (*ims_tx) = *key_manager_rx;
                (*ims_tx).cmd = metax::share_fail;
                (*ims_tx).uuid = req.uuid;
                (*ims_tx).request_id = ii->second;
                ims_tx.commit();
                m_ims_req_ids.erase(i);
                m_requests.erase(i);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_input()
{
        if (! key_manager_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_key_manager_input();
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
        key_manager_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_key_manager_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(key_manager_rx.has_data());
        const kernel_key_manager_package& in = *key_manager_rx;
        if (m_requests.end() == m_requests.find(in.request_id)
                        && (metax::request_keys_response != in.cmd
                                && metax::key_init_fail != in.cmd
                                && metax::request_keys_fail != in.cmd
                                && metax::get_user_keys_response != in.cmd
                                && metax::get_user_keys_failed != in.cmd)) {
                return;
        }
        switch (in.cmd) {
                case metax::encrypted: {
                        handle_key_manager_encrypted_request();
                        break;
                } case metax::decrypted: {
                        handle_key_manager_decrypted_request();
                        break;
                } case metax::reencrypted_key: {
                        handle_key_manager_reencrypted_key_request();
                        break;
                } case metax::decryption_fail: {
                        handle_key_manager_decryption_fail_request();
                        break;
                } case metax::reencryption_fail: {
                        handle_key_manager_failed_share_request();
                        break;
                } case metax::id_found: {
                        handle_key_manager_id_found_request();
                        break;
                } case metax::id_not_found: {
                        handle_key_manager_id_not_found_request();
                        break;
                } case metax::livestream_decrypted: {
                        send_livestream_content_to_ims(in.uuid, in);
                        break;
                } case metax::stream_decrypt_fail: {
                        handle_key_manager_stream_decrypt_fail();
                        break;
                } case metax::get_aes_key: {
                        handle_key_manager_get_aes_key();
                        break;
                } case metax::regenerate_user_keys_succeeded: {
                        handle_regenerate_user_keys_succeeded();
                        break;
                } case metax::failed: {
                        handle_user_keys_regeneration_fail();
                        break;
                } case metax::request_keys_response: {
                        handle_key_manager_request_keys_response(in);
                        break;
                } case metax::request_keys_fail: {
                        handle_request_keys_fail(in.request_id, in);
                        break;
                } case metax::key_init_fail: {
                        handle_key_init_fail();
                        break;
                } case metax::get_user_keys_failed:
                  case metax::get_user_keys_response: {
                        handle_key_manager_get_user_keys_response(in);
                        break;
                } default: {
                        poco_assert(! (bool)"Command not handled yet !!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_stream_decrypt_fail()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_key_manager_package& in = *key_manager_rx;
        poco_assert(metax::stream_decrypt_fail == in.cmd);
        send_get_livestream_fail_to_ims(in.uuid, in);
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        out.cmd = metax::cancel_livestream_get;
        router_tx.commit();
        post_router_clear_request(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


bool leviathan::metax::kernel::
is_authorized_request(ID32 req_id) const
{
        METAX_TRACE(__FUNCTION__);
        (void) req_id;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return true;
}

void leviathan::metax::kernel::
post_storage_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        //poco_assert(request::authentication == req.st);
        req.st = request::sending;
        (*storage_tx).uuid = req.uuid;
        (*storage_tx).cmd = metax::get;
        (*storage_tx).request_id = req.request_id;
        storage_tx.commit();
        METAX_INFO("Requesting content of - uuid: " +
                                                        req.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_ims_no_permissions(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::authentication == req.st);
        ims_kernel_package pkg;
        pkg.uuid = req.uuid;
        pkg.cmd = metax::no_permissions;
        send_ims_response(pkg);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_delete_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        remove_unnecessary_uuids(r);
        if (r.local) {
                post_storage_delete_request(r);
        } else {
                post_router_delete_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_start_sync()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_storage_package& in = *storage_rx;
        poco_assert(metax::start_storage_sync == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        poco_assert(nullptr != in.message.get());
        std::string msg = "Start sync process for uuids:\n";
        msg += in.message.get();
        std::replace(msg.begin(), msg.end(), ',', '\n');
        METAX_DEBUG(msg);
        Poco::JSON::Object::Ptr obj = new Poco::JSON::Object();
        obj->set("uuids", in.message.get());
        std::stringstream ss;
        obj->stringify(ss);
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        out.cmd = metax::send_journal_info;
        out.set_payload(ss.str());
        out.last_updated = r.last_updated;
        router_tx.commit();
        m_requests.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_backup_get_uuid(const kernel_backup_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get_uuid == in.cmd);
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        out.uuid = in.uuid;
        out.cmd = in.cmd;
        out = in;
        out.data_version = in.data_version;
        out.last_updated = in.last_updated;
        out.expire_date = in.expire_date;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_copy_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        poco_assert(request::t_kernel == r.typ);
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out.cmd = r.cmd;
        out.data_version = r.data_version;
        out.last_updated = r.last_updated;
        out.expire_date = r.expire_date;
        out.set_payload(r.copy_uuid.toString());
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_copy_of_each_piece(request& r, const std::string& old_uuid,
                          const std::string& new_uuid)
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = get_request_id();
        Poco::UUID u(old_uuid);
        request& c = m_requests[id];
        c = request(id, u, request::requested, metax::copy, request::t_kernel,
                        r.data_version, r.last_updated, r.expire_date);
        c.parent_req_id = r.request_id;
        c.copy_uuid.parse(new_uuid);
        c.data.set_payload(new_uuid);
        r.units.insert(id);
        r.backup_units.insert(id);
        post_storage_copy_request(c, c.data);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
generate_copy_request_for_pieces(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        poco_assert(nullptr != r.master_json);
        poco_assert(nullptr != r.old_master_json);
        Poco::JSON::Object::Ptr nm = r.master_json->getObject("pieces");
        poco_assert(nullptr != nm);
        Poco::JSON::Object::Ptr p = r.old_master_json->getObject("pieces");
        poco_assert(nullptr != p);
        if (0 == p->size()) {
                return handle_copy_of_master_object(r);
        }
        std::vector<std::string> vec;
        p->getNames(vec);
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        for (unsigned int i = 0; i < vec.size(); ++i) {
                std::string nu = g.createRandom().toString();
                handle_copy_of_each_piece(r, vec[i], nu);
                nm->set(nu, p->getObject(vec[i]));
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_copy_of_master_object(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        poco_assert(nullptr != r.master_json);
        platform::default_package pkg;
        pkg.set_payload(r.master_json);
        if (r.aes_key.empty() && r.aes_iv.empty()) {
                r.data = pkg;
                if (r.local) {
                        post_storage_save_request(r.request_id, r.data,
                                        r.copy_uuid);
                } else {
                        backup_data(r.request_id, r.copy_uuid, metax::save,
                               m_data_copy_count, r.data, r.data_version, r);
                }
        } else {
                post_key_manager_encrypt_request(r, pkg, r.aes_key, r.aes_iv);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_copy_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        poco_assert(nullptr != r.old_master_json);
        // Genetate new master json for copy request;
        Poco::Dynamic::Var v1 = r.old_master_json->get("content_type");
        Poco::UInt64 s = r.old_master_json->getValue<Poco::UInt64>("size");
        r.master_json = create_master_json_template(r, s, v1.toString());
        poco_assert(nullptr != r.master_json);
        if (r.old_master_json->has("content")) {
                r.master_json->set("content",
                           r.old_master_json->getValue<std::string>("content"));
        }
        r.backup_units.insert(r.request_id);
        // Process copy of pieces
        generate_copy_request_for_pieces(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
authenticate_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        poco_assert(! in.message.isNull());
        poco_assert(metax::get == r.cmd || metax::save == r.cmd
                 || metax::del == r.cmd || metax::update == r.cmd
                 || metax::share == r.cmd || metax::copy == r.cmd);
        /// Request from Storage authentication metadata and authenticate
        r.st = request::authentication;
        /// In case of positive answer on node existance in DB, along with
        //positive answer node metadata can be returned which will contain
        //data for authentication. For external requests data for authentication
        //can be stored in request.data member for the further processing.
        //For internal requests authentication can be done in-place
        if (metax::get == r.cmd) {
                r.data = in;
        }
        if (is_authorized_request(r.request_id)) {
                post_storage_get_request(r);
        } else {
                post_ims_no_permissions(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_update_listener(request& r, const std::string& p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::on_update_register == r.cmd);
        poco_assert(storage_rx.has_data());
        Poco::JSON::Array::Ptr parr =
                m_update_listeners->getArray(r.uuid.toString());
        if (parr.isNull()) {
                parr = new Poco::JSON::Array;
                m_update_listeners->set(r.uuid.toString(), parr);
        }
        METAX_INFO("Adding update listener for uuid: " + r.uuid.toString());
        auto f = find_if(parr->begin(), parr->end(), [&p](Poco::Dynamic::Var v) {
                        return p == v.extract<std::string>();
                        });
        if (parr->end() == f) {
                parr->add(p);
        }
        //serialize_update_listeners();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
remove_update_listener(const std::string& u, const std::string& p)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Array::Ptr parr = m_update_listeners->getArray(u);
        if (! parr.isNull()) {
                auto f = find_if(parr->begin(), parr->end(),
                                [&p](Poco::Dynamic::Var v) {
                                        return p == v.extract<std::string>();
                                });
                if (parr->end() != f) {
                        int i = std::distance(parr->begin(), f);
                        parr->remove(i);
                        if (0 == parr->size()) {
                                m_update_listeners->remove(u);
                        }
                        return true;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return false;
}

void leviathan::metax::kernel::
add_update_listener_for_local_request(request& r)
{
        add_update_listener(r, "");
        send_ims_on_update_register_response(r);
}

void leviathan::metax::kernel::
add_update_listener_for_remote_request(request& r)
{
        add_update_listener(r,
                        std::string(r.data.message.get(), r.data.size));
        post_router_on_update_register_response(r);
}

void leviathan::metax::kernel::
serialize_update_listeners()
{
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        std::ofstream ofs(m_update_listeners_path);
        if (ofs.is_open()) {
                m_update_listeners->stringify(ofs, 2);
        } else {
                METAX_WARNING("Unable to serialize on update listeners");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_on_update_register_response(request& r)
{
        METAX_TRACE(__FUNCTION__);
        (*ims_tx).cmd = metax::on_update_registered;
        (*ims_tx).uuid = r.uuid;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        (*ims_tx).request_id = m_ims_req_ids[r.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_on_update_register_response(request& r)
{
        METAX_TRACE(__FUNCTION__);
        (*router_tx).cmd = metax::on_update_registered;
        (*router_tx).uuid = r.uuid;
        (*router_tx).request_id = r.request_id;
        router_tx.commit();
        post_router_clear_request(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_manager_on_update_register(const Poco::UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, uuid, request::requested, metax::on_update_register,
                        request::t_user_manager);
        post_config_get_public_key_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_on_update_unregister_response(request& r)
{
        METAX_TRACE(__FUNCTION__);
        (*ims_tx).cmd = metax::on_update_unregistered;
        (*ims_tx).uuid = r.uuid;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        (*ims_tx).request_id = m_ims_req_ids[r.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_authentication_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        if (request::lookup_in_db != r.st) {
                r.st = request::authentication;
                r.data = (*storage_rx);
                kernel_router_package& out = *router_tx;
                out.uuid = r.uuid;
                out.request_id = r.request_id;
                out.cmd = metax::auth;
                // When authentication logic is defined also send message via
                //out.message member.
                router_tx.commit();
        }
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_save_offer_request(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        m_requests[id].st = request::save_accept;
        kernel_router_package& out = *router_tx;
        out.uuid = m_requests[id].uuid;
        out.request_id = id;
        out.cmd = metax::save_offer;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_save_data_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *storage_writer_tx;
        METAX_INFO("Asking storage to save uuid: " + r.uuid.toString());
        out.request_id = r.request_id;
        out.cmd = metax::save_data;
        out.uuid = r.uuid;
        out = r.data;
        storage_writer_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_save_data_request(request& r, const Poco::UUID& u)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *storage_writer_tx;
        METAX_INFO("Asking storage to save uuid: " + u.toString());
        out.request_id = r.request_id;
        out.cmd = metax::save_data;
        out.uuid = u;
        out = r.data;
        storage_writer_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_save_request(ID32 i, platform::default_package pkg,
                                                const Poco::UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *storage_writer_tx;
        METAX_INFO("Asking storage to save uuid: " + uuid.toString());
        out.request_id = i;
        out.cmd = metax::save_data;
        out.uuid = uuid;
        out = pkg;
        out.should_send_response = true;
        storage_writer_tx.commit();
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_save_request(platform::default_package pkg, const Poco::UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *storage_writer_tx;
        METAX_INFO("Asking storage to save uuid: " + uuid.toString());
        out.cmd = metax::save_data;
        out.uuid = uuid;
        out = pkg;
        out.should_send_response = false;
        storage_writer_tx.commit();
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_save_request(platform::default_package pkg,
                                                const std::string& uuid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *storage_writer_tx;
        METAX_INFO("Asking storage to save uuid: " + uuid);
        out.cmd = metax::save_data;
        out.uuid = Poco::UUID(uuid);
        out = pkg;
        out.should_send_response = false;
        storage_writer_tx.commit();
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_delete_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        (*storage_tx).request_id = r.request_id;
        (*storage_tx).cmd = metax::del;
        (*storage_tx).uuid = r.uuid;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_positive_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        poco_assert(metax::ack == in.cmd);
        request& r = m_requests[in.request_id];
        if (request::t_ims == r.typ || request::t_kernel == r.typ) {
                handle_storage_positive_response_for_local_request(r);
        } else if (request::t_router == r.typ) {
                handle_storage_positive_response_for_remote_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_positive_response_for_local_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        switch (r.cmd) {
                case metax::get:
                case metax::save:
                case metax::update:
                case metax::copy:
                case metax::share: {
                        authenticate_request(r);
                        break;
                } case metax::on_update_register: {
                        add_update_listener_for_local_request(r);
                        break;
                } case metax::del: {
                        if (r.keep_chunks) {
                                post_storage_delete_request(r);
                        } else {
                                authenticate_request(r);
                        }
                        break;
                } default: {
                        METAX_WARNING("This case is not handled yet.");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_positive_response_for_remote_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        switch (r.cmd) {
                case metax::get: {
                        post_storage_get_request(r);
                        break;
                } case metax::update: {
                        post_storage_save_data_request(r);
                        //post_router_authentication_request(r);
                        break;
                } case metax::del: {
                        post_storage_delete_request(r);
                        break;
                } case metax::copy: {
                        post_storage_copy_request(r, r.data);
                        break;
                } case metax::on_update_register: {
                        add_update_listener_for_remote_request(r);
                        break;
                } default: {
                        METAX_WARNING("This case is not handled yet.");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::lookup_in_db == req.st);
        METAX_DEBUG("Request id: " +
                        platform::utils::to_string(req.request_id));
        METAX_INFO("UUID not found, asking router: "
                        + req.uuid.toString());
        req.st = request::requested;
        (*router_tx).request_id = req.request_id;
        (*router_tx).cmd = metax::get;
        (*router_tx).uuid = req.uuid;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_refuse(request& r)
{
        METAX_TRACE(__FUNCTION__);
        if (request::lookup_in_db == r.st) {
                kernel_router_package& out = *router_tx;
                out.request_id = r.request_id;
                out.cmd = metax::refuse;
                out.uuid = r.uuid;
                out = r.data;
                router_tx.commit();
        }
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response_for_get_req(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get == req.cmd);
        if (request::t_ims == req.typ) {
                post_router_get_request(req);
        } else {
                poco_assert(request::t_kernel == req.typ);
                m_pending_router_get_reqs.push_front(req.request_id);
                process_router_pending_get_requests();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response_for_copy_req(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == req.cmd);
        req.local = false;
        if (request::t_ims == req.typ) {
                // Create new request object for getting master object.
                ID32 tmp_id = get_request_id();
                request& tmp_req = m_requests[tmp_id];
                tmp_req = request(tmp_id, req.uuid, request::lookup_in_db,
                                metax::copy, request::t_ims);
                tmp_req.parent_req_id = req.request_id;
                post_router_get_request(tmp_req);
        } else {
                poco_assert(request::t_kernel == req.typ);
                post_router_copy_request(req);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response_for_update_req(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == req.cmd);
        poco_assert(request::t_ims == req.typ);
        req.local = false;
        // Create new request object for getting master object.
        ID32 tmp_id = get_request_id();
        request& tmp_req = m_requests[tmp_id];
        tmp_req = request(tmp_id, req.uuid, request::lookup_in_db,
                        metax::update, request::t_ims);
        tmp_req.parent_req_id = req.request_id;
        post_router_get_request(tmp_req);
        METAX_TRACE(std::string("END") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_nack_for_on_update_register_req(request& req)
{
        METAX_TRACE(__FUNCTION__);
        post_config_get_public_key_request(req);
        METAX_TRACE(std::string("END") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_config_get_public_key_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        kernel_cfg_package& out = *config_tx;
        out.request_id = req.request_id;
        out.cmd = metax::get_public_key;
        config_tx.commit();
        METAX_TRACE(std::string("END") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response_for_delete_req(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::del == r.cmd);
        if (request::t_ims == r.typ && nullptr == r.old_master_json) {
                // Create new request object for getting master object.
                ID32 tmp_id = get_request_id();
                request& tmp_req = m_requests[tmp_id];
                tmp_req = request(tmp_id, r.uuid, request::lookup_in_db,
                                metax::del, request::t_ims);
                tmp_req.parent_req_id = r.request_id;
                tmp_req.local = false;
                post_router_get_request(tmp_req);
        } else if (request::t_router != r.typ) {
                post_router_delete_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response_for_ims_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ || request::t_kernel == r.typ);
        switch (r.cmd) {
                case metax::get: {
                        handle_storage_negative_response_for_get_req(r);
                        break;
                } case metax::copy: {
                        handle_storage_negative_response_for_copy_req(r);
                        break;
                } case metax::update: {
                        handle_storage_negative_response_for_update_req(r);
                        break;
                } case metax::on_update_register: {
                        handle_storage_nack_for_on_update_register_req(r);
                        break;
                } case metax::save: {
                        poco_assert(! (bool)"Storage negative response for"
                                        " save request is not handled yet!");
                        break;
                } case metax::del: {
                        handle_storage_negative_response_for_delete_req(r);
                        break;
                } default: {
                        poco_assert(! (bool)"Not handled yet!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_negative_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        poco_assert(metax::nack == in.cmd);
        ID32 id = in.request_id;
        request& r = m_requests[id];
        if (request::t_ims == r.typ || request::t_kernel == r.typ) {
                handle_storage_negative_response_for_ims_request(r);
        }
        if (request::t_router == r.typ) {
                // if request is external post refuse for request
                if (/*metax::get == r.cmd
                                ||*/ metax::on_update_register == r.cmd) {
                        METAX_INFO("UUID not found, inform router: " + r.uuid.toString());
                        post_router_refuse(r);
                } else if (metax::save == r.cmd) {
                        post_router_save_offer_request(id);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_clear_request(ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = req_id;
        out.cmd = metax::clear_request;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_get_journal_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(backup_rx.has_data());
        const kernel_backup_package& in = *backup_rx;
        poco_assert(metax::get_journal_info == in.cmd);
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        out.cmd = in.cmd;
        out = in;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_backup_send_uuids(const kernel_backup_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::send_uuids == in.cmd);
        poco_assert(nullptr != in.sync_uuids);
        auto i = m_requests.find(in.request_id);
        if (m_requests.end() == i) {
                return;
        }
        i->second.master_json = in.sync_uuids;
        post_router_send_uuids_request(i->second);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_router_requested_data(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::sending == req.st);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        kernel_router_package& out = *router_tx;
        out.uuid = in.uuid;
        out.request_id = req.request_id;
        out.cmd = metax::get_data;
        out = *storage_rx;
        router_tx.commit();
        post_router_clear_request(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
remove_id_from_parent_units(request& preq, ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        preq.units.erase(req_id);
        if (preq.units.empty()) {
                handle_copy_of_master_object(preq);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_copy_succeeded_response(request& preq)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(preq.request_id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[preq.request_id];
        out.cmd = metax::copy_succeeded;
        out.uuid = preq.uuid;
        out.set_payload(preq.copy_uuid.toString());
        ims_tx.commit();
        m_ims_req_ids.erase(preq.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_first_copy_success_for_master_object(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(1 == r.updated);
        send_ims_copy_succeeded_response(r);
        add_shared_info_in_user_json(r, r.aes_key, r.aes_iv);
        if (1 == m_data_copy_count) {
                if (! r.local) {
                        post_router_clear_request(r.request_id);
                }
                m_requests.erase(r.request_id);
        } else if (r.local) {
                backup_data(r.request_id, r.copy_uuid, metax::save,
                                m_data_copy_count - 1, r.data,
                                r.data_version, r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_copy_success_for_master_object(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ);
        poco_assert(metax::copy == r.cmd);
        ++r.updated;
        METAX_INFO("Received copy success (old uuid = " + r.uuid.toString()
                        + ", new uuid = " + r.copy_uuid.toString()
                        + ", copy count - "
                        + platform::utils::to_string(r.updated) + ").");
        if (1 == r.updated) {
                handle_first_copy_success_for_master_object(r);
        } else {
                update_copy_count_of_owned_uuid_in_user_json(r.copy_uuid,
                                r.updated);
                if (m_data_copy_count == r.updated) {
                        remove_from_backup_units(r, r.request_id);
                        post_router_clear_request(r.request_id);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_first_copy_success_for_piece(request& r, request& preq)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(1 == r.updated);
        preq.saved_units.insert(r.copy_uuid);
        add_new_piece_of_copy_req_in_user_json(r);
        remove_id_from_parent_units(preq, r.request_id);
        if (1 == m_data_copy_count) {
                m_requests.erase(r.request_id);
        } else if (r.local) {
                post_router_copy_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_copy_success_for_piece(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == r.typ);
        poco_assert(metax::copy == r.cmd);
        ++r.updated;
        METAX_INFO("Received copy success (old uuid = " + r.uuid.toString()
                        + ", new uuid = " + r.copy_uuid.toString()
                        + ", copy count - "
                        + platform::utils::to_string(r.updated) + ").");
        poco_assert(m_requests.end() != m_requests.find(r.parent_req_id));
        request& preq = m_requests[r.parent_req_id];
        if (1 == r.updated) {
                handle_first_copy_success_for_piece(r, preq);
        } else {
                update_copy_count_of_owned_uuid_in_user_json(r.copy_uuid,
                                r.updated);
                if (m_data_copy_count == r.updated) {
                        remove_from_backup_units(preq, r.request_id);
                        post_router_clear_request(r.request_id);
                        m_requests.erase(r.request_id);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_copy_success_for_remote_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        poco_assert(request::t_router == req.typ);
        poco_assert(metax::copy == req.cmd);
        poco_assert(nullptr != req.data.message.get());
        kernel_router_package& out = *router_tx;
        out.uuid = req.uuid;
        out.request_id = req.request_id;
        out.cmd = metax::copy_succeeded;
        router_tx.commit();
        Poco::UUID uuid(req.data.message.get());
        add_new_uuid_in_device_json(req, uuid, req.data_version);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_send_uuids_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != r.master_json);
        if (0 == r.master_json->size()) {
                post_router_sync_finished(r.request_id);
        } else {
                m_should_block_get_request = true;
                kernel_router_package& out = *router_tx;
                out.request_id = r.request_id;
                std::vector<std::string> k;
                r.master_json->getNames(k);
                std::string s = Poco::cat(std::string("\n"),
                                k.begin(), k.end());
                METAX_DEBUG("Requested uuids for sync:\n" + s);
                out.cmd = metax::send_uuids;
                out.set_payload(Poco::cat(std::string(","),
                                        k.begin(), k.end()));
                router_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_send_uuids_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = (*router_rx);
        poco_assert(metax::send_uuids == in.cmd);
        poco_assert(nullptr != in.message.get());
        std::string s = "Processing uuids for uuids=";
        METAX_DEBUG(s + in.message.get());
        kernel_storage_package& out = *storage_tx;
        out.request_id = in.request_id;
        out.cmd = in.cmd;
        out = in;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_get_uuid()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = (*router_rx);
        poco_assert(metax::get_uuid == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                METAX_WARNING("Received " + in.uuid.toString() +
                                " uuid content for removed sync request");
                return;
        }
        request& r = m_requests[in.request_id];
        poco_assert(metax::get_journal_info == r.cmd);
        METAX_INFO("Received " + in.uuid.toString() + " uuid in sync process");
        //post_storage_remove_from_cache_request(in.uuid);
        post_storage_save_request(in, in.uuid);
        if (r.uuid == m_user_json_uuid) {
                handle_router_get_data_for_user_manager_request(in);
        }
        notify_update(in.uuid, metax::update, in.data_version);
        process_requested_uuid_for_sync(r, in);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_requested_uuid_for_sync(request& req, const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json);
        add_new_uuid_in_device_json(in);
        poco_assert(req.master_json->has(in.uuid.toString()));
        req.master_json->remove(in.uuid.toString());
        if (0 == req.master_json->size()) {
                post_router_sync_finished(req.request_id);
                m_requests.erase(req.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_start_storage_sync()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::start_storage_sync == in.cmd);
        poco_assert(m_requests.end() == m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        r = request(in.request_id, Poco::UUID::null(), request::max_state,
                        in.cmd, request::t_kernel);
        r.last_updated = in.last_updated;
        kernel_storage_package& out = *storage_tx;
        out.request_id = in.request_id;
        out.cmd = metax::start_storage_sync;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_sync_finished(ID32 rid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = rid;
        out.cmd = metax::sync_finished;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
update_info_for_deleted_uuid(const std::string& uuid,
                const std::string last_updated,
                const std::string dversion)
{
        METAX_TRACE(__FUNCTION__);
        using U = platform::utils;
        uint32_t v = U::from_string<uint32_t>(dversion);
        Poco::UInt64 lu = U::from_string<Poco::UInt64>(last_updated);
        mark_uuid_as_deleted_in_device_json(Poco::UUID(uuid), v, lu);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_journal_info_for_sync(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        request& r = m_requests[in.request_id];
        r = request(in.request_id, Poco::UUID::null(), request::max_state,
                        metax::get_journal_info, request::t_kernel);
        kernel_backup_package& out = *backup_tx;
        out.cmd = metax::get_journal_info;
        out.request_id = in.request_id;
        out.sync_class = m_sync_class;
        out = in;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_get_journal_info(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        if (nullptr == in.message.get()) {
                post_router_sync_finished(in.request_id);
        } else {
                process_journal_info_for_sync(in);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_send_journal_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = (*router_rx);
        poco_assert(metax::send_journal_info == in.cmd);
        kernel_backup_package& out = *backup_tx;
        out.cmd = metax::send_journal_info;
        out.request_id = in.request_id;
        out.time = in.last_updated;
        out = in;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_copy_succeeded(ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        if (m_requests.end() == m_requests.find(req_id)) {
                return;
        }
        request& r = m_requests[req_id];
        poco_assert(metax::copy == r.cmd);
        poco_assert(request::t_kernel == r.typ);
        handle_copy_success_for_piece(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_failed(const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        auto j = m_save_units.find(in.request_id);
        if (m_save_units.end() != j) {
                poco_assert(nullptr != in.message.get());
                return handle_canceled_save_unit(j->second, in.request_id,
                                in.message.get());
        }
        auto i = m_requests.find(in.request_id);
        if (m_requests.end() != i) {
                handle_failed_request(i->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_copy_succeeded(ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        if (m_requests.end() == m_requests.find(req_id)) {
                return;
        }
        request& r = m_requests[req_id];
        poco_assert(metax::copy == r.cmd);
        if (request::t_kernel == r.typ) {
                poco_assert(0 == r.updated);
                handle_copy_success_for_piece(r);
        } else {
                poco_assert(request::t_router == r.typ);
                handle_copy_success_for_remote_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_no_space_copy()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        poco_assert(metax::no_space == in.cmd);
        if (m_requests.end() != m_requests.find(in.request_id)) {
                request& r = m_requests[in.request_id];
                r.local = false;
                post_router_copy_request(r);
        } else {
                std::string w = "Request by request id - ";
                w += platform::utils::to_string(in.request_id);
                METAX_WARNING(w + " is not found");
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_get_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        METAX_INFO("Processing file got from storage - uuid: "
                                                        + r.uuid.toString());
        switch (r.typ) {
                case request::t_ims: {
                        got_master_object(in, r);
                        break;
                } case request::t_kernel: {
                        handle_piece(in, r);
                        break;
                } case request::t_router: {
                        send_router_requested_data(r);
                        break;
                } default: {
                        METAX_WARNING("Unexpected request type");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_input()
{
        if (! storage_rx.has_data()) {
                return;
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

void leviathan::metax::kernel::
handle_storage_writer_input()
{
        if (! storage_writer_rx.has_data()) {
                return;
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

void leviathan::metax::kernel::
handle_cache_input()
{
        if (! cache_rx.has_data()) {
                return;
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

void leviathan::metax::kernel::
process_storage_writer_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        METAX_INFO("Processing storage input for " + in.uuid.toString());
        switch (in.cmd) {
                case metax::save_confirm: {
                        handle_storage_save_confirm();
                        break;
                } case metax::failed: {
                        handle_storage_failed(*storage_writer_rx);
                        break;
                } case metax::no_space: {
                        handle_storage_no_space();
                        break;
                } default: {
                        METAX_WARNING("This command from storage is unknown.");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_cache_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(cache_rx.has_data());
        const kernel_storage_package& in = *cache_rx;
        METAX_INFO("Processing storage input for " + in.uuid.toString());
        switch (in.cmd) {
                case metax::move_cache_to_storage_completed: {
                        handle_move_cache_to_storage_completed();
                        break;
                } default: {
                        METAX_WARNING("This command from storage is unknown.");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_storage_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        METAX_INFO("Processing storage input for " + in.uuid.toString());
        switch (in.cmd) {
                case metax::ack: {
                        handle_storage_positive_response();
                        break;
                } case metax::nack: {
                        handle_storage_negative_response();
                        break;
                } case metax::get_data: {
                        handle_storage_get_response();
                        break;
                } case metax::deleted: {
                        handle_storage_deleted();
                        break;
                //} case metax::move_cache_to_storage_completed: {
                //        handle_move_cache_to_storage_completed();
                //        break;
                } case metax::copy_succeeded: {
                        handle_storage_copy_succeeded(in.request_id);
                        break;
                } case metax::failed: {
                        handle_storage_failed(*storage_rx);
                        break;
                } case metax::no_space: {
                        handle_storage_no_space_copy();
                        break;
                } case metax::start_storage_sync: {
                        handle_storage_start_sync();
                        break;
                } default: {
                        METAX_WARNING("This command from storage is unknown.");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_wallet_input()
{
        if (! wallet_rx.has_data()) {
                return;
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

void leviathan::metax::kernel::
process_wallet_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(wallet_rx.has_data());
        std::string s((*wallet_rx).message);
        METAX_INFO( + " Got message from wallet: " + s);
        (*wallet_tx).set_payload(std::string(name() + " end of the connection "));
        wallet_tx.commit();
        wallet_tx.deactivate();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_look_up_request(const request& r, bool check_cache)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = (*storage_tx);
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out.check_cache = check_cache;
        out.cmd = metax::look_up;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_copy_request(request& req, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = (*storage_tx);
        out.request_id = req.request_id;
        out.uuid = req.uuid;
        out.cmd = req.cmd;
        out = pkg;
        storage_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_copy_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        METAX_DEBUG("Received copy from router for uuid=" + in.uuid.toString());
        request& r = m_requests[in.request_id];
        r = request(in.request_id, in.uuid, request::lookup_in_db,
                        in.cmd, request::t_router, in.data_version,
                        in.last_updated, in.expire_date);
        r.data = in;
        post_storage_look_up_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::kernel::
handle_router_get_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        ID32 id = in.request_id;
        METAX_DEBUG("Received get for uuid=" + in.uuid.toString());
        // push request info into map
        // if there is a request already posted to storage for  uuid
        // there is no need to do it second time
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() != li) {
                request& r = m_requests[id];
                r = request(id, in.uuid, request::lookup_in_db,
                                in.cmd, request::t_router);
                post_router_stream_auth_request(r);
                li->second->listeners.push_back(r.request_id);
        } else {
                std::thread t(&storage::get_data_from_db_for_router_async,
                                m_get_storage, id, in.uuid, true);
                t.detach();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_stream_auth_request(const request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = (*router_tx);
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out.cmd = metax::stream_auth;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::JSON::Object::Ptr
leviathan::metax::kernel::
parse_json_data(platform::default_package pkg) const
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr o = nullptr;
        try {
                std::string d(pkg.message.get(), pkg.size);
                using U = platform::utils;
                o = U::parse_json<Poco::JSON::Object::Ptr>(d);
        } catch (const Poco::Exception& e) {
                METAX_ERROR(e.displayText());
        } catch (...) {
                METAX_ERROR("Failed to parse message");
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return o;
}

bool leviathan::metax::kernel::
is_valid_master_node(const Poco::JSON::Object::Ptr& obj) const
{
        return nullptr != obj
                && obj->has("pieces")
                && obj->has("content_type")
                && obj->has("size");
}

bool leviathan::metax::kernel::
is_valid_shared_object(const Poco::JSON::Object::Ptr& obj) const
{
        return nullptr != obj
                && obj->has("aes_key")
                && obj->has("aes_iv")
                && obj->has("master_uuid");
}

bool leviathan::metax::kernel::
is_streaming_media(request& req)
{
        const std::string& ct = req.content_type;
        if (! ct.empty()) {
                std::string m = ct.substr(0, 5);
                return "video" == m || "audio" == m;
        }
        return false;
}

void leviathan::metax::kernel::
lookup_master_node(const UUID& uuid, const request& preq)
{
        METAX_TRACE(__FUNCTION__);
        ID32 mid = get_request_id();
        request& r = m_requests[mid];
        r = request(mid, uuid,
                        request::lookup_in_db,
                        preq.cmd,
                        request::t_kernel);
        r.parent_req_id = preq.request_id;
        post_storage_look_up_request(r);
        METAX_TRACE(std::string("END") + __FUNCTION__);
}

uint32_t leviathan::metax::kernel::
increase_version(Poco::JSON::Object::Ptr obj) const
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != obj);
        uint32_t v = DATA_VERSION_START;
        if (obj->has("version")) {
                v = obj->getValue<uint32_t>("version");
                ++v;
        }
        METAX_TRACE(std::string("END") + __FUNCTION__);
        return v;
}

void leviathan::metax::kernel::
parse_master_object(request& req, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        using U = leviathan::platform::utils;
        using P = Poco::JSON::Object::Ptr;
        Poco::Exception err("Invalid master object");
        if (nullptr == pkg.message.get()) {
                throw err;
        }
        P obj = U::parse_json<P>(pkg.message.get());
        if (! is_valid_master_node(obj)) {
                throw err;
        }
        std::string c = obj->getValue<std::string>("content_type");
        if (metax::get == req.cmd) {
                req.content_type = c;
                req.master_json = obj;
        } else {
                poco_assert(metax::update == req.cmd
                         || metax::copy == req.cmd || metax::del == req.cmd);
                if (req.content_type.empty()) {
                        req.content_type = c;
                }
                req.old_master_json = obj;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_unencrypted_master_object(request& req, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        try {
                parse_master_object(req, pkg);
                process_unencrypted_master_json(req);
        } catch (const Poco::Exception& e) {
                METAX_WARNING(e.displayText() + " : " + req.uuid.toString());
                std::string s = "Master object is not valid or "
                        "encrypted but key not found.";
                req.data.set_payload(s);
                return handle_failed_request(req);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
got_master_object(const platform::default_package& in, request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get == req.cmd || metax::update == req.cmd
                       || metax::del == req.cmd || metax::copy == req.cmd);
        poco_assert(request::t_ims == req.typ);
        Poco::JSON::Object::Ptr p = get_master_info_from_user_json(req.uuid);
        if (p.isNull()) {
                if (metax::update == req.cmd || metax::copy == req.cmd) {
                        req.aes_key = "";
                        req.aes_iv = "";
                }
        } else {
                get_keys_from_master_info(req, p);
        }
        if (req.aes_key.empty() && req.aes_iv.empty()) {
                handle_unencrypted_master_object(req, in);
        } else {
                post_key_manager_decrypt_request(req, in,
                                req.aes_key, req.aes_iv);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
get_keys_from_master_info(request& req, Poco::JSON::Object::Ptr u)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != u);
        Poco::Dynamic::Var v1 = u->get("aes_key");
        Poco::Dynamic::Var v2 = u->get("aes_iv");
        poco_assert(! v1.isEmpty() && ! v2.isEmpty()
                        && v1.isString() && v2.isString());
        req.aes_key = v1.extract<std::string>();
        req.aes_iv = v2.extract<std::string>();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::JSON::Object::Ptr leviathan::metax::kernel::
get_master_info_from_user_json(const UUID& uuid) const
{
        return get_master_info_from_user_json(uuid.toString());
}

Poco::JSON::Object::Ptr leviathan::metax::kernel::
get_master_info_from_user_json(const std::string& uuid) const
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr own = m_user_json->getObject("owned_data");
        poco_assert(! own.isNull());
        Poco::JSON::Object::Ptr u = own->getObject(uuid);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return u;
}

void leviathan::metax::kernel::
process_share_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::share == req.cmd);
        if (req.aes_key.empty() && req.aes_iv.empty()) {
                send_ims_share_confirm(req);
        } else {
                kernel_key_manager_package& out = *key_manager_tx;
                out = *ims_rx;
                out.request_id = req.request_id;
                out.cmd = metax::reencrypt_key;
                out.aes_key = req.aes_key;
                out.aes_iv = req.aes_iv;
                key_manager_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
set_piece_key(request& req, request& preq)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr pieces = preq.master_json->getObject("pieces");
        Poco::JSON::Object::Ptr pjo = pieces->getObject(req.uuid.toString());
        if (pjo->has("aes_iv")) {
                req.aes_iv = pjo->getValue<std::string>("aes_iv");
        } else {
                req.aes_iv = preq.aes_iv;
        }
        if (pjo->has("aes_key")) {
                req.aes_key = pjo->getValue<std::string>("aes_key");
        } else {
                req.aes_key = preq.aes_key;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
construct_get_request_for_piece(const UUID& uuid, request& preq, ID32 size)
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = get_request_id();
        poco_assert(metax::get == preq.cmd);
        request& r = m_requests[id];
        r = request(id, uuid, request::lookup_in_db,
                        metax::update == preq.cmd ? metax::save : preq.cmd,
                        request::t_kernel);
        r.parent_req_id = preq.request_id;
        r.cache = preq.cache;
        r.size = size;
        set_piece_key(r, preq);
        preq.units.insert(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_look_up_request_for_piece(const UUID& uuid,
                                        request& preq, Poco::UInt64 size)
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = get_request_id();
        poco_assert(metax::save == preq.cmd || metax::get == preq.cmd
                        || metax::update == preq.cmd);
        request& r = m_requests[id];
        r = request(id, uuid, request::lookup_in_db,
                        metax::update == preq.cmd ? metax::save : preq.cmd,
                        request::t_kernel);
        set_piece_key(r, preq);
        r.parent_req_id = preq.request_id;
        r.cache = preq.cache;
        r.size = size;
        r.content_type = preq.content_type;
        post_storage_look_up_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
prepare_file_write(Poco::JSON::Object::Ptr master, const std::string& path)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != master);
        std::ofstream ofs(path);
        if (master->has("content")) {
                std::string c = platform::utils::base64_decode(
                               master->getValue<std::string>("content"));
                ofs.write(c.c_str(), c.size());
        } else {
                ofs.seekp((long long)((Poco::UInt64)master->get("size") - 1));
                ofs.write("0", 1);
        }
        ofs.close();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
start_getting_pieces(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != r.master_json && r.master_json->has("pieces"));
        r.response_file_path =
                new Poco::TemporaryFile(platform::utils::tmp_dir);
        r.file_path = r.response_file_path->path();
        prepare_file_write(r.master_json, r.file_path);
        Poco::JSON::Object::Ptr obj = r.master_json->getObject("pieces");
        poco_assert(nullptr != obj);
        if (0 == obj->size()) {
                return send_ims_requested_data(r);
        }
        Poco::JSON::Object::ConstIterator b = obj->begin();
        for (int index = 0; b != obj->end(); ++b, ++index) {
                Poco::JSON::Object::Ptr o =
                        b->second.extract<Poco::JSON::Object::Ptr>();
                ID32 s = o->getValue<ID32>("size");
                construct_get_request_for_piece(UUID(b->first), r, s);
        }
        for (int i = 0; i < max_get_count; ++i) {
                post_storage_next_piece_get_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_get_request_for_streaming_pieces(request& preq)
{
        METAX_TRACE(__FUNCTION__);
        auto pi = m_pending_ims_get_requests.find(preq.uuid);
        poco_assert(m_pending_ims_get_requests.end() != pi);
        for (Poco::UInt64 p : pi->second) {
                request& r = m_requests[p];
                post_storage_next_streaming_chunk_get_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_decrypt_request(request& req, platform::default_package pkg,
                const std::string& key, const std::string& iv)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(! key.empty() && ! iv.empty());
        kernel_key_manager_package& out = *key_manager_tx;
        out = pkg;
        out.request_id = req.request_id;
        if ("video/mp2t" == req.content_type) {
                out.cmd = metax::decrypt_with_portions;
        } else {
                out.cmd = metax::decrypt;
        }
        out.aes_key = key;
        out.aes_iv = iv;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_requested_data(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get == req.cmd);
        ims_kernel_package pkg;
        pkg.uuid = req.uuid;
        pkg.file_path = req.file_path;
        pkg.response_file_path = req.response_file_path;
        pkg.content_type = req.content_type;
        pkg.cmd = metax::get_data;
        send_ims_response(pkg);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_response(const ims_kernel_package& pkg)
{
        METAX_TRACE(__FUNCTION__);
        uuid_request_map::iterator p =
                        m_pending_ims_get_requests.find(pkg.uuid);
        poco_assert(m_pending_ims_get_requests.end() != p);
        typedef std::vector<ID32>::const_iterator IDX;
        for (IDX b = p->second.begin(); b != p->second.end(); ++b) {
                if (m_ims_req_ids.end() == m_ims_req_ids.find(*b)) {
                        continue;
                }
                (*ims_tx) = pkg;
                (*ims_tx).request_id = m_ims_req_ids[*b];
                ims_tx.commit();
                m_ims_req_ids.erase(*b);
                m_requests.erase(*b);
        }
        m_pending_ims_get_requests.erase(pkg.uuid);
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_share_confirm(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::share == req.cmd);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[req.request_id];
        out.cmd = metax::share_confirm;
        out.uuid = req.uuid;
        out.aes_key = req.aes_key;
        out.aes_iv = req.aes_iv;
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
remove_unnecessary_uuids(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == r.cmd || metax::del == r.cmd);
        poco_assert(nullptr != r.old_master_json);
        r.data_version = increase_version(r.old_master_json);
        Poco::JSON::Object::Ptr p = r.old_master_json->getObject("pieces");
        if (nullptr == p) {
                return;
        }
        std::vector<std::string> vec;
        p->getNames(vec);
        for (unsigned int i = 0; i < vec.size(); ++i) {
                Poco::UUID u(vec[i]);
                ID32 id = handle_piece_delete(u, r.request_id);
                r.units.insert(id);
        }
        r.units.insert(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_save_confirm(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        poco_assert(metax::save == r.cmd || metax::update == r.cmd);
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[r.request_id];
        out.cmd = metax::save == r.cmd ? metax::save_confirm : metax::updated;
        out.uuid = r.uuid;
        ims_tx.commit();
        if (metax::update == r.cmd) {
                update_owned_uuid_in_user_json(r);
                post_storage_remove_from_cache_request(r.uuid);
                remove_unnecessary_uuids(r);
        }
        m_ims_req_ids.erase(r.request_id);
        // Do not remove main request to be available for backup process.
        //m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
update_owned_uuid_in_user_json(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == r.cmd);
        poco_assert(request::t_ims == r.typ);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr own = m_user_json->getObject("owned_data");
        poco_assert(! own.isNull());
        Poco::JSON::Object::Ptr u = own->getObject(r.uuid.toString());
        if (u.isNull()) {
                add_shared_info_in_user_json(r, r.aes_key, r.aes_iv);
        } else {
                u->set("aes_key", r.aes_key);
                u->set("aes_iv", r.aes_iv);
                send_update_to_user_manager(metax::save, r.uuid,
                                        r.aes_iv, r.aes_key,
                                        u->getValue<int>("copy_count"));
        }
        if (r.local) {
                add_new_uuid_in_device_json(r, r.uuid, r.data_version);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
update_copy_count_of_owned_uuid_in_user_json(const Poco::UUID& uuid, int count)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr own = m_user_json->getObject("owned_data");
        poco_assert(! own.isNull());
        Poco::JSON::Object::Ptr u = own->getObject(uuid.toString());
        if (! u.isNull()) {
                u->set("copy_count", count);
                send_update_to_user_manager(metax::save, uuid,
                                        u->optValue<std::string>("aes_iv", ""),
                                        u->optValue<std::string>("aes_key", ""),
                                        count);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
mark_uuid_as_deleted_in_device_json(const Poco::UUID& uuid,
                uint32_t dversion, Poco::UInt64 last_updated)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out = *backup_tx;
        out.cmd = metax::del;
        out.is_deleted = true;
        out.data_version = dversion;
        out.last_updated = last_updated;
        out.uuid = uuid;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
remove_owned_uuid_in_user_json(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::del == r.cmd);
        poco_assert(request::t_kernel == r.typ || request::t_ims == r.typ);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr own = m_user_json->getObject("owned_data");
        poco_assert(! own.isNull());
        own->remove(r.uuid.toString());
        send_update_to_user_manager(metax::del, r.uuid);
        mark_uuid_as_deleted_in_device_json(r.uuid, r.data_version,
                        r.last_updated);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_new_uuid_in_device_json(const request& req, const Poco::UUID& uuid,
                uint32_t version)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out = *backup_tx;
        out.cmd = metax::save;
        out.is_deleted = false;
        out.data_version = version;
        out.last_updated = req.last_updated;
        out.expire_date = req.expire_date;
        out.uuid = uuid;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_new_uuid_in_device_json(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out = *backup_tx;
        out.cmd = metax::save;
        out.is_deleted = false;
        out.data_version = in.data_version;
        out.last_updated = in.last_updated;
        out.expire_date = in.expire_date;
        out.uuid = in.uuid;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_new_piece_of_copy_req_in_user_json(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        if (r.local) {
                add_new_uuid_in_device_json(r, r.copy_uuid, r.data_version);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_new_owned_uuid_to_user_json(request& r, const save_unit& u)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save == r.cmd || metax::update == r.cmd);
        poco_assert(nullptr != m_user_json);
        Poco::JSON::Object::Ptr own = m_user_json->getObject("owned_data");
        poco_assert(! own.isNull());
        uint32_t v = DATA_VERSION_START;
        if (u.type == save_unit::master) {
                Poco::JSON::Object::Ptr d = new Poco::JSON::Object;
                d->set("copy_count", u.bkp_count);
                d->set("aes_key", r.aes_key);
                d->set("aes_iv", r.aes_iv);
                v = r.data_version;
                own->set(u.uuid.toString(), d);
                send_update_to_user_manager(metax::save, u.uuid,
                                r.aes_iv, r.aes_key, u.bkp_count);
        }
        if (r.local) {
                add_new_uuid_in_device_json(r, u.uuid, v);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::UInt64 leviathan::metax::kernel::
get_start_position(request& req, const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json && req.master_json->has("pieces"));
        Poco::JSON::Object::Ptr p = req.master_json->getObject("pieces");
        poco_assert(nullptr != p);
        Poco::JSON::Object::Ptr obj = p->getObject(uuid.toString());
        poco_assert(nullptr != obj);
        poco_assert(obj->has("from") && obj->has("size"));
        Poco::UInt64 from = obj->getValue<Poco::UInt64>("from");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return from;
}

void leviathan::metax::kernel::
write_data_in_tmp(const std::string& tmp, const platform::default_package& data)
{
        METAX_TRACE(__FUNCTION__);
        std::fstream fs(tmp, std::ios_base::out | std::ios_base::binary);
        fs.write(data.message, data.size);
        fs.close();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::TemporaryFile* leviathan::metax::kernel::
prepare_partial_chunk(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.streaming_state.list);
        auto& v = *req.streaming_state.list;
        auto& curr = req.streaming_state.curr;
        std::ifstream ifs (v[curr].file_path->path(), std::ios::binary);
        poco_assert(ifs.is_open());
        Poco::UInt64 b = 0;
        if (v[curr].start < req.get_range.first) {
                b = req.get_range.first - v[curr].start;
                ifs.seekg(b);
        }
        Poco::UInt64 e = v[curr].size;
        if ((v[curr].start + v[curr].size - 1) > req.get_range.second) {
                e = req.get_range.second - v[curr].start + 1;
        }
        Poco::TemporaryFile* tmp =
                new Poco::TemporaryFile(platform::utils::tmp_dir);
        std::fstream ofs(tmp->path(),
                        std::ios_base::out | std::ios_base::binary);
        char c = ifs.get();
        while(!ifs.eof() && b < e) {
                ofs.put(c);
                c = ifs.get();
                ++b;
        }
        ifs.close();
        ofs.close();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return tmp;
}

void leviathan::metax::kernel::
send_available_chunks_to_ims(request& req, const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.streaming_state.list);
        auto& v = *req.streaming_state.list;
        auto& curr = req.streaming_state.curr;
        auto& last = req.streaming_state.last;
        while (last > curr && v[curr].available == true) {
                ims_kernel_package pkg;
                pkg.uuid = uuid;
                if ((v[curr].start < req.get_range.first) ||
                                (v[curr].start + v[curr].size - 1) >
                                                        req.get_range.second) {
                        pkg.response_file_path = prepare_partial_chunk(req);
                } else {
                        pkg.response_file_path = v[curr].file_path;
                }
                pkg.content_type = req.content_type;
                pkg.cmd = metax::get_data;
                (*ims_tx) = pkg;
                (*ims_tx).request_id = m_ims_req_ids[req.request_id];
                ims_tx.commit();
                ++curr;
        }
        post_storage_next_streaming_chunk_get_request(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

Poco::UInt64 leviathan::metax::kernel::
get_streaming_chunk_index(request& preq, const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != preq.streaming_state.list);
        auto& v = *preq.streaming_state.list;
        auto it = std::find_if(v.begin(), v.end(),
                                [&uuid](const chunk_streaming_info& a) {
                        return a.uuid == uuid;
                }
        );
        poco_assert(v.end() != it);
        Poco::UInt64 i = std::distance(v.begin(), it);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return i;
}

void leviathan::metax::kernel::
handle_streaming_chunk(const platform::default_package& data, request& preq,
                const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == preq.typ);
        Poco::UInt64 i = get_streaming_chunk_index(preq, uuid);
        poco_assert(nullptr != preq.streaming_state.list);
        auto& v = *preq.streaming_state.list;
        write_data_in_tmp(v[i].file_path->path(), data);
        v[i].available = true;
        process_pending_get_streaming_requests(preq, i);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_pending_get_streaming_requests(request& preq, Poco::UInt64 i)
{
        METAX_TRACE(__FUNCTION__);
        auto p = m_pending_ims_get_requests.find(preq.uuid);
        poco_assert(m_pending_ims_get_requests.end() != p);
        auto& v = p->second;
        for (const auto& b : v) {
                poco_assert(m_requests.end() != m_requests.find(b));
                request& rj = m_requests[b];
                if (i == rj.streaming_state.curr) {
                        send_available_chunks_to_ims(rj, preq.uuid);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_next_piece_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        if (! req.units.empty()) {
                auto b = m_requests.find(*req.units.begin());
                poco_assert(m_requests.end() != b);
                post_storage_look_up_request(b->second);
                req.units.erase(*req.units.begin());
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_next_streaming_chunk_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(! req.streaming_state.list.isNull());
        auto& v = *req.streaming_state.list;
        Poco::UInt64 next_requested = req.streaming_state.curr;
        for (int i = 0; i < max_get_count &&
                        next_requested < req.streaming_state.last; ++i) {
                if (! v[next_requested].requested) {
                        poco_assert(! v[next_requested].available);
                        METAX_DEBUG("requested uuid for streaming: "
                                        + v[next_requested].uuid.toString());
                        post_look_up_request_for_piece(
                                        v[next_requested].uuid, req,
                                        v[next_requested].size);
                        v[next_requested].requested = true;
                }
                ++next_requested;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
write_data_to_stream(const platform::default_package& data, request& preq,
                const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        Poco::UInt64 from = get_start_position(preq, uuid);
        poco_assert(request::t_ims == preq.typ);
        Poco::File p(preq.file_path);
        std::fstream fs(preq.file_path, std::ios_base::in
                        | std::ios_base::out | std::ios_base::binary);
        fs.seekp((long long)from);
        fs.write(data.message, data.size);
        fs.close();
        poco_assert(nullptr != preq.master_json);
        preq.remove_uuid_from_master_json(uuid);
        if (preq.is_master_json_empty()) {
                send_ims_requested_data(preq);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_piece(const platform::default_package& in, request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == req.typ);
        poco_assert(metax::get == req.cmd);
        auto it = m_requests.find(req.parent_req_id);
        if (m_requests.end() == it) {
                m_requests.erase(req.request_id);
                return;
        }
        request& preq = it->second;
        post_storage_next_piece_get_request(preq);
        if (req.aes_key.empty() && req.aes_iv.empty()) {
                process_unencrypted_piece(in, preq, req.uuid);
                m_requests.erase(req.request_id);
        } else {
                post_key_manager_decrypt_request(req, in,
                        req.aes_key, req.aes_iv);
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_ims_update_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.old_master_json);
        req.data_version = increase_version(req.old_master_json);
        process_ims_save_request(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_unencrypted_master_json(request& req)
{
        METAX_TRACE(__FUNCTION__);
        switch (req.cmd) {
                case metax::get: {
                        handle_decrypted_master_node_for_get(req);
                        break;
                } case metax::update: {
                        process_ims_update_request(req);
                        break;
                } case metax::del: {
                        process_delete_request(req);
                        break;
                } case metax::copy: {
                        process_copy_request(req);
                        break;
                } default: {
                        METAX_ERROR("Unexpected command");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_get_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::get_data == in.cmd);
        auto i = m_requests.find(in.request_id);
        if (m_requests.end() == i) {
                return;
        }
        request& req = i->second;
        METAX_DEBUG("Received data for uuid=" + req.uuid.toString());
        if (metax::del != req.cmd && req.typ != request::t_user_manager) {
                post_storage_add_cache_request(req);
        }
        post_router_clear_request(in.request_id);
        switch (req.typ) {
                case request::t_ims: {
                        process_received_master_object_from_router(in, req);
                        break;
                } case request::t_kernel: {
                        reduce_router_get_input_size(req.size);
                        process_router_pending_get_requests();
                        handle_piece(in, req);
                        break;
                } case request::t_user_manager: {
                        handle_router_get_data_for_user_manager_request(in);
                        m_requests.erase(in.request_id);
                        break;
                } default: {
                        METAX_WARNING("Unexpected type of request");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_get_data_for_user_manager_request(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        kernel_user_manager_package& out = *user_manager_tx;
        out.cmd = metax::save_data;
        out = in;
        user_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_user_manager_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_user_manager == req.typ);
        switch (req.cmd) {
                case metax::save: {
                        handle_user_json_save_fail(req);
                        break;
                } case metax::get: {
                        handle_user_json_get_fail(req);
                        break;
                } case metax::update: {
                        METAX_INFO("Failed to update user json(uuid="
                                     + req.uuid.toString()
                                     + "), updated count - "
                                     + platform::utils::to_string(req.updated));
                        break;
                } default: {
                }
        }
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_json_save_fail(request& req)
{
        METAX_INFO("Failed to backup user json(uuid=" + req.uuid.toString()
                        + "), copy count - "
                        + platform::utils::to_string(req.updated));
        if (0 == req.updated) {
                (*user_manager_tx).cmd = metax::save_fail;
                user_manager_tx.commit();
        }
        add_shared_info_in_user_json(req, "", "", req.updated + 1);
}

void leviathan::metax::kernel::
handle_user_json_get_fail(request& req)
{
        METAX_INFO("Failed to get user json(uuid="
                        + req.uuid.toString() + ").");
        (*user_manager_tx).cmd = metax::failed;
        user_manager_tx.commit();
}

void leviathan::metax::kernel::
process_received_master_object_from_router(const platform::default_package& in,
                request& req)
{
        METAX_TRACE(__FUNCTION__);
        if (metax::get == req.cmd) {
                got_master_object(in, req);
        } else {
                auto i = m_requests.find(req.parent_req_id);
                if (m_requests.end() == i) {
                        METAX_WARNING("Parent request is not found.");
                        return;
                }
                m_requests.erase(req.request_id);
                got_master_object(in, i->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_add_cache_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        poco_assert(metax::get_data == (*router_rx).cmd);
        poco_assert(metax::get == req.cmd || metax::update == req.cmd
                        || metax::copy == req.cmd);
        kernel_storage_package& out = *cache_tx;
        if (metax::get == req.cmd && true == req.cache) {
                out.cmd = metax::add_in_cache;
                out.uuid = req.uuid;
                out = *router_rx;
                cache_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_storage_remove_from_cache_request(const UUID& uuid)
{
        METAX_TRACE(__FUNCTION__);
        kernel_storage_package& out = *cache_tx;
        out.cmd = metax::delete_from_cache;
        out.uuid = uuid;
        cache_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_find_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        ID32 id = in.request_id;
        METAX_DEBUG("Received find peer request");
        // push request info into map
        request& r = m_requests[in.request_id];
        r = request(id, in.uuid, request::lookup_in_db,
                        in.cmd, request::t_router);
        r.data = in;
        r.data_version = in.data_version;
        kernel_key_manager_package& out = *key_manager_tx;
        out = in;
        out.request_id = id;
        out.cmd = metax::check_id;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save == in.cmd);
        if (2 > m_storage_class) {
                return;
        }
        METAX_INFO("Asking storage to save uuid: " + in.uuid.toString());
        request r(in.request_id, in.uuid, request::save_accept, in.cmd,
                        request::t_router, in.data_version,
                        in.last_updated, in.expire_date);
        add_new_uuid_in_device_json(r, r.uuid, r.data_version);
        std::thread t(&storage::save_data_async_for_router,
                        m_save_storage, in.request_id, in.uuid, in);
        t.detach();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_update_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::update == in.cmd);
        request r(in.request_id, in.uuid, request::lookup_in_db, in.cmd,
                        request::t_router, in.data_version,
                        in.last_updated, in.expire_date);
        post_storage_remove_from_cache_request(r.uuid);
        if (m_save_storage->look_up_data_in_db(r.uuid, false)) {
                add_new_uuid_in_device_json(r, r.uuid, r.data_version);
                notify_update(r.uuid, metax::update, r.data_version);
                std::thread t(&storage::update_data_async_for_router,
                        m_save_storage, in.request_id, in.uuid, in);
                t.detach();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_save_stream_offer(const request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save_stream == r.cmd);
        poco_assert(request::t_router == r.typ);
        METAX_INFO( "Sending save stream offer to router: "
                                                        + r.uuid.toString());
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.cmd = metax::save_stream_offer;
        out.uuid = r.uuid;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_router_save_stream_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save_stream == r.cmd);
        if (m_hosted_livestreams.end() == m_hosted_livestreams.find(r.uuid)) {
                m_hosted_livestreams[r.uuid].reset(new livestream_info(r.request_id));
        } else {
                auto& lsi = m_hosted_livestreams[r.uuid];
                poco_assert(nullptr != lsi);
                post_router_clear_request(lsi->request_id);
                m_requests.erase(lsi->request_id);
                lsi->request_id = r.request_id;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_stream_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save_stream == in.cmd);
        poco_assert(m_requests.end() == m_requests.find(in.request_id));
        if (! m_enable_livstream_hosting) {
                (*router_tx).request_id = in.request_id;
                (*router_tx).cmd = metax::refuse;
                (*router_tx).uuid = in.uuid;
                (*router_tx) = (platform::default_package)in;
                router_tx.commit();
                return;
        }
        request& r = m_requests[in.request_id];
        r = request(in.request_id, in.uuid, request::save_accept, in.cmd,
                        request::t_router);
        post_router_save_stream_offer(r);
        process_router_save_stream_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_recording_start()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::recording_start == in.cmd);
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() == li) {
                METAX_INFO("livestream not found: " + in.uuid.toString());
                (*router_tx).cmd = metax::refuse;
                (*router_tx).request_id = in.request_id;
                (*router_tx).uuid = in.uuid;
                router_tx.commit();
        } else {
                poco_assert(m_requests.end() == m_requests.find(in.request_id));
                request& r = m_requests[in.request_id];
                r = request(in.request_id, in.uuid, request::requested,
                                metax::recording_start, request::t_router);
                create_and_save_master_for_recording(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_recording_stop()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::recording_stop == in.cmd);
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() == li) {
                METAX_INFO("livestream not found: " + in.uuid.toString());
                (*router_tx).cmd = metax::refuse;
                (*router_tx).request_id = in.request_id;
                (*router_tx).uuid = in.uuid;
                (*router_tx) = (platform::default_package)in;
                router_tx.commit();
        } else {
                poco_assert(nullptr != li->second);
                stop_recording_and_send_response_to_router(*li->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
stop_recording_and_send_response_to_router(livestream_info& li)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        const Poco::UUID rec_uuid(std::string(in.message.get(), in.size));
        auto ri = std::find_if(li.recordings.begin(), li.recordings.end(),
                        [&rec_uuid](const recording_info& i) {
                                return i.uuid == rec_uuid;
                        });
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        if (li.recordings.end() != ri) {
                METAX_INFO("Stopping recording: " + rec_uuid.toString());
                Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
                Poco::UUID uuid = g.createRandom();
                post_storage_save_request(li.rec_chunk, uuid);
                METAX_INFO("saving livestream chunk for recording: " + uuid.toString());
                add_livestream_chunk_to_recording(li, ri, uuid);
                li.recordings.erase(ri);
                out.cmd = metax::recording_stopped;
        } else {
                std::string m = "no recording " + rec_uuid.toString() +
                                " found for livestream " + in.uuid.toString();
                METAX_INFO(m);
                out.cmd = metax::recording_fail;
                out.set_payload(m);
        }
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save_data == in.cmd);
        ID32 id = in.request_id;
        if (m_requests.end() == m_requests.find(id)) {
                return;
        }
        request& r = m_requests[id];
        poco_assert(request::save_accept == r.st);
        poco_assert(in.uuid == r.uuid);
        r.data.set_payload(in);
        post_storage_save_data_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_master_object_save_offer_for_copy_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == r.cmd);
        kernel_router_package& out = *router_tx;
        out.uuid = r.uuid;
        out = r.data;
        out.request_id = r.request_id;
        out.cmd = metax::save_data;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_offer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save_offer == in.cmd);
        ID32 id = in.request_id;
        if (m_save_units.end() != m_save_units.find(id)) {
                kernel_router_package& out = *router_tx;
                save_unit& u = m_save_units[id];
                u.winners_count += in.winners_count;
                out.uuid = u.uuid;
                // TODO this will not work for encrypted case
                out = u.content;
                out.request_id = id;
                out.cmd = metax::save_data;
                router_tx.commit();
        } else if (m_requests.end() != m_requests.find(id)) {
                request& r = m_requests[id];
                poco_assert(metax::copy == r.cmd);
                handle_master_object_save_offer_for_copy_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_save_stream_offer_to_ims(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[r.request_id];
        out.cmd = metax::save_stream_offer;
        out.uuid = r.uuid;
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_router_save_stream_offer_for_encrypted_stream(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(r.enc);
        Poco::JSON::Object::Ptr p = get_master_info_from_user_json(r.uuid);
        if (p.isNull()) {
                post_key_manager_gen_key_for_stream(r);
        } else {
                get_keys_from_master_info(r, p);
                if (r.aes_key.empty() && r.aes_iv.empty()) {
                        post_key_manager_gen_key_for_stream(r);
                } else {
                        post_key_manager_prepare_cipher_for_stream(r);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_stream_offer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save_stream_offer == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        if (r.enc) {
                process_router_save_stream_offer_for_encrypted_stream(r);
        } else {
                add_shared_info_in_user_json(r, "", "");
        }
        post_save_stream_offer_to_ims(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_livestream_content_for_ims_requests(std::unique_ptr<livestream_info>& li)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != li);
        const kernel_router_package& in = *router_rx;
        if (m_pending_ims_get_requests.end() !=
                                m_pending_ims_get_requests.find(in.uuid)) {
                poco_assert(m_requests.end() !=
                                m_requests.find(li->request_id));
                request& lir = m_requests[li->request_id];
                if (! lir.aes_key.empty() && ! lir.aes_iv.empty()) {
                        post_key_manager_stream_decrypt_request(lir, in);
                } else {
                        send_livestream_content_to_ims(in.uuid, in);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_livestream_content_for_router_request(std::vector<ID32>& listeners)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        using U = platform::utils;
        METAX_INFO("Livestream listeners size from router: " +
                        U::to_string(listeners.size()));
        for (ID32 i : listeners) {
                auto lri = m_requests.find(i);
                if (m_requests.end() == lri) {
                        METAX_INFO("There is a listener with id: "
                                   + U::to_string(i) +
                                   ", but corresponding request isn't found");
                        continue;
                }
                request& lr = lri->second;
                METAX_INFO( "Listener request: " +
                                U::to_string(lr.request_id));
                send_get_livestream_content_to_router(lr, *router_rx);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_content_to_recording_chunk(livestream_info& li,
                                platform::default_package d, uint32_t orig_size)
{
        if (li.recordings.empty()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        li.rec_chunk.append(d.message.get(), d.size);
        li.rec_chunk_size += orig_size;
        if (recording_max_chunk_size <= li.rec_chunk.size()) {
                add_livestream_chunk_to_all_recordings(li);
                li.rec_chunk = "";
                li.rec_chunk_size = 0;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_livestream_content()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() == li) {
                // TODO shouldn't be peer informed so it doesn't continue
                // sending not uncessary ???
                METAX_INFO( "No hosted livestream found: " +
                                in.uuid.toString());
                return;
        }
        poco_assert(nullptr != li->second);
        ID32 i = li->second->request_id;
        METAX_INFO("Livestream request id: " +
                        platform::utils::to_string(i));
        handle_livestream_content_for_ims_requests(li->second);
        handle_livestream_content_for_router_request(li->second->listeners);
        add_content_to_recording_chunk(*li->second, in, in.data_version);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_livestream_content_to_ims(UUID uuid, platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        uuid_request_map::iterator i = m_pending_ims_get_requests.find(uuid);
        if (m_pending_ims_get_requests.end() == i) {
                METAX_INFO("no entry in pendings: " + uuid.toString());
                return;
        }
        typedef std::vector<ID32>::const_iterator IDX;
        using U = platform::utils;
        METAX_INFO("Livestream listeners size from ims: " +
                        U::to_string(i->second.size()));
        for (IDX b = i->second.begin(); b != i->second.end(); ++b) {
                METAX_INFO("found pending request: " +
                                platform::utils::to_string(*b));
                if (m_ims_req_ids.end() == m_ims_req_ids.find(*b)) {
                        METAX_INFO("Corresponding ims request isn't found");
                        continue;
                }
                (*ims_tx) = pkg;
                (*ims_tx).request_id = m_ims_req_ids[*b];
                (*ims_tx).cmd = metax::livestream_content;
                ims_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
reduce_storage_input_size()
{
        if (0 == m_storage_input_cnt) {
                METAX_ERROR("Storage input count could not be negative !!!");
        } else {
                --m_storage_input_cnt;
        }
}

void leviathan::metax::kernel::
reduce_router_save_input_size(uint32_t s)
{
        if (s > m_router_save_input_size) {
                METAX_ERROR("Router maximum input size for save requests "
                                "could not be negative !!!");
                m_router_save_input_size = 0;
        } else {
                m_router_save_input_size -= s;
        }
}

void leviathan::metax::kernel::
reduce_router_get_input_size(uint32_t s)
{
        if (s > m_router_get_input_size) {
                METAX_ERROR("Router maximum input size for get requests "
                                "could not be negative !!!");
                m_router_get_input_size = 0;
        } else {
                m_router_get_input_size -= s;
        }
}

void leviathan::metax::kernel::
print_debug_info() const
{
        METAX_TRACE(__FUNCTION__);
        METAX_DEBUG("m_ims_req_ids size = " +
                platform::utils::to_string(m_ims_req_ids.size()));
        METAX_DEBUG("m_requests size = " +
                platform::utils::to_string(m_requests.size()));
        METAX_DEBUG("m_save_units size = " +
                platform::utils::to_string(m_save_units.size()));
        METAX_DEBUG("m_storage_input_cnt = " +
                platform::utils::to_string(m_storage_input_cnt));
        METAX_DEBUG("m_router_save_input_size = " +
                platform::utils::to_string(m_router_save_input_size));
        METAX_DEBUG("m_router_get_input_size = " +
                platform::utils::to_string(m_router_get_input_size));
        METAX_DEBUG("m_pending_ims_get_requests size = " +
                platform::utils::to_string(m_pending_ims_get_requests.size()));
        METAX_DEBUG("m_pending_save_requests is empty: " +
                platform::utils::to_string(m_pending_save_requests.empty()));
        METAX_DEBUG("m_pending_router_save_reqs size: " +
                platform::utils::to_string(m_pending_router_save_reqs.size()));
        METAX_DEBUG("m_pending_router_get_reqs size: " +
                platform::utils::to_string(m_pending_router_get_reqs.size()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_confirm_for_request_object(request& r)
{
        METAX_TRACE(__FUNCTION__);
        if (metax::copy == r.cmd) {
                handle_copy_success_for_master_object(r);
        } else if (metax::save == r.cmd) {
                poco_assert(request::t_user_manager == r.typ);
                handle_save_confirm_for_user_json(r);
        } else if (metax::update == r.cmd) {
                poco_assert(request::t_user_manager == r.typ);
                handle_user_json_updated_response(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_json_updated_response(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(r.uuid == m_user_json_uuid);
        ++r.updated;
        METAX_DEBUG("Updated user json(uuid="
                        + m_user_json_uuid.toString() + "), copy count - "
                        + platform::utils::to_string(r.updated));
        if (m_data_copy_count - 1 == r.updated) {
                m_requests.erase(r.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_confirm_for_user_json(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(r.uuid == m_user_json_uuid);
        ++r.updated;
        METAX_DEBUG("Received save confirm for user json(uuid="
                        + m_user_json_uuid.toString() + "), copy count - "
                        + platform::utils::to_string(r.updated));
        if (1 == r.updated) {
                kernel_user_manager_package& out = *user_manager_tx;
                out.cmd = metax::save_confirm;
                user_manager_tx.commit();
                std::string s = "{\"event\":\"user json backed up\"}";
                send_notification_to_ims(r.request_id, s);
        }
        if (m_data_copy_count - 1 == r.updated) {
                add_shared_info_in_user_json(r, "", "", m_data_copy_count);
                m_requests.erase(r.request_id);
                handle_user_manager_on_update_register(m_user_json_uuid);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_save_confirm()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::save_confirm == in.cmd || metax::updated == in.cmd);
        if (m_save_units.end() != m_save_units.find(in.request_id)) {
                save_unit& u = m_save_units[in.request_id];
                handle_save_confirm_for_local_save_unit(u, in.request_id);
        } else if (m_requests.end() != m_requests.find(in.request_id)) {
                request& r = m_requests[in.request_id];
                handle_router_save_confirm_for_request_object(r);
        } else {
                std::string w = "Request by request id - ";
                w += platform::utils::to_string(in.request_id);
                METAX_WARNING(w + " is not found");
                post_router_clear_request(in.request_id);
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_peer_found(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_requests.end() != m_requests.find(id));
        poco_assert(router_rx.has_data());
        kernel_router_package& out = *router_tx;
        request& req = m_requests[id];
        out.request_id = id;
        out.cmd = metax::send_to_peer;
        out = req.data;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_send_to_peer_confirm(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.cmd = metax::send_to_peer_confirm;
        out.request_id = id;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_send_to_peer(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        if (m_requests.end() == m_requests.find(id)) {
                return;
        }
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::send_to_peer == in.cmd);
        poco_assert(metax::find_peer == m_requests[id].cmd);
        send_notification_to_ims(id, in);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_failed()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        ID32 id = in.request_id;
        if (m_requests.end() == m_requests.find(id)) {
                return;
        }
        switch (m_requests[id].cmd) {
                case metax::send_to_peer: {
                        ims_kernel_package& out = *ims_tx;
                        out.request_id = m_ims_req_ids[id];
                        out.cmd = metax::deliver_fail;
                        out = in;
                        ims_tx.commit();
                        break;
                } default: {
                        METAX_WARNING("This case is not handled yet.");
                }
        }
        m_ims_req_ids.erase(id);
        post_router_clear_request(id);
        m_requests.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_send_to_peer_confirm(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        if (m_requests.end() == m_requests.find(id)) {
                return;
        }
        if (m_ims_req_ids.end() == m_ims_req_ids.find(id)) {
                return;
        }
        poco_assert(metax::send_to_peer == m_requests[id].cmd);
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = metax::send_to_peer_confirm;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        post_router_clear_request(id);
        m_requests.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_notification_to_ims(ID32 id, platform::default_package m)
{
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        out.request_id = id;
        out.cmd = metax::send_to_peer;
        out = m;
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_notification_to_user_manager()
{
        METAX_TRACE(__FUNCTION__);
        kernel_user_manager_package& out = *user_manager_tx;
        out.cmd = metax::connected;
        user_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_notification_to_router(request& r, platform::default_package p,
                                        metax::command cmd, uint32_t v)
{
        METAX_TRACE(__FUNCTION__);
        //platform::default_package pkg;
        //pkg.set_payload(p);
        //post_find_peer_request(r.request_id, pkg);
        (*router_tx).request_id = r.request_id;
        (*router_tx).cmd = cmd;
        (*router_tx).set_payload(p);
        (*router_tx).uuid = r.uuid;
        (*router_tx).data_version = v;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
notify_update(const Poco::UUID& u, metax::command cmd, uint32_t v)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == cmd || metax::del == cmd);
        if (m_update_listeners->has(u.toString())) {
                Poco::JSON::Array::Ptr parr =
                        m_update_listeners->getArray(u.toString());
                for (size_t i = 0; i < parr->size(); ++i) {
                        std::string p = parr->getElement<std::string>(i);
                        request& r =
                                create_request_for_update_notification(u, cmd);
                        if ("" == p) {
                                send_notification_to_ims(r.request_id, r.data);
                        } else {
                                METAX_INFO("Found remote update listener");
                                send_notification_to_router(r, p,
                                                metax::update == cmd ?
                                                        metax::notify_update :
                                                        metax::notify_delete,
                                                        v);
                        }
                }
                if (metax::del == cmd) {
                        m_update_listeners->remove(u.toString());
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_info_dump_confirm(ID32 req_id)
{
        METAX_TRACE(__FUNCTION__);
        auto ri = m_requests.find(req_id);
        if (m_requests.end() == ri) {
                return;
        }
        request& r = ri->second;
        poco_assert(metax::dump_user_info == r.cmd
                        || metax::regenerate_user_keys == r.cmd);
        ++r.updated;
        if (2 == r.updated) {
                send_dump_user_info_succeeded_response_to_ims(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_save_confirm_for_master_object(save_unit& u)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(1 == u.bkp_count);
        poco_assert(nullptr != u.main_request);
        request& r = *u.main_request;
        poco_assert(save_unit::master == u.type);
        if (metax::update == r.cmd) {
                update_copy_count_of_owned_uuid_in_user_json(u.uuid,
                                u.bkp_count);
                notify_update(u.uuid, metax::update, r.data_version);
                post_storage_remove_from_cache_request(u.uuid);
        } else {
                add_new_owned_uuid_to_user_json(r, u);
        }
        send_ims_save_confirm(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
start_master_object_save(request& r)
{
        METAX_TRACE(__FUNCTION__);
        Poco::UInt64 s = get_data_size_from_request_object(r);
        Poco::JSON::Object::Ptr m = create_master_json_template(r, s,
                        r.content_type);
        poco_assert(nullptr != m);
        if (s <= max_chunk_size) {
                platform::default_package p =
                        get_piece_data_for_save(r, 0, s);
                m->set("content", platform::utils::base64_encode(p));
        }
        poco_assert(nullptr != r.master_json);
        m->set("pieces", r.master_json);
        platform::default_package pkg;
        pkg.set_payload(m);
        if (r.enc) {
                post_key_manager_encrypt_request(r, pkg, r.aes_key, r.aes_iv);
        } else {
                save_master_object(r, pkg);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_save_confirm_for_piece(save_unit& u, ID32 uid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(1 == u.bkp_count);
        poco_assert(nullptr != u.main_request);
        request& r = *u.main_request;
        poco_assert(save_unit::piece == u.type);
        add_new_owned_uuid_to_user_json(r, u);
        r.units.erase(uid);
        r.saved_units.insert(u.uuid);
        if (r.units.empty()) {
                start_master_object_save(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
clean_up_save_request(save_unit& u, ID32 uid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != u.main_request);
        request& r = *u.main_request;
        if (r.local) {
                reduce_storage_input_size();
                if (save_unit::master == u.type) {
                        m_requests.erase(r.request_id);
                }
        } else {
                reduce_router_save_input_size(u.size);
                remove_from_backup_units(r, uid);
                post_router_clear_request(uid);
        }
        m_save_units.erase(uid);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_first_backup_of_save_unit(save_unit& u, ID32 uid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(1 == u.bkp_count);
        poco_assert(nullptr != u.main_request);
        if (save_unit::master == u.type) {
                process_save_confirm_for_master_object(u);
        } else {
                poco_assert(save_unit::piece == u.type);
                process_save_confirm_for_piece(u, uid);
        }
        if (1 == m_data_copy_count) {
                clean_up_save_request(u, uid);
        } else if (u.main_request->local) {
                reduce_storage_input_size();
                ++u.winners_count;
                m_pending_router_save_reqs.push_front(uid);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_confirm_for_local_save_unit(save_unit& u, ID32 uid)
{
        METAX_TRACE(__FUNCTION__);
        ++u.bkp_count;
        METAX_INFO("Received save confirm for " + u.uuid.toString()
                        + " uuid, copy count - "
                        + platform::utils::to_string(u.bkp_count));
        if (1 == u.bkp_count) {
                handle_first_backup_of_save_unit(u, uid);
        } else {
                update_copy_count_of_owned_uuid_in_user_json(u.uuid,
                                u.bkp_count);
                if (u.bkp_count == m_data_copy_count) {
                        reduce_router_save_input_size(u.size);
                        poco_assert(nullptr != u.main_request);
                        request& r = *u.main_request;
                        remove_from_backup_units(r, uid);
                        m_save_units.erase(uid);
                        post_router_clear_request(uid);
                }
        }
        process_pending_save_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_no_space_for_local_save_unit(save_unit& u, ID32 uid)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(0 == u.bkp_count);
        poco_assert(nullptr != u.main_request);
        u.main_request->local = false;
        reduce_storage_input_size();
        m_pending_router_save_reqs.push_front(uid);
        process_pending_save_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
remove_from_backup_units(request&r, ID32 uid)
{
        r.backup_units.erase(uid);
        if (r.backup_units.empty()) {
                m_requests.erase(r.request_id);
        }
}

void leviathan::metax::kernel::
handle_save_confirm_for_copy_or_remote_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        if (request::t_router == r.typ) {
                if (metax::recording_start == r.cmd) {
                        handle_save_confirm_for_remote_recording_start(r);
                } else {
                        post_router_save_confirmed_response(r);
                }
        } else if (request::t_ims == r.typ) {
                if (metax::copy == r.cmd) {
                        handle_copy_success_for_master_object(r);
                } else if (metax::recording_start == r.cmd) {
                        handle_save_confirm_for_local_recording_start(r);
                }
        } else {
                METAX_WARNING("Invalid request for save confirm response!");
                m_requests.erase(r.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_confirm_for_remote_recording_start(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        auto li = m_hosted_livestreams.find(r.uuid);
        // perhaps wrong assert should if and error report
        poco_assert(m_hosted_livestreams.end() != li);
        poco_assert(nullptr != li->second);
        li->second->recordings.push_back(recording_info(in.uuid, r.master_json));
        recording_info& rec = li->second->recordings.back();
        create_and_save_playlist_for_recording(r, rec, in.uuid);
        kernel_router_package& out = *router_tx;
        out.cmd = metax::recording_started;
        out.uuid = rec.uuid;
        out.request_id = r.request_id;
        router_tx.commit();
        post_router_clear_request(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_save_confirm_for_local_recording_start(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        auto li = m_hosted_livestreams.find(r.uuid);
        // perhaps wrong assert should if and error report
        poco_assert(m_hosted_livestreams.end() != li);
        poco_assert(nullptr != li->second);
        li->second->recordings.push_back(recording_info(in.uuid, r.master_json));
        recording_info& rec = li->second->recordings.back();
        create_and_save_playlist_for_recording(r, rec, in.uuid);
        send_ims_recording_started_response(r, rec.uuid);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_save_confirm()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        poco_assert(metax::save_confirm == in.cmd);
        if (m_save_units.end() != m_save_units.find(in.request_id)) {
                save_unit& u = m_save_units[in.request_id];
                poco_assert(nullptr != u.main_request);
                handle_save_confirm_for_local_save_unit(u, in.request_id);
        } else if (m_requests.end() != m_requests.find(in.request_id)) {
                request& r = m_requests[in.request_id];
                handle_save_confirm_for_copy_or_remote_request(r);
        } else {
                std::string w = "Request by request id - ";
                w += platform::utils::to_string(in.request_id);
                METAX_WARNING(w + " is not found");
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::kernel::
handle_no_space_for_copy_or_remote_request(request& r,
                const kernel_storage_package& in)
{
        METAX_TRACE(__FUNCTION__);
        if (request::t_ims == r.typ) {
                if (metax::copy == r.cmd) {
                        backup_data(r.request_id, r.copy_uuid,
                                        metax::save,
                                        m_data_copy_count, r.data,
                                        r.data_version, r);
                } else if (metax::recording_start == r.cmd) {
                        handle_recording_fail(r, in);
                }
        } else if (request::t_kernel == r.typ) {
                post_router_copy_request(r);
        } else {
                m_requests.erase(r.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_no_space()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_writer_rx.has_data());
        const kernel_storage_package& in = *storage_writer_rx;
        poco_assert(metax::no_space == in.cmd);
        if (m_save_units.end() != m_save_units.find(in.request_id)) {
                save_unit& u = m_save_units[in.request_id];
                poco_assert(nullptr != u.main_request);
                METAX_INFO("Received no space from strage for " +
                                u.uuid.toString() + " uuid");
                handle_no_space_for_local_save_unit(u, in.request_id);
        } else if (m_requests.end() != m_requests.find(in.request_id)) {
                request& r = m_requests[in.request_id];
                handle_no_space_for_copy_or_remote_request(r, in);
        } else {
                std::string w = "Request by request id - ";
                w += platform::utils::to_string(in.request_id);
                METAX_WARNING(w + " is not found");
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_save_confirmed_response(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save == req.cmd || metax::update == req.cmd);
        if (metax::update == req.cmd) {
                notify_update(req.uuid, metax::update, req.data_version);
                (*router_tx).cmd = metax::updated;
        } else {
                (*router_tx).cmd = metax::save_confirm;
        }
        add_new_uuid_in_device_json(req, req.uuid, req.data_version);
        (*router_tx).uuid = req.uuid;
        (*router_tx).request_id = req.request_id;
        router_tx.commit();
        m_requests.erase(req.request_id);
        // Commented out to prevent clearing save request in router. With this
        // we give chance to router to ignore messages arriving late, instead
        // of taking them as a new remote tender.
        post_router_clear_request(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_deleted_for_external_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_router == r.typ);
        poco_assert(metax::del == r.cmd);
        kernel_router_package& out = *router_tx;
        out.cmd = metax::deleted;
        out.uuid = r.uuid;
        out.request_id = r.request_id;
        router_tx.commit();
        notify_update(r.uuid, metax::del, r.data_version);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_delete_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.cmd = metax::del;
        out.uuid = r.uuid;
        out.data_version = r.data_version;
        out.last_updated = r.last_updated;
        out.max_response_cnt = m_data_copy_count - 1;
        out.request_id = r.request_id;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_deleted_response_for_kernel_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == r.typ);
        if (m_requests.end() == m_requests.find(r.parent_req_id)) {
                return;
        }
        request& p = m_requests[r.parent_req_id];
        p.units.erase(r.request_id);
        if (p.units.empty() &&
                     m_ims_req_ids.end() != m_ims_req_ids.find(p.request_id)) {
                ims_kernel_package& out = (*ims_tx);
                out.request_id = m_ims_req_ids[p.request_id];
                out.cmd = metax::deleted;
                out.uuid = p.uuid;
                ims_tx.commit();
                m_ims_req_ids.erase(p.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_deleted_response_for_kernel_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == r.typ);
        METAX_INFO("Deleted " + r.uuid.toString() + " from storage");
        ++r.updated;
        handle_deleted_response_for_kernel_request(r);
        post_router_delete_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_deleted_response_for_ims_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ);
        METAX_INFO("Deleted " + r.uuid.toString() + " from storage");
        ++r.updated;
        handle_deleted_response_for_ims_request(r);
        notify_update(r.uuid, metax::del, r.data_version);
        post_router_delete_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_deleted_response_for_ims_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ);
        r.units.erase(r.request_id);
        if (r.units.empty() &&
                     m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id)) {
                ims_kernel_package& out = (*ims_tx);
                out.request_id = m_ims_req_ids[r.request_id];
                out.cmd = metax::deleted;
                out.uuid = r.uuid;
                ims_tx.commit();
                m_ims_req_ids.erase(r.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_storage_deleted()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(storage_rx.has_data());
        const kernel_storage_package& in = *storage_rx;
        poco_assert(metax::deleted == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        poco_assert(metax::del == r.cmd);
        mark_uuid_as_deleted_in_device_json(r.uuid, r.data_version,
                        r.last_updated);
        switch (r.typ) {
                case request::t_ims: {
                        handle_storage_deleted_response_for_ims_request(r);
                        break;
                } case request::t_kernel: {
                        handle_storage_deleted_response_for_kernel_request(r);
                        break;
                } case request::t_router: {
                        handle_storage_deleted_for_external_request(r);
                        break;
                } default: {
                        METAX_ERROR("This case should not be happened.");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_move_cache_to_storage_completed()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_storage_package& in = *cache_rx;
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        ID32 id = in.request_id;
        poco_assert(request::t_ims == m_requests[id].typ);
        (*ims_tx).cmd = metax::move_cache_to_storage_completed;
        *ims_tx = in;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        (*ims_tx).request_id = m_ims_req_ids[id];
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        m_requests.erase(id);
        METAX_INFO("Moving cache to storage is done");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_update_auth_request(save_unit& u)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        u.winners_count += in.winners_count;
        kernel_router_package& out = *router_tx;
        out = u.content; // send content for update
        out.request_id = in.request_id;
        out.cmd = metax::auth_data;
        out.uuid = u.uuid;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_no_permissions()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::no_permissions == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        post_ims_no_permissions(m_requests[in.request_id]);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_auth_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::auth == in.cmd);
        auto uit = m_save_units.find(in.request_id);
        if (m_save_units.end() != uit) {
                return handle_update_auth_request(uit->second);
        }
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        r.winners_count += in.winners_count;
        poco_assert((request::requested == r.st && metax::get == r.cmd)
                || metax::update == r.cmd || metax::share == r.cmd
                || metax::del == r.cmd || metax::copy == r.cmd);
        r.st = request::authentication;
        /// Process authorization message which is in.message
        kernel_router_package& out = *router_tx;
        /// Write auth_data
        /// out = authenticatin_data;
        out = metax::update == r.cmd || metax::copy == r.cmd ?
                r.data : out; // send content for update and copy
        out.request_id = in.request_id;
        out.cmd = metax::auth_data;
        out.uuid = r.uuid;
        router_tx.commit();
        /// TODO: Check whether this is valid
        r.st = request::receiving;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_no_permission()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        kernel_router_package& out = *router_tx;
        out.request_id = in.request_id;
        out.cmd = metax::no_permissions;
        out.uuid = in.uuid;
        router_tx.commit();
        m_requests.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
is_external_request_authorized(const kernel_router_package&) const
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return true;
}

void leviathan::metax::kernel::
handle_router_authorized_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::auth_data == in.cmd);
        poco_assert(is_external_request_authorized(in));
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        poco_assert(metax::max_command != r.cmd);
        switch (r.cmd) {
                case metax::update: {
                        r.data = in;
                        post_storage_save_data_request(r);
                        break;
                } case metax::get: {
                        handle_authorized_get_request(r);
                        break;
                } case metax::del: {
                        post_storage_delete_request(r);
                        break;
                } case metax::copy: {
                        post_storage_copy_request(r, in);
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_auth_data()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::auth_data == in.cmd);
        METAX_INFO("Received auth data from router for " + in.uuid.toString());
        if (is_external_request_authorized(in)) {
                handle_router_authorized_request();
        } else {
                post_router_no_permission();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_delete_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::del == in.cmd);
        METAX_INFO("Received delete request for uuid=" + in.uuid.toString());
        bool response_needed = false;
        if (m_save_storage->look_up_data_in_db(in.uuid, false)) {
                mark_uuid_as_deleted_in_device_json(in.uuid,
                                        in.data_version, in.last_updated);
                notify_update(in.uuid, metax::del, in.data_version);
                response_needed = true;
        }
        std::thread t(&storage::delete_data_async_for_router, m_get_storage,
                                in.request_id, in.uuid, response_needed);
        t.detach();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_deleted()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::deleted == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        poco_assert(metax::del == r.cmd);
        METAX_INFO("Deleted " + r.uuid.toString() + " from peer");
        if (request::t_ims == r.typ) {
                handle_deleted_response_for_ims_request(r);
        } else {
                poco_assert(request::t_kernel == r.typ);
                handle_deleted_response_for_kernel_request(r);
        }
        ++r.updated;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_register()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        if (0 == in.size) {
                kernel_router_package& out = *router_tx;
                out.request_id = in.request_id;
                out.uuid = in.uuid;
                out.set_payload(std::string(" Invalid key."));
                out.cmd = metax::on_update_register_fail;
                router_tx.commit();
                post_router_clear_request(in.request_id);
        } else {
                ID32 id = in.request_id;
                METAX_DEBUG("Registering remote peer on update of uuid: "
                                + in.uuid.toString());
                request& r = m_requests[id];
                r = request(id, in.uuid, request::lookup_in_db,
                                in.cmd, request::t_router);
                r.data =  in;
                post_storage_look_up_request(r, false);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_register_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::on_update_register_fail == in.cmd);
        auto i = m_requests.find(in.request_id);
        if (m_requests.end() == i) {
                METAX_WARNING("Received fail response for deleted register"
                                " on update request.");
        } else {
                if (m_user_json_uuid == i->second.uuid) {
                        METAX_WARNING("Failed to register on update of user"
                                        "json " + m_user_json_uuid.toString());
                        m_requests.erase(i);
                } else if (0 == in.size) {
                        handle_failed_on_update_register_request(i->second);
                } else {
                        poco_assert(nullptr != in.message.get());
                        handle_failed_on_update_register_request(i->second,
                                        in.message.get());
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_unregister()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        METAX_DEBUG("Unregistering remote peer on update of uuid: "
                                                        + in.uuid.toString());
        if (0 == in.size) {
                kernel_router_package& out = *router_tx;
                out.request_id = in.request_id;
                out.uuid = in.uuid;
                out.set_payload(std::string(" Invalid key."));
                out.cmd = metax::on_update_unregister_fail;
                router_tx.commit();
                post_router_clear_request(in.request_id);
        } else {
                process_router_on_update_unregister(in);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_router_on_update_unregister(const kernel_router_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != in.message.get());
        bool b = remove_update_listener(in.uuid.toString(),
                        std::string(in.message.get(), in.size));
        if (b) {
                (*router_tx).cmd = metax::on_update_unregistered;
        } else {
                METAX_INFO("The peer not registered for uuid: "
                                + in.uuid.toString());
                (*router_tx).cmd = metax::refuse;
                (*router_tx) = (platform::default_package)in;
        }
        (*router_tx).request_id = in.request_id;
        (*router_tx).uuid = in.uuid;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_unregistered()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::on_update_unregistered == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        if (m_ims_req_ids.end() == m_ims_req_ids.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        send_ims_on_update_unregister_response(r);
        post_router_clear_request(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_unregister_fail()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::on_update_unregister_fail == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        post_router_clear_request(r.request_id);
        poco_assert(nullptr != in.message.get());
        std::string err = "Failed to unregister on update of "
                        + r.uuid.toString() + "." + in.message.get();
        handle_failed_on_update_unregister_request(r, err);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_stream_auth_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::stream_auth == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        if (metax::get != r.cmd) {
                return;
        }
        r.cmd = metax::get_livestream_content;
        Poco::JSON::Object::Ptr p = get_master_info_from_user_json(r.uuid);
        if (! p.isNull()) {
                get_keys_from_master_info(r, p);
        }
        if (! r.aes_key.empty() && ! r.aes_iv.empty()) {
                post_key_manager_prepare_cipher_for_stream(r);
        }
        // Do not send auth_data request in simplified protocol.
        //kernel_router_package& out = *router_tx;
        //out.request_id = r.request_id;
        //out.cmd = metax::auth_data;
        //out.uuid = r.uuid;
        //router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_on_update_registered()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::on_update_registered == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        if (m_ims_req_ids.end() == m_ims_req_ids.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        post_router_clear_request(r.request_id);
        if (m_user_json_uuid == r.uuid) {
                METAX_DEBUG("Registeried on update of user json " +
                                r.uuid.toString());
                m_requests.erase(r.request_id);
        } else {
                send_ims_on_update_register_response(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_authorized_get_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        poco_assert(metax::get == r.cmd);
        auto li = m_hosted_livestreams.find(r.uuid);
        if (m_hosted_livestreams.end() != li) {
                li->second->listeners.push_back(r.request_id);
        } else {
                post_storage_get_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_get_livestream_content()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::get_livestream_content == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        poco_assert(metax::get_livestream_content == r.cmd);
        if (! r.aes_key.empty() && ! r.aes_iv.empty()) {
                post_key_manager_stream_decrypt_request(r, in);
        } else {
                send_livestream_content_to_ims(r.uuid, in);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
clean_up_request_from_livestream_listeners(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get == r.cmd);
        auto li = m_hosted_livestreams.find(r.uuid);
        if (m_hosted_livestreams.end() != li) {
                auto& linfo = li->second;
                poco_assert(nullptr != linfo);
                std::vector<ID32>& listeners = linfo->listeners;
                auto i = std::find(listeners.begin(), listeners.end(),
                                r.request_id);
                if (listeners.end() != i) {
                        poco_assert(request::t_router == r.typ);
                        post_router_clear_request(r.request_id);
                        m_requests.erase(r.request_id);
                        listeners.erase(i);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_cancel_livestream_get()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::cancel_livestream_get == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        clean_up_request_from_livestream_listeners(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_recording_started()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::recording_started == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        send_ims_recording_started_response(r, in.uuid);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_recording_started_response(request& r, const Poco::UUID& u)
{
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        ID32 ims_id = m_ims_req_ids[r.request_id];
        out.request_id = ims_id;
        out.cmd = metax::recording_started;
        out.uuid = u;
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_recording_stopped()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::recording_stopped == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[r.request_id];
        out.cmd = metax::recording_stopped;
        out.uuid = r.uuid;
        out.rec_uuid.parse(std::string(r.data.message.get(), r.data.size));
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_recording_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        poco_assert(metax::recording_fail == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        handle_recording_fail(r, in);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_update_fail_response(request& req, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == req.cmd);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ims_kernel_package& out = *ims_tx;
        std::string m = "Failed updating uuid=" + req.uuid.toString() + '.';
        if (! err.empty()) {
                m += ' ' + err;
        }
        out.set_payload(m);
        out.cmd = metax::update_fail;
        out.uuid = req.uuid;
        out.request_id = m_ims_req_ids[req.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_save_fail_response(request& r, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        out.request_id = m_ims_req_ids[r.request_id];
        out.uuid = r.uuid;
        out.cmd = metax::save_fail;
        out.set_payload(err);
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        switch (req.typ) {
                case request::t_kernel: {
                        handle_failed_kernel_get_request(req);
                        break;
                } case request::t_ims: {
                        handle_failed_ims_get_request(req);
                        break;
                } default: {
                        poco_assert(!"shouldn't happen");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_kernel_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(req.typ == request::t_kernel);
        auto i = m_requests.find(req.parent_req_id);
        if (m_requests.end() != i) {
                std::string m = "Getting file failed: uuid=";
                m += i->second.uuid.toString() +
                        ". There is a missing chunk in the requested"
                        " object: chunk_uuid = " + req.uuid.toString();
                ims_kernel_package pkg;
                pkg.cmd = metax::get_fail;
                pkg.set_payload(m);
                pkg.uuid = i->second.uuid;
                send_ims_response(pkg);
        } else {
                METAX_WARNING("The parent request has been failed."
                                + req.uuid.toString());
        }
        reduce_router_get_input_size(req.size);
        process_router_pending_get_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_ims_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(req.typ == request::t_ims);
        auto i = m_requests.find(req.request_id);
        if (m_requests.end() != i) {
                std::string m = "Getting file failed: uuid=";
                m += req.uuid.toString() + ".";
                if (0 != req.data.message.get()) {
                        m += ' ';
                        m += req.data.message.get();
                }
                ims_kernel_package pkg;
                pkg.cmd = metax::get_fail;
                pkg.set_payload(m);
                pkg.uuid =i->second.uuid;
                send_ims_response(pkg);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_share_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ims_kernel_package& out = *ims_tx;
        std::string f = "Fail to share uuid=";
        out.set_payload(f + req.uuid.toString());
        out.cmd = metax::share_fail;
        out.uuid = req.uuid;
        out.request_id = m_ims_req_ids[req.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_send_to_peer_request(ID32 id)
{
        METAX_TRACE(__FUNCTION__);
        auto i = m_requests.find(id);
        if (m_requests.end() != i && request::t_ims == (i->second).typ) {
                (*ims_tx).cmd = metax::send_to_fail;
                std::string s = "Failed to deliver data to peer";
                (*ims_tx).set_payload(s);
                poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
                (*ims_tx).request_id = m_ims_req_ids[id];
                ims_tx.commit();
                m_ims_req_ids.erase(id);
                m_requests.erase(id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_on_update_register_request(request& r, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        (*ims_tx).cmd = metax::on_update_register_fail;
        (*ims_tx).set_payload(err);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        (*ims_tx).request_id = m_ims_req_ids[r.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_on_update_unregister_request(request& r, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::on_update_unregister_fail;
        out.set_payload(err);
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(r.request_id));
        out.request_id = m_ims_req_ids[r.request_id];
        out.uuid = r.uuid;
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_sync_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get_journal_info == req.cmd);
        if (nullptr == req.master_json) {
                METAX_WARNING("Failed to sync storage");
        } else {
                std::vector<std::string> k;
                req.master_json->getNames(k);
                std::string s = Poco::cat(std::string(","), k.begin(), k.end());
                METAX_WARNING("Failed sync process for the following uuids: "
                                + s);
        }
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_delete_failed_response(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::del == req.cmd);
        poco_assert(request::t_ims == req.typ);
        auto i = m_ims_req_ids.find(req.request_id);
        if (m_ims_req_ids.end() == i) {
                return;
        }
        ims_kernel_package& out = (*ims_tx);
        out.request_id = i->second;
        out.cmd = metax::delete_fail;
        out.uuid = req.uuid;
        out.set_payload("Failed to delete " + req.uuid.toString() + " uuid.");
        ims_tx.commit();
        m_ims_req_ids.erase(i);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_delete_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::del == req.cmd);
        if (0 != req.updated) {
                std::string t = req.updated == req.winners_count
                        ? "Deleted all " : "Deleted only ";
                using U = platform::utils;
                METAX_INFO(t + U::to_string(req.updated) + " copies of "
                                + req.uuid.toString());
        }
        if (request::t_ims == req.typ) {
                if (request::requested == req.st) {
                        auto i = m_requests.find(req.parent_req_id);
                        poco_assert(m_requests.end() != i);
                        send_ims_delete_failed_response(i->second);
                        m_requests.erase(i);
                } else {
                        send_ims_delete_failed_response(req);
                }
        }
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_ims_update_request(request& req, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::update == req.cmd);
        poco_assert(request::t_ims == req.typ);
        poco_assert(request::requested == req.st);
        auto i = m_requests.find(req.parent_req_id);
        poco_assert(m_requests.end() != i);
        send_ims_update_fail_response(i->second, "UUID is not found.");
        m_requests.erase(req.request_id);
        m_requests.erase(i);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_router != req.typ);
        switch (req.cmd) {
                case metax::get: {
                        handle_failed_get_request(req);
                        break;
                } case metax::send_to_peer: {
                        handle_failed_send_to_peer_request(req.request_id);
                        break;
                } case metax::on_update_register: {
                        handle_failed_on_update_register_request(req);
                        break;
                } case metax::on_update_unregister: {
                        handle_failed_on_update_unregister_request(req,
                              "Listener not found for: " + req.uuid.toString());
                        break;
                } case metax::copy: {
                        handle_failed_copy_request(req);
                        break;
                } case metax::del: {
                        handle_failed_delete_request(req);
                        break;
                } case metax::get_journal_info: {
                        handle_failed_sync_request(req);
                        break;
                } case metax::update: {
                        send_ims_update_fail_response(req,
                                nullptr == req.data.message.get() ?
                                "" : req.data.message.get());
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
        print_debug_info();
        process_pending_save_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_kernel_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == req.typ);
        if ((m_requests.end() == m_requests.find(req.parent_req_id)
                                && metax::del != req.cmd)
                        || metax::get_journal_info == req.cmd) {
                handle_failed_request(req);
        } else {
                handle_canceled_ims_request(req);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_ims_save_stream_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        poco_assert(metax::save_stream == req.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ID32 ims_id = m_ims_req_ids[req.request_id];
        out.request_id = ims_id;
        out.cmd = metax::save_stream_fail;
        out.set_payload(std::string("Failed to save stream"));
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_stream_req_ids.erase(ims_id);
        m_requests.erase(req.request_id);
        //reduce_router_save_input_size();
        process_pending_save_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_recording_fail(request& req, platform::default_package m)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id));
        ID32 ims_id = m_ims_req_ids[req.request_id];
        out.request_id = ims_id;
        out.cmd = metax::recording_fail;
        out = m;
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_stream_req_ids.erase(ims_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_ims_recording_start(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::recording_start == req.cmd);
        handle_recording_fail(req, "livestream not found: "
                                + req.uuid.toString());
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_ims_recording_stop(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::recording_stop == req.cmd);
        handle_recording_fail(req, std::string("livestream not found: "
                                + req.uuid.toString()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_get_livestream_fail_to_ims(const UUID& uuid,
                platform::default_package pkg)
{
        METAX_TRACE(__FUNCTION__);
        auto it = m_pending_ims_get_requests.find(uuid);
        if (m_pending_ims_get_requests.end() == it) {
                return;
        }
        for (ID32 i : it->second) {
                ims_kernel_package& out = *ims_tx;
                poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(i));
                out.request_id = m_ims_req_ids[i];
                out.cmd = metax::get_livestream_fail;
                out = pkg;
                ims_tx.commit();
                m_ims_req_ids.erase(i);
                m_requests.erase(i);
        }
        m_pending_ims_get_requests.erase(it);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}


void leviathan::metax::kernel::
handle_canceled_ims_get_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ || request::t_kernel == req.typ);
        poco_assert(req.fail_count < max_get_fail_count);
        req.st = request::lookup_in_db;
        post_router_get_request(req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_ims_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        if (metax::send_to_peer == req.cmd || metax::copy == req.cmd
                        || metax::on_update_register == req.cmd
                        || metax::on_update_unregister == req.cmd
                        || metax::del == req.cmd
                        || (metax::get == req.cmd
                        && req.fail_count >= max_get_fail_count)) {
                return handle_failed_request(req);
        }
        switch (req.cmd) {
                case metax::get: {
                        handle_canceled_ims_get_request(req);
                        break;
                } case metax::update: {
                        handle_failed_ims_update_request(req,
                                        "UUID is not found.");
                        break;
                } case metax::save_stream: {
                        handle_canceled_ims_save_stream_request(req);
                        break;
                } case metax::livestream_content:
                  case metax::get_livestream_content: {
                        send_get_livestream_fail_to_ims(req.uuid,
                                        std::string("Livestream is finished"));
                        break;
                } case metax::recording_start: {
                        handle_canceled_ims_recording_start(req);
                        break;
                } case metax::recording_stop: {
                        handle_canceled_ims_recording_stop(req);
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
clean_up_hosted_livestream_listeners(std::vector<ID32>& listeners)
{
        METAX_TRACE(__FUNCTION__);
        for (auto i : listeners) {
                auto lri = m_requests.find(i);
                if (m_requests.end() != lri) {
                        poco_assert(request::t_router == lri->second.typ);
                        post_router_clear_request(i);
                        m_requests.erase(i);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_livestream_chunk_to_all_recordings(livestream_info& li)
{
        METAX_TRACE(__FUNCTION__);
        if (li.recordings.empty()) {
                return;
        }
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        Poco::UUID uuid = g.createRandom();
        post_storage_save_request(li.rec_chunk, uuid);
        METAX_INFO("saving livestream chunk for recording: " + uuid.toString());
        for (auto ri = li.recordings.begin();
                        ri != li.recordings.end(); ++ri) {
                add_livestream_chunk_to_recording(li, ri, uuid);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
add_livestream_chunk_to_recording(livestream_info& li,
                std::vector<recording_info>::iterator ri,
                const Poco::UUID& uuid)
{
        using O = Poco::JSON::Object;
        Poco::UInt64 s =
                ri->master->getValue<Poco::UInt64>("size");
        ri->master->set("size", s + li.rec_chunk_size);
        O::Ptr p(new Poco::JSON::Object);
        p->set("from", s);
        p->set("size", li.rec_chunk_size);
        O::Ptr ps = ri->master->getObject("pieces");
        ps->set(uuid.toString(), p);
        std::ostringstream ossm;
        ri->master->stringify(ossm);
        post_storage_save_request(ossm.str(),
                        ri->playlist->getValue<std::string>("recording"));
        METAX_INFO("appending livestream chunk to recording "
                        + ri->uuid.toString());
        ri->playlist->set("duration", ri->start_time.elapsed() / 1000000);
        std::ostringstream ossp;
        ri->playlist->stringify(ossp);
        post_storage_save_request(ossp.str(), ri->uuid);
}

void leviathan::metax::kernel::
clean_up_hosted_livestream(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save_stream == req.cmd);
        auto li = m_hosted_livestreams.find(req.uuid);
        poco_assert(m_hosted_livestreams.end() != li);
        auto& linfo = li->second;
        poco_assert(nullptr != linfo);
        poco_assert(req.request_id == linfo->request_id);
        send_get_livestream_fail_to_ims(req.uuid,
                        std::string("Livestream is finished"));
        clean_up_hosted_livestream_listeners(linfo->listeners);
        add_livestream_chunk_to_all_recordings(*linfo);
        m_hosted_livestreams.erase(li);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_copy_failed_response(request& req, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == req.typ);
        METAX_INFO("Send copy failed response to ims for "
                        + req.uuid.toString() + " uuid");
        if (m_ims_req_ids.end() != m_ims_req_ids.find(req.request_id)) {
                ims_kernel_package& out = *ims_tx;
                out.request_id = m_ims_req_ids[req.request_id];
                out.cmd = metax::copy_failed;
                out.uuid = req.uuid;
                out.set_payload(err);
                ims_tx.commit();
                m_ims_req_ids.erase(req.request_id);
        }
        for (auto& i : req.saved_units) {
                handle_piece_delete(Poco::UUID(i), req.request_id);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_kernel_copy_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_kernel == req.typ);
        poco_assert(m_requests.end() != m_requests.find(req.parent_req_id));
        request& preq = m_requests[req.parent_req_id];
        if (0 == req.updated) {
                METAX_INFO("Failed to copy " + req.uuid.toString() + " uuid");
                send_ims_copy_failed_response(preq, "Failed to copy.");
        }
        remove_from_backup_units(preq, req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_ims_copy_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        if (0 == req.updated) {
                return send_ims_copy_failed_response(req, "Failed to copy.");
        }
        remove_from_backup_units(req, req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_copy_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::copy == req.cmd);
        poco_assert(request::t_kernel == req.typ || request::t_ims == req.typ);
        METAX_INFO("Cleaning up copy request for " + req.uuid.toString()
                        + " uuid (copy count = " +
                        platform::utils::to_string(req.updated) + ")");
        if (request::t_kernel == req.typ) {
                handle_failed_kernel_copy_request(req);
        } else {
                poco_assert(request::t_ims == req.typ);
                if (request::requested == req.st) {
                        auto i = m_requests.find(req.parent_req_id);
                        poco_assert(m_requests.end() != i);
                        m_requests.erase(req.request_id);
                        send_ims_copy_failed_response(i->second,
                                        "Failed to copy.");  
                        remove_from_backup_units(i->second,
                                        i->second.request_id);
                } else {
                        handle_failed_ims_copy_request(req);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_save_unit(save_unit& unit, ID32 uid, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        ++unit.fail_count;
        if (max_save_fail_count == unit.fail_count) {
                handle_failed_save_unit(unit, uid, err);
        } else {
                (*router_tx).cmd = unit.cmd;
                (*router_tx).request_id = uid;
                (*router_tx).uuid = unit.uuid;
                router_tx.commit();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::ID32 leviathan::metax::kernel::
handle_piece_delete(const Poco::UUID& uuid, ID32 preq_id)
{
        METAX_TRACE(__FUNCTION__);
        ID32 id = get_request_id();
        request& d = m_requests[id];
        d = request(id, uuid, request::requested,
                        metax::del, request::t_kernel);
        d.parent_req_id = preq_id;
        ++d.data_version;
        post_storage_delete_request(d);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return id;
}

void leviathan::metax::kernel::
process_not_backed_up_save_unit(request& r, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        for (auto i : r.units) {
                auto ui = m_save_units.find(i);
                poco_assert(m_save_units.end() != ui);
                if (ui->second.backup) {
                        reduce_router_save_input_size(ui->second.size);
                }
                m_save_units.erase(ui);
        }
        for (auto& i : r.saved_units) {
                handle_piece_delete(Poco::UUID(i), r.request_id);
        }
        if (metax::update == r.cmd) {
                send_ims_update_fail_response(r,
                                "Could not find uuid for update");
        } else {
                send_ims_save_fail_response(r, err);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_failed_save_unit(save_unit& unit, ID32 uid, const std::string& err)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != unit.main_request);
        request& r = *unit.main_request;
        poco_assert(metax::save == r.cmd || metax::update == r.cmd);
        if (0 == unit.bkp_count) {
                process_not_backed_up_save_unit(r, err);
        } else {
                METAX_INFO("Backup of uuid=" + unit.uuid.toString()
                                + " is failed, copy count - "
                                + platform::utils::to_string(unit.bkp_count));
                update_copy_count_of_owned_uuid_in_user_json(unit.uuid,
                                unit.bkp_count);
                ID32 i = save_unit::master == unit.type ? r.request_id : uid;
                remove_from_backup_units(r, i);
                reduce_router_save_input_size(unit.size);
                m_save_units.erase(uid);
        }
        print_debug_info();
        process_pending_save_requests();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_canceled_router_request(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_router == req.typ);
        switch (req.cmd) {
                case metax::save_stream: {
                        clean_up_hosted_livestream(req);
                        break;
                } case metax::livestream_content: {
                        clean_up_request_from_livestream_listeners(req);
                        break;
                } default: {
                        m_requests.erase(req.request_id);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_cancel()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_router_package& in = *router_rx;
        auto j = m_save_units.find(in.request_id);
        if (m_save_units.end() != j) {
                 return handle_canceled_save_unit(j->second, in.request_id,
                                 "Failed to save.");
        }
        // TODO - cleanup function calls for save request.
        if (m_requests.end() == m_requests.find(in.request_id)) {
                METAX_DEBUG("request NOT found: " +
                                platform::utils::to_string(in.request_id));
                return;
        }
        request& req = m_requests[in.request_id];
        METAX_DEBUG("Canceled for uuid=" + req.uuid.toString());
        ++req.fail_count;
        switch (req.typ) {
                case request::t_ims: {
                        handle_canceled_ims_request(req);
                        break;
                } case request::t_kernel: {
                        handle_canceled_kernel_request(req);
                        break;
                } case request::t_router: {
                        handle_canceled_router_request(req);
                        break;
                } case request::t_user_manager: {
                        handle_canceled_user_manager_request(req);
                        break;
                } default: {
                        METAX_ERROR("This case should not be happened !!!");
                }
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_input()
{
        if (! router_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                METAX_INFO("available data count: " +
                        platform::utils::to_string(router_rx.data_count()));
                process_router_input();
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
        router_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_router_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(router_rx.has_data());
        const kernel_router_package& in = *router_rx;
        switch (in.cmd) {
                case metax::get: {
                        handle_router_get_request();
                        break;
                } case metax::get_data: {
                        handle_router_get_data();
                        break;
                } case metax::notify_update:
                  case metax::notify_delete:
                  case metax::find_peer: {
                        handle_router_find_peer();
                        break;
                } case metax::update: {
                        handle_router_update_request();
                        break;
                } case metax::save: {
                        handle_router_save_request();
                        break;
                } case metax::save_stream: {
                        handle_router_save_stream_request();
                        break;
                } case metax::recording_start: {
                        handle_router_recording_start();
                        break;
                } case metax::recording_stop: {
                        handle_router_recording_stop();
                        break;
                } case metax::save_data: {
                        handle_router_save_data();
                        break;
                } case metax::auth: {
                        handle_router_auth_request();
                        break;
                } case metax::auth_data: {
                        handle_router_auth_data();
                        break;
                } case metax::del: {
                        handle_router_delete_request();
                        break;
                } case metax::no_permissions: {
                        handle_router_no_permissions();
                        break;
                } case metax::cancel: {
                        handle_router_cancel();
                        break;
                } case metax::save_offer: {
                        handle_router_save_offer();
                        break;
                } case metax::save_stream_offer: {
                        handle_router_save_stream_offer();
                        break;
                } case metax::livestream_content: {
                        handle_router_livestream_content();
                        break;
                } case metax::updated:
                  case metax::save_confirm: {
                        handle_router_save_confirm();
                        break;
                } case metax::peer_found: {
                        handle_router_peer_found(in.request_id);
                        break;
                } case metax::send_to_peer: {
                        handle_router_send_to_peer(in.request_id);
                        break;
                } case metax::send_to_peer_confirm: {
                        handle_router_send_to_peer_confirm(in.request_id);
                        break;
                } case metax::deliver_fail: {
                        handle_router_failed();
                        break;
                } case metax::deleted: {
                        handle_router_deleted();
                        break;
                } case metax::on_update_register: {
                        handle_router_on_update_register();
                        break;
                } case metax::on_update_register_fail: {
                        handle_router_on_update_register_fail();
                        break;
                } case metax::on_update_registered: {
                        handle_router_on_update_registered();
                        break;
                } case metax::on_update_unregister: {
                        handle_router_on_update_unregister();
                        break;
                } case metax::on_update_unregistered: {
                        handle_router_on_update_unregistered();
                        break;
                } case metax::on_update_unregister_fail: {
                        handle_router_on_update_unregister_fail();
                        break;
                } case metax::stream_auth: {
                        handle_router_stream_auth_request();
                        break;
                } case metax::get_livestream_content: {
                        handle_router_get_livestream_content();
                        break;
                } case metax::cancel_livestream_get: {
                        handle_router_cancel_livestream_get();
                        break;
                } case metax::recording_started: {
                        handle_router_recording_started();
                        break;
                } case metax::recording_stopped: {
                        handle_router_recording_stopped();
                        break;
                } case metax::recording_fail: {
                        handle_router_recording_fail();
                        break;
                } case metax::copy: {
                        handle_router_copy_request();
                        break;
                } case metax::copy_succeeded: {
                        handle_router_copy_succeeded(in.request_id);
                        break;
                } case metax::send_journal_info: {
                        handle_router_send_journal_info();
                        break;
                } case metax::get_journal_info: {
                        handle_router_get_journal_info(in);
                        break;
                } case metax::send_uuids: {
                        handle_router_send_uuids_request();
                        break;
                } case metax::get_uuid: {
                        handle_router_get_uuid();
                        break;
                } case metax::start_storage_sync: {
                        handle_router_start_storage_sync();
                        break;
                } case metax::sync_finished: {
                        handle_router_sync_finished_request();
                        break;
                } case metax::connected: {
                        send_notification_to_user_manager();
                        send_notification_to_ims(get_request_id(), in);
                        break;
                  case metax::disconnected:
                        send_notification_to_ims(get_request_id(), in);
                        break;
                } default: {
                        METAX_WARNING("Command is not handled yet");
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_router_sync_finished_request()
{
        METAX_TRACE(__FUNCTION__);
        m_should_block_get_request = false;
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::sync_finished;
        ims_tx.commit();



        // TODO sync may finish earlier than m_user_json_uuid is initialized.
        // This block of code should be executed in palce where the mentioned
        // case is also covered
        /*
        ID32 id = get_request_id();
        request& r = m_requests[id];
        r = request(id, m_user_json_uuid, request::max_state,
                    metax::on_update_register, request::t_kernel);


        kernel_cfg_package& cout = *config_tx;
        cout.request_id = r.request_id;
        cout.cmd = metax::get_public_key;
        config_tx.commit();
        */


        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_kernel_cfg()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::kernel_cfg == in.cmd);
        m_storage_class = in.storage_class;
        m_sync_class = in.sync_class;
        m_enable_livstream_hosting = in.enable_livestream_hosting;
        m_data_copy_count = in.data_copy_count;
        max_router_save_input_size = in.router_save_queue_size;
        max_router_get_input_size = in.router_get_queue_size;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_send_to_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::send_to_peer == in.cmd);
        send_notification_to_ims(get_request_id(), in);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_friend_found()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_friend_found == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        poco_assert(m_requests.end() != m_requests.find(id));
        request& r = m_requests[id];
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out.set_payload(in);
        ims_tx.commit();
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_friend_not_found()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_friend_not_found == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        poco_assert(m_requests.end() != m_requests.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        ims_tx.commit();
        m_requests.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_add_trusted_peer_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::add_peer_confirm == in.cmd
                        || metax::add_peer_failed == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_remove_trusted_peer_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::remove_peer_confirm == in.cmd
                        || metax::remove_peer_failed == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_add_friend_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::add_friend_confirm == in.cmd
                        || metax::add_friend_failed == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_trusted_peer_list_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_peer_list_response == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_friend_list_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_friend_list_response == in.cmd);
        ID32 id = in.request_id;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(id));
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[id];
        out.cmd = in.cmd;
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_public_key_response()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_public_key_response == in.cmd);
        poco_assert(m_requests.end() != m_requests.find(in.request_id));
        request& r = m_requests[in.request_id];
        r.data = in;
        switch (r.cmd) {
                case metax::get_public_key: {
                        send_ims_get_public_key_response(r);
                        break;
                } case metax::on_update_register: {
                        post_router_on_update_register_request(r);
                        break;
                } case metax::on_update_unregister: {
                        post_router_on_update_unregister_request(r);
                        break;
                } default: {
                       METAX_WARNING("Unexpected request type for get_public_key response");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_get_user_keys_response(const kernel_key_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::get_user_keys_response == in.cmd
                        || metax::get_user_keys_failed == in.cmd);
        ims_kernel_package& out = *ims_tx;
        out.cmd = in.cmd;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_get_public_key_response(request& r)
{
        METAX_TRACE(__FUNCTION__);
        (*ims_tx) = r.data;
        (*ims_tx).cmd = metax::get_public_key_response;
        (*ims_tx).request_id = m_ims_req_ids[r.request_id];
        ims_tx.commit();
        m_ims_req_ids.erase(r.request_id);
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_on_update_register_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out = r.data;
        out.cmd = metax::on_update_register;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_on_update_unregister_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out = r.data;
        out.cmd = metax::on_update_unregister;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_update_listeners_path()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_cfg_package& in = *config_rx;
        poco_assert(0 != in.size);
        m_update_listeners_path.assign(in.message.get(), in.size);
        std::ifstream ifs(m_update_listeners_path);
        if (ifs.is_open()) {
                namespace P = leviathan::platform;
                m_update_listeners =
                        P::utils::parse_json<Poco::JSON::Object::Ptr>(ifs);
                ifs.close();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_user_manager_input()
{
        if (! user_manager_rx.has_data()) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_user_manager_input();
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
        user_manager_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_user_manager_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(user_manager_rx.has_data());
        const kernel_user_manager_package& in = *user_manager_rx;
        switch (in.cmd) {
                case metax::user_json: {
                        initialize_user_json();
                        break;
                } case metax::save_confirm: {
                        handle_user_info_dump_confirm(in.request_id);
                        break;
                } case metax::save:
                  case metax::update: {
                        backup_user_json();
                        break;
                } case metax::get: {
                        get_user_json_from_peer();
                        break;
                } case metax::on_update_register: {
                        handle_user_manager_on_update_register(in.uuid);
                        break;
                } case metax::key_init_fail: {
                        handle_key_init_fail();
                        break;
                } default: {
                        METAX_WARNING("This case should not be happened!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
initialize_user_json()
{
        METAX_TRACE(__FUNCTION__);
        const kernel_user_manager_package& in = *user_manager_rx;
        poco_assert(nullptr != in.message.get());
        std::string s(in.message.get(), in.size);
        using U = platform::utils;
        if (nullptr == m_user_json) {
                std::string m = R"({"event":"keys are generated"})";
                ims_kernel_package& out = *ims_tx;
                out.request_id = get_request_id();
                out.cmd = metax::send_to_peer;
                out.set_payload(m);
                ims_tx.commit();
                m_user_json_wait_counter = 0;
        }
        m_user_json = U::parse_json<Poco::JSON::Object::Ptr>(s);
        poco_assert(is_valid_user_json());
        m_user_json_uuid = in.uuid;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
backup_user_json()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(user_manager_rx.has_data());
        const kernel_user_manager_package& in = *user_manager_rx;
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested, in.cmd,
                        request::t_user_manager, in.data_version,
                        in.last_updated, 0);
        r.local = false;
        m_user_json_uuid = in.uuid;
        backup_data(id, r.uuid, r.cmd, m_data_copy_count - 1,
                        in, r.data_version, r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
get_user_json_from_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(user_manager_rx.has_data());
        const kernel_user_manager_package& in = *user_manager_rx;
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::lookup_in_db,
                        metax::get, request::t_user_manager);
        m_user_json_uuid = in.uuid;
        post_router_get_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_backup_input()
{
        if (! backup_rx.has_data()) {
                return;
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

void leviathan::metax::kernel::
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

void leviathan::metax::kernel::
process_backup_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(backup_rx.has_data());
        const kernel_backup_package& in = *backup_rx;
        switch (in.cmd) {
                case metax::get_journal_info: {
                        post_router_get_journal_info();
                        break;
                } case metax::send_uuids: {
                        handle_backup_send_uuids(in);
                        break;
                } case metax::get_uuid: {
                        handle_backup_get_uuid(in);
                        break;
                } case metax::notify_delete: {
                        // TODO is data_version meaningful here?
                        notify_update(in.uuid, metax::del, in.data_version);
                        break;
                } case metax::save_confirm: {
                        handle_user_info_dump_confirm(in.request_id);
                        break;
                } default: {
                        METAX_WARNING("This case should not be happened!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_config_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        switch (in.cmd) {
                case metax::kernel_cfg: {
                        handle_kernel_cfg();
                        break;
                } case metax::send_to_peer: {
                        handle_config_send_to_peer();
                        break;
                } case metax::get_peer_list_response: {
                        handle_config_get_trusted_peer_list_response();
                        break;
                } case metax::add_peer_failed:
                  case metax::add_peer_confirm: {
                        handle_config_add_trusted_peer_response();
                        break;
                } case metax::remove_peer_failed:
                  case metax::remove_peer_confirm: {
                        handle_config_remove_trusted_peer_response();
                        break;
                } case metax::add_friend_failed:
                  case metax::add_friend_confirm: {
                        handle_config_add_friend_response();
                        break;
                } case metax::get_friend_list_response: {
                        handle_config_get_friend_list_response();
                        break;
                } case metax::get_friend_found: {
                        handle_config_get_friend_found();
                        break;
                } case metax::get_friend_not_found: {
                        handle_config_get_friend_not_found();
                        break;
                } case metax::get_public_key_response: {
                        handle_config_get_public_key_response();
                        break;
                } case metax::update_listeners_path: {
                        //handle_config_update_listeners_path();
                        break;
                } case metax::get_online_peers_response: {
                        handle_config_get_online_peers_response();
                        break;
                } case metax::get_pairing_peers_response: {
                        handle_config_get_pairing_peers_response();
                        break;
                } case metax::start_pairing_fail:
                  case metax::get_generated_code: {
                        handle_config_get_generated_code();
                        break;
                } case metax::request_keys_fail: {
                        handle_request_keys_fail(in.request_id, in);
                        break;
                } case metax::get_metax_info_response: {
                        handle_config_get_metax_info_response();
                        break;
                } case metax::set_metax_info_ok: {
                        handle_config_set_metax_info_ok();
                        break;
                } case metax::set_metax_info_fail: {
                        handle_config_set_metax_info_fail();
                        break;
                } default: {
                        METAX_WARNING("Command not handled yet !!");
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_hosted_livestream_get_request(request& r, std::unique_ptr<livestream_info>& li)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != li);
        poco_assert(m_requests.end() != m_requests.find(li->request_id));
        request& lsr = m_requests[li->request_id];
        using U = platform::utils;
        METAX_INFO("Found hosted livestream, livestream request is: " +
                        U::to_string(lsr.request_id) + ", listener is: " +
                        U::to_string(r.request_id));
        Poco::JSON::Object::Ptr p = get_master_info_from_user_json(lsr.uuid);
        if (! p.isNull()) {
                get_keys_from_master_info(lsr, p);
        }
        if (! lsr.aes_key.empty() && ! lsr.aes_iv.empty()
                        && m_pending_ims_get_requests.end() ==
                        m_pending_ims_get_requests.find(lsr.uuid)) {
                post_key_manager_prepare_cipher_for_stream(lsr);
        }
        m_pending_ims_get_requests[r.uuid].push_back(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_copy_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::copy == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::lookup_in_db,
                        metax::copy, request::t_ims);
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        r.copy_uuid = g.createRandom();
        r.expire_date = in.expire_date;
        r.local = should_save_in_local(ims_kernel_package::DEFAULT);
        METAX_INFO("Processing copy request of : " + r.uuid.toString());
        post_storage_look_up_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_ims_get_fail_request()
{
        METAX_TRACE(__FUNCTION__);
        const ims_kernel_package& in = *ims_rx;
        ims_kernel_package& out = *ims_tx;
        out.request_id = in.request_id;
        out.uuid = in.uuid;
        std::string err = "Getting file failed. Please retry later.";
        out.set_payload(err);
        out.cmd = metax::get_fail;
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get == in.cmd);
        if (m_should_block_get_request) {
                return send_ims_get_fail_request();
        }
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::lookup_in_db,
                        metax::get, request::t_ims);
        r.get_range = in.get_range;
        r.cache = in.cache;
        METAX_INFO("Processing get request of : " + r.uuid.toString());
        auto hl = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() != hl) {
                poco_assert(nullptr != hl->second);
                process_hosted_livestream_get_request(r, hl->second);
        } else {
                process_ims_get_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_ims_get_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        auto c = m_pending_ims_get_requests.find(r.uuid);
        m_pending_ims_get_requests[r.uuid].push_back(r.request_id);
        if(m_pending_ims_get_requests.end() == c) {
                post_storage_look_up_request(r);
        } else {
                auto& l = c->second;
                poco_assert(0 != l.size());
                request& e = m_requests[l[0]];
                if (! e.streaming_state.list.isNull()) {
                        set_streaming_state(e, r);
                        send_available_chunks_to_ims(r, r.uuid);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
set_streaming_state(request& orig_req, request& new_req)
{
        METAX_TRACE(__FUNCTION__);
        new_req.streaming_state.list = orig_req.streaming_state.list;
        new_req.content_type = orig_req.content_type;
        new_req.aes_iv = orig_req.aes_iv;
        new_req.aes_key = orig_req.aes_key;
        new_req.master_json = orig_req.master_json;
        calculate_stream_current_chunk_from_range(new_req);
        process_stream_prepare(new_req);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
calculate_stream_current_chunk_from_range(request& req)
{
        METAX_TRACE(__FUNCTION__);
        normalize_streaming_range(req);
        poco_assert(nullptr != req.streaming_state.list);
        auto& v = *req.streaming_state.list;
        for (int i = 0; i < v.size(); ++i) {
                if (v[i].start < req.get_range.first &&
                        req.get_range.first < v[i].start + v[i].size) {
                        req.streaming_state.curr = i;
                }
                if (v[i].start < req.get_range.second &&
                        req.get_range.second < v[i].start + v[i].size) {
                        req.streaming_state.last = i + 1;
                        // TODO seems I can break the for loop here
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
normalize_streaming_range(request& req)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(nullptr != req.master_json);
        Poco::UInt64 l = req.master_json->getValue<Poco::UInt64>("size");
        if (-1 == req.get_range.first) {
                if (-1 == req.get_range.second) {
                        req.get_range.first = 0;
                } else {
                        req.get_range.first = l - req.get_range.second;
                }
                req.get_range.second = l - 1;
        } else {
                if (-1 == req.get_range.second) {
                        req.get_range.second = l - 1;
                } else {
                        req.get_range.second += req.get_range.first - 1;
                }
        }
        if (req.get_range.first > l) {
                req.get_range.first = 0;
        }
        if (req.get_range.second > l) {
                req.get_range.second = l - 1;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_router_pending_get_requests()
{
        METAX_TRACE(__FUNCTION__);
        while (m_router_get_input_size < max_router_get_input_size &&
                        ! m_pending_router_get_reqs.empty()) {
                ID32 i = m_pending_router_get_reqs.back();
                auto ri = m_requests.find(i);
                if (m_requests.end() != ri) {
                        request& req = ri->second;
                        m_router_get_input_size += req.size;;
                        post_router_get_request(req);
                }
                m_pending_router_get_reqs.pop_back();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_create_request()
{
        METAX_TRACE(__FUNCTION__);
        METAX_NOTICE("Create request is not handled yet!!!!");
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

uint32_t leviathan::metax::kernel::
get_piece_size(Poco::UInt64 size) const
{
        if (size < min_save_pieces * max_chunk_size) {
                uint32_t d = (min_save_pieces / 2) + 1;
                uint32_t s = (uint32_t) (size / min_save_pieces);
                return (size % min_save_pieces) < d && 0 != s ?
                        s : s + 1;
        }
        return max_chunk_size;
}

Poco::JSON::Object::Ptr
leviathan::metax::kernel::
create_piece_json(Poco::UInt64 from, uint32_t size)
{
        Poco::JSON::Object::Ptr o = new Poco::JSON::Object();
        o->set("from", from);
        o->set("size", size);
        return o;
}

Poco::JSON::Object::Ptr
leviathan::metax::kernel::
create_master_json_template(const request& r, Poco::UInt64 size,
                const std::string& ct)
{
        METAX_TRACE(__FUNCTION__);
        Poco::JSON::Object::Ptr m = new Poco::JSON::Object();
        m->set("size", size);
        m->set("version", r.data_version);
        m->set("content_type", ct);
        Poco::JSON::Object::Ptr pi = new Poco::JSON::Object();
        m->set("pieces", pi);
        m->set("last_updated", r.last_updated);
        m->set("expiration_date", r.expire_date);
        m->set("protocol_version", MASTER_VERSION);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return m;
}

void leviathan::metax::kernel::
get_piece_data_for_save(save_unit& u)
{
        METAX_TRACE(__FUNCTION__);
        platform::default_package pkg;
        poco_assert(0 != u.main_request);
        request& r = *u.main_request;
        u.content = get_piece_data_for_save(r, u.start, u.size);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::platform::default_package leviathan::metax::kernel::
get_piece_data_for_save(const request& r, uint64_t start, uint64_t size)
{
        METAX_TRACE(__FUNCTION__);
        platform::default_package pkg;
        if (r.file_path.empty()) {
                std::string p(r.data.message + (uint32_t)start, size);
                pkg.set_payload(p);
        } else {
                std::ifstream ifs(r.file_path, std::ios::binary);
                ifs.seekg((long long)start);
                pkg.resize(size);
                pkg.message = new char[size];
                ifs.read(pkg.message, size);
                ifs.close();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return pkg;
}

void leviathan::metax::kernel::
handle_piece_for_backup(ID32 i, save_unit& u, request& preq)
{
        METAX_TRACE(__FUNCTION__);
        uint32_t v = save_unit::master == u.type ? preq.data_version :
                DATA_VERSION_START;
        if (preq.local) {
                backup_data(i, u.uuid, u.cmd, m_data_copy_count - 1,
                                u.content, v, preq);
        } else {
                get_piece_data_for_save(u);
                if (preq.enc) {
                        poco_assert(! preq.aes_key.empty()
                                        && ! preq.aes_iv.empty());
                        request& mr = create_request_for_unit_encrypt(i,
                                        u.content, u.uuid, preq);
                        post_key_manager_encrypt_request(mr, u.content, "", "");
                } else {
                        backup_data(i, u.uuid, u.cmd, m_data_copy_count - 1,
                                        u.content, v, preq);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_pending_save_requests()
{
        process_storage_pending_save_requests();
        process_router_pending_save_requests();
}

void leviathan::metax::kernel::
process_router_pending_save_requests()
{
        METAX_TRACE(__FUNCTION__);
        auto i = m_pending_router_save_reqs.begin();
        while(i != m_pending_router_save_reqs.end()) {
                auto uit = m_save_units.find(*i);
                if (m_save_units.end() != uit) {
                        save_unit& u = uit->second;
                        if (m_router_save_input_size + u.size <=
                                        max_router_save_input_size) {
                                m_router_save_input_size += u.size;
                                u.backup = true;
                                poco_assert(nullptr != u.main_request);
                                handle_piece_for_backup(*i, u, *u.main_request);
                                i = m_pending_router_save_reqs.erase(i);
                        } else {
                                ++i;
                        }
                } else {
                        i = m_pending_router_save_reqs.erase(i);
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_storage_pending_save_requests()
{
        METAX_TRACE(__FUNCTION__);
        while (m_pending_router_save_reqs.size() < max_storage_input_cnt
                        && m_storage_input_cnt < max_storage_input_cnt
                        && ! m_pending_save_requests.empty()) {
                ID32 i = m_pending_save_requests.front();
                auto uit = m_save_units.find(i);
                if (m_save_units.end() != uit) {
                        save_unit& u = uit->second;
                        poco_assert(nullptr != u.main_request);
                        request& preq = *u.main_request;
                        get_piece_data_for_save(u);
                        poco_assert(save_unit::piece == u.type);
                        poco_assert(preq.local);
                        ++m_storage_input_cnt;
                        handle_piece_save(i, u.content, u.uuid, preq);
                }
                m_pending_save_requests.pop_front();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
should_save_in_local(ims_kernel_package::save_option l) const
{
        METAX_TRACE(__FUNCTION__);
        switch (l) {
                case ims_kernel_package::DEFAULT: {
                        return 1 <= m_storage_class;
                } case ims_kernel_package::NO_LOCAL: {
                        return false;
                } case ims_kernel_package::LOCAL: {
                        return true;
                } default: {
                        METAX_WARNING("Invalid save type. Forwarding to default");
                        return 1 <= m_storage_class;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_save_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::save == in.cmd);
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        UUID uuid = g.createRandom();
        METAX_NOTICE("Processing save request for "
                        + uuid.toString() + " uuid");
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, uuid, request::requested, metax::save, request::t_ims);
        r.data = in;
        r.file_path = in.file_path;
        r.enc = in.enc;
        r.local = should_save_in_local(in.local);
        r.content_type = in.content_type;
        r.expire_date = in.expire_date;
        process_ims_save_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_save_stream_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::save_stream == in.cmd);
        UUID uuid = in.uuid;
        if (uuid.isNull()) {
                uuid = Poco::UUIDGenerator::defaultGenerator().createRandom();
        }
        std::string s = uuid.toString();
        METAX_NOTICE("Process save stream for " + s + " uuid");
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        m_stream_req_ids[in.request_id] = id;
        request& r = m_requests[id];
        r = request(id, uuid, request::requested, metax::save_stream,
                        request::t_ims);
        r.enc = in.enc;
        r.content_type = in.content_type;
        backup_data(r.request_id, r.uuid, r.cmd, 1,
                        platform::default_package(), r.data_version, r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_livestream_content()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::livestream_content == in.cmd);
        auto i = m_stream_req_ids.find(in.request_id);
        if (m_stream_req_ids.end() == i) {
                return;
        }
        poco_assert(m_requests.end() != m_requests.find(i->second));
        request& r = m_requests[i->second];
        poco_assert(metax::save_stream == r.cmd);
        if (r.enc) {
                r.data = in;
                post_key_manager_stream_encrypt_request(r);
        } else {
                send_livestream_content_to_router(r, in, in.size);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_update_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::update == in.cmd);
        poco_assert(! in.uuid.isNull());
        METAX_NOTICE("Processing update request for "
                        + in.uuid.toString() + " uuid");
        ID32 id = get_request_id();
        request& r = m_requests[id];
        r = request(id, in.uuid,
                        request::lookup_in_db,
                        metax::update,
                        request::t_ims);
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        r.file_path = in.file_path;
        r.data.set_payload(in);
        // Reset to false if missing in local storage.
        r.local = true;
        r.enc = in.enc;
        r.content_type = in.content_type;
        r.expire_date = in.expire_date;
        post_storage_look_up_request(r, true);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_accept_share_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::accept_share == in.cmd);
        ims_kernel_package& out = *ims_tx;
        if (in.uuid.isNull()) {
                out.cmd = metax::accept_share_fail;
                out.request_id = in.request_id;
                out.set_payload("Invalid uuid");
                ims_tx.commit();
        } else {
                process_ims_accept_share_request();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_reconnect_to_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::reconnect_to_peers == in.cmd);
        kernel_cfg_package& out = *config_tx;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_start_pairing()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::start_pairing == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out = (platform::default_package)in;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_cancel_pairing()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::cancel_pairing == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_pairing_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_pairing_peers == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_request_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::request_keys == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out = (platform::default_package)in;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_recording_start()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        METAX_INFO("Porcessing recording start for livestream: "
                        + in.uuid.toString());
        poco_assert(metax::recording_start == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested,
                        metax::recording_start, request::t_ims);
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() == li) {
                post_router_recording_start(r);
        } else {
                create_and_save_master_for_recording(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_recording_start(request& r)
{
        METAX_TRACE(__FUNCTION__);
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out.cmd = r.cmd;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_router_recording_stop()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        ID32 id = get_request_id();
        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested,
                        metax::recording_stop, request::t_ims);
        r.data.set_payload(in.rec_uuid.toString());
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_router_package& out = *router_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out = r.data;
        out.cmd = r.cmd;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
create_and_save_master_for_recording(request& r)
{
        METAX_TRACE(__FUNCTION__);
        Poco::UUID lu = r.uuid;
        r.content_type = "video/mls";
        r.master_json = create_master_json_template(r, 0, r.content_type);
        r.master_json->set("livestream", lu.toString());
        std::stringstream ss;
        r.master_json->stringify(ss);
        r.data.set_payload(ss.str());
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        UUID uuid = g.createRandom();
        METAX_INFO("Saving recording: " + uuid.toString());
        // TODO consider changing the below call with the following two
        // commente out calls.  They will will take care also for
        // backup of the recordings, which the current one doesn't
        post_storage_save_data_request(r, uuid);
        //r.local = should_save_in_local(ims_kernel_package::DEFAULT);
        //save_master_object(r, platform::default_package(ss.str()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
create_and_save_playlist_for_recording(request& req, recording_info& r,
                                                const Poco::UUID& rec_uuid)
{
        METAX_TRACE(__FUNCTION__);
        r.playlist = create_master_json_template(req, 0, "video/mpl");
        r.playlist->set("recording", rec_uuid.toString());
        r.playlist->set("duration", 0);
        std::stringstream ss;
        r.playlist->stringify(ss);
        Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
        UUID uuid = g.createRandom();
        METAX_INFO("Saving playlist: " + uuid.toString());
        // TODO consider changing the below call with the following two
        // commente out calls.  They will will take care also for
        // backup of the recordings, which the current one doesn't
        post_storage_save_request(ss.str(), uuid);
        r.uuid = uuid;
        //r.local = should_save_in_local(ims_kernel_package::DEFAULT);
        //save_master_object(r, platform::default_package(ss.str()));
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_recording_stop()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::recording_stop == in.cmd);
        auto li = m_hosted_livestreams.find(in.uuid);
        if (m_hosted_livestreams.end() == li) {
                METAX_INFO("livestream not found: " + in.uuid.toString());
                post_router_recording_stop();
        } else {
                poco_assert(nullptr != li->second);
                stop_recording_and_send_response_to_ims(*li->second);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_dump_user_info_succeeded_response_to_ims(request& req)
{
        if (m_ims_req_ids.end() == m_ims_req_ids.find(req.request_id)) {
                return;
        }
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        out.request_id = m_ims_req_ids[req.request_id];
        if (metax::regenerate_user_keys == req.cmd) {
                out.cmd = metax::regenerate_user_keys_succeeded;
        } else {
                out.cmd = metax::dump_user_info_succeeded;
        }
        ims_tx.commit();
        m_ims_req_ids.erase(req.request_id);
        m_requests.erase(req.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_dump_user_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::dump_user_info == in.cmd);
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(in.request_id));
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& req = m_requests[id];
        req = request(id, Poco::UUID::null(), request::requested,
                        metax::dump_user_info, request::t_ims);
        send_dump_user_json_request(id);
        send_dump_device_json_request(id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_regenerate_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::regenerate_user_keys == in.cmd);
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(in.request_id));
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& req = m_requests[id];
        req = request(id, Poco::UUID::null(), request::requested,
                        metax::regenerate_user_keys, request::t_ims);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = req.request_id;
        out.set_payload(m_user_json_uuid.toString());
        out.cmd = in.cmd;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
stop_recording_and_send_response_to_ims(livestream_info& li)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        auto ri = std::find_if(li.recordings.begin(), li.recordings.end(),
                        [&in](const recording_info& i) {
                                return i.uuid == in.rec_uuid;
                        });
        ims_kernel_package& out = *ims_tx;
        out.request_id = in.request_id;
        if (li.recordings.end() != ri) {
                METAX_INFO("Recording stopped");
                Poco::UUIDGenerator& g = Poco::UUIDGenerator::defaultGenerator();
                Poco::UUID uuid = g.createRandom();
                post_storage_save_request(li.rec_chunk, uuid);
                METAX_INFO("saving livestream chunk for recording: " + uuid.toString());
                add_livestream_chunk_to_recording(li, ri, uuid);
                li.recordings.erase(ri);
                out.cmd = metax::recording_stopped;
        } else {
                out.cmd = metax::recording_fail;
                out.set_payload("no recording " + in.rec_uuid.toString() +
                                " found for livestream " + in.uuid.toString());
        }
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_share_accept_for_public_data(request& r, ID32 ims_id)
{
        METAX_TRACE(__FUNCTION__);
        add_shared_info_in_user_json(r, "", "");
        ims_kernel_package& out = *ims_tx;
        out.cmd = metax::share_accepted;
        out.request_id = ims_id;
        ims_tx.commit();
        m_requests.erase(r.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_ims_accept_share_request()
{
        METAX_TRACE(__FUNCTION__);
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::accept_share == in.cmd);
        poco_assert(! in.uuid.isNull());
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested, metax::accept_share,
                        request::t_ims);
        if (in.aes_key.empty() && in.aes_iv.empty()){
                return handle_share_accept_for_public_data(r, in.request_id);
        }
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_key_manager_package& out = *key_manager_tx;
        out.cmd = metax::decrypt_key;
        out.request_id = id;
        out.aes_key = in.aes_key;
        out.aes_iv = in.aes_iv;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_share_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::share == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested, metax::share,
                        request::t_ims);
        Poco::JSON::Object::Ptr p = get_master_info_from_user_json(in.uuid);
        if (p.isNull()) {
                return handle_failed_share_request(r);
        }
        get_keys_from_master_info(r, p);
        process_share_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// TODO change req_id to req
void leviathan::metax::kernel::
post_find_peer_request(ID32 req_id, const platform::default_package& p)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(m_requests.end() != m_requests.find(req_id));
        // TODO - Fill tender requirements.
        (*router_tx).request_id = req_id;
        (*router_tx).cmd = metax::find_peer;
        (*router_tx) = p;
        router_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_failed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::deliver_fail == in.cmd);
        if (m_requests.end() == m_requests.find(in.request_id)) {
                return;
        }
        request& r = m_requests[in.request_id];
        if (metax::find_peer == r.cmd) {
                kernel_router_package& out = *router_tx;
                out.cmd = metax::deliver_fail;
                out.request_id = in.request_id;
                out = in;
                router_tx.commit();
        } if (r.uuid == m_user_json_uuid) {
                return;
        } else {
                METAX_WARNING("This case is not handled yet.");
        }
        m_requests.erase(in.request_id);
        post_router_clear_request(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_send_to_peer_confirmed()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::send_to_peer_confirm == in.cmd);
        auto i = m_requests.find(in.request_id);
        if (m_requests.end() == i || (i->second).uuid == m_user_json_uuid) {
                return;
        }
        poco_assert(metax::find_peer == (i->second).cmd
                        || metax::send_to_peer == (i->second).cmd
                        || metax::notify_update == (i->second).cmd
                        || metax::notify_delete == (i->second).cmd);
        post_send_to_peer_confirm(in.request_id);
        m_requests.erase(in.request_id);
        post_router_clear_request(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_send_to_peer_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::send_to_peer == in.cmd);
        ID32 id = get_request_id();

        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        poco_assert(m_requests.end() == m_requests.find(id));

        request& r = m_requests[id];
        r = request(id, in.uuid, request::requested,
                        metax::send_to_peer, request::t_ims);
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        r.data.set_payload(in);
        r.cmd = metax::send_to_peer;
        post_find_peer_request(id, in.user_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_encrypt_request(request& r, platform::default_package pkg,
                const std::string& key, const std::string& iv)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ || request::t_kernel == r.typ);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = r.request_id;
        out.cmd = metax::encrypt;
        out = pkg;
        out.aes_key = key;
        out.aes_iv = iv;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_stream_encrypt_request(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ || request::t_kernel == r.typ);
        poco_assert(metax::save_stream == r.cmd);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = r.request_id;
        out.cmd = metax::stream_encrypt;
        out.uuid = r.uuid;
        out = r.data;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_stream_decrypt_request(request& r, const platform::default_package& pkg)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::save_stream == r.cmd ||
                        metax::get_livestream_content == r.cmd);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = r.request_id;
        out.cmd = metax::stream_decrypt;
        out.uuid = r.uuid;
        out = pkg;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_gen_key_for_stream(request& r)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(request::t_ims == r.typ || request::t_kernel == r.typ);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = r.request_id;
        out.uuid = r.uuid;
        out.cmd = metax::gen_key_for_stream;
        out.aes_key = "";
        out.aes_iv = "";
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
post_key_manager_prepare_cipher_for_stream(request& h)
{
        METAX_TRACE(__FUNCTION__);
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = h.request_id;
        out.uuid = h.uuid;
        out.cmd = metax::prepare_cipher_for_stream;
        out.aes_key = h.aes_key;
        out.aes_iv = h.aes_iv;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_delete_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::del == in.cmd);
        poco_assert(! in.uuid.isNull());
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        poco_assert(m_requests.end() == m_requests.find(id));
        request& d = m_requests[id];
        d = request(id, in.uuid, request::lookup_in_db,
                        metax::del, request::t_ims);
        d.keep_chunks = in.keep_chunks;
        post_storage_look_up_request(d);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_friend()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_friend == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, in.uuid, request::lookup_in_db,
                    in.cmd, request::t_ims);
        // Post lookup in configuration manager
        // TODO: This should be changed to uuid getter function
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out.set_payload(in);
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_add_trusted_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::add_peer == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out.set_payload(in);
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_remove_trusted_peer()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::remove_peer == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out.set_payload(in);
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_add_friend()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::add_friend == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out.set_payload(in);
        out.user_id = in.user_id;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_friend_list()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_friend_list == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_user_public_key()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_public_key == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, UUID::null(), request::max_state,
                        in.cmd, request::t_ims);
        post_config_get_public_key_request(r);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_user_keys()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_user_keys == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_key_manager_package& out = *key_manager_tx;
        out.request_id = id;
        out.cmd = metax::get_user_keys;
        key_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_online_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_online_peers_response == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_pairing_peers_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_pairing_peers_response == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_generated_code()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_generated_code == in.cmd ||
                        metax::start_pairing_fail == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_manager_request_keys_response(const kernel_key_manager_package& in)
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(metax::request_keys_response == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        send_dump_device_json_request(in.request_id, in.aes_key, in.aes_iv);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_request_keys_fail(ID32 request_id, platform::default_package in)
{
        METAX_TRACE(__FUNCTION__);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(request_id));
        out.request_id = m_ims_req_ids[request_id];
        out = in;
        out.cmd = metax::request_keys_fail;
        ims_tx.commit();
        m_ims_req_ids.erase(request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_key_init_fail()
{
        METAX_TRACE(__FUNCTION__);
        std::string s = "{\"event\":\"key initialization in Metax failed\"}";
        send_notification_to_ims(get_request_id(), s);
        m_valid_keys = false;
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_get_metax_info_response()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::get_metax_info_response == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_set_metax_info_ok()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::set_metax_info_ok == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_config_set_metax_info_fail()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(config_rx.has_data());
        const kernel_cfg_package& in = *config_rx;
        poco_assert(metax::set_metax_info_fail == in.cmd);
        ims_kernel_package& out = *ims_tx;
        poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(in.request_id));
        out.request_id = m_ims_req_ids[in.request_id];
        out = in;
        out.cmd = in.cmd;
        ims_tx.commit();
        m_ims_req_ids.erase(in.request_id);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_online_peers()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_online_peers == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_metax_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_metax_info == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_set_metax_info()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::set_metax_info == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        out = (platform::default_package)in;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_get_trusted_peer_list()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::get_peer_list == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        m_ims_req_ids[id] = in.request_id;
        kernel_cfg_package& out = *config_tx;
        out.request_id = id;
        out.cmd = in.cmd;
        config_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_move_cache_to_storage()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::move_cache_to_storage == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_ims_req_ids.end() == m_ims_req_ids.find(id));
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, Poco::UUID::null(), request::max_state,
                    in.cmd, request::t_ims);
        (*cache_tx).cmd = metax::move_cache_to_storage;
        (*cache_tx).request_id = r.request_id;
        cache_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_clean_stream_request()
{
        METAX_TRACE(__FUNCTION__);
        const ims_kernel_package& in = *ims_rx;
        auto iii = std::find_if(m_ims_req_ids.begin(), m_ims_req_ids.end(),
                        [&in](const std::pair<ID32, ID32>& mo) {
                                return mo.second == in.request_id;
                        }
                      );
        if (m_ims_req_ids.end() == iii) {
                return;
        }
        auto ri = m_requests.find(iii->first);
        poco_assert(m_requests.end() != ri);
        auto p = m_pending_ims_get_requests.find((ri->second).uuid);
        poco_assert(m_pending_ims_get_requests.end() != p);
        auto& v = p->second;
        auto b = std::find(v.begin(), v.end(), iii->first);
        poco_assert(v.end() != b);
        v.erase(b);
        if (v.empty()) {
                m_pending_ims_get_requests.erase(p);
        }
        m_requests.erase(ri);
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_register_on_update_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::on_update_register == in.cmd);
        ID32 id = get_request_id();
        poco_assert(m_requests.end() == m_requests.find(id));
        m_ims_req_ids[id] = in.request_id;
        request& r = m_requests[id];
        r = request(id, in.uuid, request::max_state,
                    in.cmd, request::t_ims);
        post_storage_look_up_request(r, false);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_unregister_on_update_request()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        poco_assert(metax::on_update_unregister == in.cmd);
        bool b = remove_update_listener(in.uuid.toString(), "");
        if (b) {
                (*ims_tx).cmd = metax::on_update_unregistered;
                (*ims_tx).uuid = in.uuid;
                (*ims_tx).request_id = in.request_id;
                ims_tx.commit();
        } else {
                ID32 id = get_request_id();
                poco_assert(m_requests.end() == m_requests.find(id));
                m_ims_req_ids[id] = in.request_id;
                request& r = m_requests[id];
                r = request(id, in.uuid, request::max_state,
                                in.cmd, request::t_ims);
                post_config_get_public_key_request(r);
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
handle_ims_livestream_cancel()
{
        METAX_TRACE(__FUNCTION__);
        const ims_kernel_package& in = *ims_rx;
        auto i = m_stream_req_ids.find(in.request_id);
        if (m_stream_req_ids.end() == i) {
                return;
        }
        auto ri = m_requests.find(i->second);
        poco_assert(m_requests.end() != ri);
        request& lr = ri->second;
        post_router_clear_request(lr.request_id);
        // Do not clean m_hosted_livestreams map as for now stream save is not
        // allowed for own kernel.
        // TODO - handle this case properly.
        //clean_up_hosted_livestream(lr);
        m_ims_req_ids.erase(i->second);
        m_stream_req_ids.erase(i);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
clean_livestream_get_request_from_pendings(const UUID& uuid, ID32& rid)
{
        METAX_TRACE(__FUNCTION__);
        auto p = m_pending_ims_get_requests.find(uuid);
        poco_assert(m_pending_ims_get_requests.end() != p);
        auto& v = p->second;
        auto b = std::find(v.begin(), v.end(), rid);
        poco_assert(v.end() != b);
        METAX_DEBUG("livestream get pending requests size for the uuid " +
                        uuid.toString() + " is " +
                        platform::utils::to_string(v.size()));
        if (v.begin() == b &&  1 < v.size()) {
                auto e = v.back();
                METAX_DEBUG("canceled the first request - " +
                        platform::utils::to_string(*b) +
                        " swaping with the last - " +
                        platform::utils::to_string(e))
                poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(*b));
                poco_assert(m_ims_req_ids.end() != m_ims_req_ids.find(e));
                METAX_DEBUG("update corresponding ims requests - " +
                        platform::utils::to_string(m_ims_req_ids[*b]) +
                        " swaping with the last - " +
                        platform::utils::to_string(m_ims_req_ids[e]))
                m_ims_req_ids[*b] = m_ims_req_ids[e];
                m_requests.erase(e);
                m_ims_req_ids.erase(e);
                v.pop_back();
        } else {
                METAX_DEBUG("canceled the livestream get - " +
                        platform::utils::to_string(*b));
                rid = v.front();
                m_requests.erase(*b);
                m_ims_req_ids.erase(*b);
                v.erase(b);
        }
        bool last = false;
        if (v.empty()) {
                m_pending_ims_get_requests.erase(p);
                last = true;
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
        return last;
}

void leviathan::metax::kernel::
handle_ims_cancel_livestream_get()
{
        METAX_TRACE(__FUNCTION__);
        const ims_kernel_package& in = *ims_rx;
        METAX_DEBUG("received livestream get cancel for ims request - " +
                        platform::utils::to_string(in.request_id));
        auto i = std::find_if(m_ims_req_ids.begin(), m_ims_req_ids.end(),
                        [&in](const std::pair<ID32, ID32>& mo) {
                                return mo.second == in.request_id;
                        }
                      );
        if (m_ims_req_ids.end() == i) {
                METAX_DEBUG("livestream get not found with ims id - " +
                                platform::utils::to_string(in.request_id));
                return;
        }
        auto ri = m_requests.find(i->first);
        // Check whether the stream is already cleaned up by router.
        if (m_requests.end() == ri) {
                METAX_DEBUG("livestream get ims id is found but "
                                "the kernel request is already cleaned up - " +
                                platform::utils::to_string(i->first));
                return;
        }
        request& lr = ri->second;
        ID32 lrid = lr.request_id;
        bool last = clean_livestream_get_request_from_pendings(lr.uuid, lrid);
        auto li = m_hosted_livestreams.find(lr.uuid);
        if (m_hosted_livestreams.end() == li && last) {
                kernel_router_package& out = *router_tx;
                out.request_id = lrid;
                out.cmd = metax::cancel_livestream_get;
                router_tx.commit();
                post_router_clear_request(lrid);
        }
        print_debug_info();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_dump_user_json_request(ID32 req_id, const std::string& key,
                const std::string& iv)
{
        METAX_TRACE(__FUNCTION__);
        kernel_user_manager_package& out = *user_manager_tx;
        out.request_id = req_id;
        out.key = key;
        out.iv = iv;
        out.cmd = metax::dump_user_info;
        user_manager_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_dump_device_json_request(ID32 req_id, const std::string& key, const std::string& iv)
{
        METAX_TRACE(__FUNCTION__);
        kernel_backup_package& out  = *backup_tx;
        out.request_id = req_id;
        out.key = key;
        out.iv = iv;
        out.cmd = metax::dump_user_info;
        backup_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

bool leviathan::metax::kernel::
should_wait_for_user_json(command cmd) const
{
        return ((nullptr == m_user_json) &&
                        (metax::get == cmd
                         || metax::save == cmd
                         || metax::update == cmd
                         || metax::del == cmd
                         || metax::share == cmd
                         || metax::copy == cmd
                         || metax::dump_user_info == cmd
                         || metax::get_public_key == cmd
                         || metax::get_user_keys == cmd
                         || metax::accept_share == cmd));
}

void leviathan::metax::kernel::
handle_ims_input()
{
        if (! ims_rx.has_data()) {
                return;
        }
        if (! m_valid_keys && metax::deliver_fail != (*ims_rx).cmd
                           && metax::send_to_peer_confirm != (*ims_rx).cmd) {
                send_response_to_ims_unhandled_command();
                ims_rx.consume();
                return;
        }
        if (should_wait_for_user_json((*ims_rx).cmd)) {
                sleep(300); // millisecods
                ++m_user_json_wait_counter;
                if (600 < m_user_json_wait_counter) {
                        handle_key_init_fail();
                }
                Poco::Thread::yield();
                return;
        }
        METAX_TRACE(__FUNCTION__);
        try {
                process_ims_input();
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
        ims_rx.consume();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
process_ims_input()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        switch ((*ims_rx).cmd) {
                case metax::get: {
                        handle_ims_get_request();
                        break;
                } case metax::create: {
                        handle_create_request();
                        break;
                } case metax::save: {
                        handle_ims_save_request();
                        break;
                } case metax::copy: {
                        handle_ims_copy_request();
                        break;
                } case metax::save_stream: {
                        handle_ims_save_stream_request();
                        break;
                } case metax::livestream_content: {
                        handle_ims_livestream_content();
                        break;
                } case metax::del: {
                        handle_ims_delete_request();
                        break;
                } case metax::update: {
                        handle_ims_update_request();
                        break;
                } case metax::share: {
                        handle_ims_share_request();
                        break;
                } case metax::send_to_peer: {
                        handle_ims_send_to_peer_request();
                        break;
                } case metax::deliver_fail: {
                        handle_ims_failed();
                        break;
                } case metax::send_to_peer_confirm: {
                        handle_ims_send_to_peer_confirmed();
                        break;
                } case metax::get_friend: {
                        handle_ims_get_friend();
                        break;
                } case metax::add_peer: {
                        handle_ims_add_trusted_peer();
                        break;
                } case metax::remove_peer: {
                        handle_ims_remove_trusted_peer();
                        break;
                } case metax::get_friend_list: {
                        handle_ims_get_friend_list();
                        break;
                } case metax::add_friend: {
                        handle_ims_add_friend();
                        break;
                } case metax::get_public_key: {
                        handle_ims_get_user_public_key();
                        break;
                } case metax::get_user_keys: {
                        handle_ims_get_user_keys();
                        break;
                } case metax::get_online_peers: {
                        handle_ims_get_online_peers();
                        break;
                } case metax::get_metax_info: {
                        handle_ims_get_metax_info();
                        break;
                } case metax::set_metax_info: {
                        handle_ims_set_metax_info();
                        break;
                } case metax::get_peer_list: {
                        handle_ims_get_trusted_peer_list();
                        break;
                } case metax::move_cache_to_storage: {
                        handle_ims_move_cache_to_storage();
                        break;
                } case metax::clean_stream_request: {
                        handle_ims_clean_stream_request();
                        break;
                } case metax::on_update_register: {
                        handle_ims_register_on_update_request();
                        break;
                } case metax::on_update_unregister: {
                        handle_ims_unregister_on_update_request();
                        break;
                } case metax::livestream_cancel: {
                        handle_ims_livestream_cancel();
                        break;
                } case metax::cancel_livestream_get: {
                        handle_ims_cancel_livestream_get();
                        break;
                } case metax::accept_share: {
                        handle_ims_accept_share_request();
                        break;
                } case metax::reconnect_to_peers: {
                        handle_ims_reconnect_to_peers();
                        break;
                } case metax::start_pairing: {
                        handle_ims_start_pairing();
                        break;
                } case metax::cancel_pairing: {
                        handle_ims_cancel_pairing();
                        break;
                } case metax::get_pairing_peers: {
                        handle_ims_get_pairing_peers();
                        break;
                } case metax::request_keys: {
                        handle_ims_request_keys();
                        break;
                } case metax::recording_start: {
                        handle_ims_recording_start();
                        break;
                } case metax::recording_stop: {
                        handle_ims_recording_stop();
                        break;
                } case metax::dump_user_info: {
                        handle_ims_dump_user_info();
                        break;
                } case metax::regenerate_user_keys: {
                        handle_ims_regenerate_user_keys();
                        break;
                } default: {
                        send_response_to_ims_unhandled_command();
                        break;
                }
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
wait_for_input_channels()
{
        METAX_TRACE(__FUNCTION__);
        while (true) {
                METAX_INFO("WAITING");
                // wait to read data
                if (!wait()) {
                        break;
                }
                handle_config_input();
                handle_key_manager_input();
                handle_storage_input();
                handle_storage_writer_input();
                handle_cache_input();
                handle_wallet_input();
                handle_ims_input();
                handle_router_input();
                handle_backup_input();
                handle_user_manager_input();
        }
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
send_response_to_ims_unhandled_command()
{
        METAX_TRACE(__FUNCTION__);
        poco_assert(ims_rx.has_data());
        const ims_kernel_package& in = *ims_rx;
        METAX_NOTICE("Unhandled Command");
        (*ims_tx).cmd = metax::failed;
        (*ims_tx).request_id = in.request_id;
        ims_tx.commit();
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

void leviathan::metax::kernel::
runTask()
try {
        METAX_TRACE(__FUNCTION__);
        wait_for_input_channels();
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

leviathan::metax::kernel::
kernel(storage* g, storage* s)
        : platform::csp::task("kernel", Poco::Logger::get("metax.kernel"))
        , key_manager_rx(this)
        , storage_rx(this)
        , storage_writer_rx(this)
        , cache_rx(this)
        , wallet_rx(this)
        , ims_rx(this)
        , router_rx(this)
        , config_rx(this)
        , backup_rx(this)
        , user_manager_rx(this)
        , key_manager_tx(this)
        , storage_tx(this)
        , storage_writer_tx(this)
        , cache_tx(this)
        , wallet_tx(this)
        , ims_tx(this)
        , router_tx(this)
        , config_tx(this)
        , backup_tx(this)
        , user_manager_tx(this)
        , m_request_counter(KERNEL_REQUEST_LOWER_BOUND)
        , m_ims_req_ids()
        , m_stream_req_ids()
        , m_backup_ids()
        , m_hosted_livestreams()
        , m_save_units()
        , m_storage_input_cnt(0)
        , m_router_save_input_size(0)
        , m_router_get_input_size(0)
        , m_update_listeners(new Poco::JSON::Object)
        , m_update_listeners_path("")
        , m_user_json(nullptr)
        , m_user_json_uuid()
        , m_should_block_get_request(false)
        , m_valid_keys(true)
        , m_user_json_wait_counter(0)
        , m_get_storage(g)
        , m_save_storage(s)
        , max_get_fail_count(1)
        , max_save_fail_count(1)
        , m_data_copy_count(4)
        , min_save_pieces(3)
        , max_chunk_size(7000000) // 7M
        , max_storage_input_cnt(150)
        , max_router_save_input_size(21000000)
        , max_router_get_input_size(21000000)
        , max_get_count(2)
        , recording_max_chunk_size(1200000) // 1.2mb
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

leviathan::metax::kernel::
~kernel()
{
        METAX_TRACE(__FUNCTION__);
        METAX_TRACE(std::string("END ") + __FUNCTION__);
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:


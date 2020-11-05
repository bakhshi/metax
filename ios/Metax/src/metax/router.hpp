/**
 * @file src/metax/router.hpp
 *
 * @brief Definition of the class @ref
 * leviathan::metax::router
 *
 * COPYWRITE_TODO
 *
 */

#ifndef LEVIATHAN_METAX_ROUTER_HPP
#define LEVIATHAN_METAX_ROUTER_HPP

// Headers from this project
#include "protocols.hpp"

// Headers from other projects
#include <platform/task.hpp>
#include <platform/fifo.hpp>

// Headers from third party libraries
#include <Poco/Timer.h>
#include <Poco/Timestamp.h>

// Headers from standard libraries
#include <list>
#include <map>
#include <vector>

// Forward declarations
namespace leviathan {
        namespace metax {
                struct router;
        }
}


/**
 * @brief Performs routing logic. Receives all the request/responses from kernel
 * for sending to peers, keeps track and forwards all the requests received from
 * peers, routs the messages to the right peers if two other peers talking to
 * others through an intermediate peer.
 *
 */
struct leviathan::metax::router: 
        public leviathan::platform::csp::task
{
        /// @name Public type definitions
public:

        using INPUT = platform::csp::input<platform::default_package>;
        using OUTPUT = platform::csp::output<platform::default_package>;
        using KERNEL_RX = platform::csp::input<kernel_router_package>;
        using KERNEL_TX = platform::csp::output<kernel_router_package>;
        using CONFIG_TX = platform::csp::output<router_cfg_package>;
        using CONFIG_RX = platform::csp::input<router_cfg_package>;
        using LINK_RX = platform::csp::input<router_link_package>;
        using LINK_TX = platform::csp::output<router_link_package>;
        using STORAGE_RX = platform::csp::input<kernel_storage_package>;

        /// @name Private type definitions
private:

        /**
         * Key - request ID got from kernel or id of request sent from router to
         * kernel.
         */
        typedef std::map<ID32, UUID> request_tender_map;


        /// Described tender within router
        struct tender: leviathan::metax::router_data
        {
                enum type {
                        /**
                         * Tender is internal one i.e. created based on the
                         * kernel request
                         */
                        internal,

                        /**
                         * Tender is remote one i.e. created based on the
                         * request from remote host and local host serves the
                         * request , i.e. it is able to satisfy the request.
                         */
                        remote,

                        max_type };

                enum state {
                        /**
                         * Tender is broadcasted and waiting for responses.
                         * If the tender is announced it means it waits for the
                         * responses to choose the best one.
                         */
                        announced,

                        /**
                         * Tender winner is already confirmed by local host and
                         * negotiation/data transfer is in progress.
                         * The tender can be as internal as well as external.
                         */
                        confirmed,
                        /**
                         * Tender is received from remote host and passed to
                         * kernel for process.
                         * This state implies that tender is remote one.
                         * If local host is able to satisfy the request the
                         * succeeding state is "confirmed" otherwise
                         * "transporting".
                         */
                        received,
                        /**
                         * Tender is received from remote host and is
                         * broadcasted  and succeeding data transfer related to
                         * this request will be also transmitted to the
                         * corresponding peers.
                         */
                        transporting,

                        max_state
                };

                typedef std::set<ID32> peers;

                // Request id accociated with tender;
                ID32 request_id;

                // Type of the tender
                type typ;

                // State of the tender
                state st;

                // List of peers related to tender src/dests.
                // The order of peer list is significant during the bypass
                // as response of newly inserted peer should be bypassed after
                // bypassing the responses of previously inserted peers.
                // TODO - peers list can be std::deque instead of std::set.
                peers prs;

                // Link peer from which the tender is received.
                // Initialized during the broadcast.
                ID32 src;

                // Timestamp corresponding to tenders last activity
                Poco::Timestamp time;

                // Acceptable idling interval for the tender
                Poco::Timestamp::TimeVal idling_interval;

                // Specifies the maximum number of responses to be send to
                // kernel for the same tender.
                // Used for metax::update request to handle the responses of all
                // updated nodes by using one tender.
                unsigned short max_response_cnt;
                // Specifies the number of responses which were sent to the
                // kernel at any time.
                unsigned short received_response_cnt;

                std::vector<std::pair<ID32, ID32>> winners;

                bool is_canceled;

                void read_router_data(const router_data& r)
                {
                        router_data::operator=(r);
                }

                void write_router_data(router_data& r) const
                {
                        r.tender_id = tender_id;
                        r.uuid = uuid;
                        r.cmd = cmd;
                        r.hop_id = hop_id;
                        r.data_version = data_version;
                        r.last_updated = last_updated;
                        r.expire_date = expire_date;
                }

                bool is_expired(const Poco::Timestamp& now = Poco::Timestamp())
                        const
                {
                        return idling_interval < (now - time);
                }

                static bool less_than (tender*& t1, tender*& t2)
                {
                        Poco::Timestamp now;
                        if (t1->is_expired(now) && ! t2->is_expired(now)) {
                                return false;
                        } else  if (t2->is_expired(now) && ! t1->is_expired(now)) {
                                return true;
                        } else {
                                return t1->time > t2->time;
                        }
                }

                tender()
                        : src(0)
                        , max_response_cnt(1)
                        , received_response_cnt(0)
                        , winners()
                        , is_canceled(false)
                {
                }
        };

        struct response {
                ID32 peer;
                std::vector<ID32> offers;

                response(ID32 p, std::vector<ID32> so = std::vector<ID32>())
                        : peer(p)
                        , offers(so)
                {}
        };

        /**
         * It is based on heap data structure. Any time when element is
         * removed/added/updated from/to/within tenders timeline, it will be
         * heapified. 
         */
        typedef std::vector<tender*> tenders_timeline;


        /**
         * Key - tender id 
         * Value - tender info including corresponding request_id, uuid, cmd, etc.
         */
        typedef std::map<UUID, tender> tenders;

        /**
         * Key - tender id 
         * Value - response structure
         */
        typedef std::map<UUID, std::vector<response>> responses;


        /// @name Public interface
public:
        /**
         * @brief Does the functionality of the task
         */
        void runTask();

        /// @name Private helper member functions
private:

        void wait_for_input_channels();
        void handle_config_input();
        void process_config_input();
        unsigned int get_command_rating(metax::command);
        void configure_router();
        void handle_kernel_input();
        void handle_storage_input();
        void process_storage_input();
        void handle_storage_writer_input();
        void process_storage_writer_input();
        void handle_cache_input();
        void process_cache_input();
        void process_kernel_input();
        void handle_link_input();
        void process_link_input();
        void handle_wallet_input();
        void process_wallet_input();

        void handle_link_data();
        void send_link_data_to_kernel();
        void handle_link_nack ();
        void handle_expired_tenders();
        void handle_kernel_get();
        void handle_kernel_save_stream();
        void handle_kernel_recording_start();
        void handle_kernel_recording_stop();
        void handle_kernel_find_peer();
        void handle_kernel_confirm_for_multiwinner_tender();
        void handle_kernel_on_update_register_request();
        void handle_kernel_on_update_unregister_request();
        void handle_kernel_data_transfer();
        void handle_storage_data_transfer(const kernel_storage_package& in);
        void handle_kernel_offer();
        void handle_kernel_offer_for_multiwinner_tender(tender& t);
        void handle_kernel_accept_for_multiwinner_tender();
        void handle_kernel_save_request(const kernel_router_package&);
        void handle_refuse(ID32 i, platform::default_package pkg);
        void handle_kernel_clear_request();
        void forward_data_to_winner_peer(tender& t, ID32 p,
                                const std::vector<ID32>& w);
        void handle_peer_accept_for_multiwinner_tender(tender& t);
        void post_link_kernel_tender(tender& t,
                        const kernel_router_package&);
        void send_tender_data(tender& t);
        void send_tender_data(tender& t, metax::command cmd,
                                                platform::default_package p);
        void handle_new_remote_tender();
        void handle_tender_response(const tender& t);
        void process_existing_tender();
        void process_remote_tender(tender& t);
        bool is_first_command_of_multiwinner_tender(metax::command cmd)
                const;
        bool is_confirm_command_of_multiwinner_tender(metax::command cmd)
                const;
        bool is_accept_command_of_multiwinner_tender(metax::command rcmd,
                        metax::command tcmd) const;
        bool is_offer_command_of_multiwinner_tender(metax::command rcmd,
                        metax::command tcmd) const;
        void add_multiwinner_tender_response(const tender& t);
        void add_tender_response(const tender& t);
        void add_incoming_tender_to_responses();
        void send_incoming_tender_data_to_kernel(tender& t);
        void handle_peer_auth_data(tender& t);
        void handle_peer_accept(tender& t);
        void handle_peer_data(tender& t);
        void handle_peer_confirm_for_multiwinner_tender(tender& t);
        void process_existing_tender_data();
        void bypass_incoming_tender(tender& t);
        void clean_up_tender(tender& t_id);
        void clean_up_tender_info(tender& t_id);
        void decide_tender_winners(tender& t);
        void decide_tender_winners_for_save_request(tender& t);
        void select_all_responses_as_winners(tender& t);
        void send_response_to_kernel(const router_link_package* winner);
        void bypass_tender_response(const router_link_package* winner);
        void send_multiwinner_tender_offer_to_kernel(tender& t);
        void send_multiwinner_tender_offer_to_announcer_peer(tender& t);
        void handle_multiwinner_tender_responses(tender& t);
        void process_expired_tender(tender& t);
        tender* get_expired_tender();
        void cancel_request(ID32 r_id);
        void broadcast_tender_to_peers(tender& t, platform::default_package p);
        void send_accept_to_winner_peers(tender&);
        void forward_remote_tender(tender& t);

        void onTimer(Poco::Timer& t);

        ID32 get_request_id();

        UUID get_tender_id();

        void handle_backup_input();
        void process_backup_input();
        void post_new_tender_to_kernel(ID32 r_id);
        bool is_invalid_new_tender(command cmd) const;
        void process_new_tender(tender& t);
        void handle_new_sync_tender();
        void handle_sync_tenders();
        bool is_own_sync_tender(command cmd) const;
        void process_existing_sync_tender(tender& t);
        void handle_sync_data_transfer();
        void handle_kernel_sync_finished();
        void print_tenders_debug_info();
        void clean_up_tender_from_timeline(tender& t);
        void clean_up_sync_tender_info(tender& t);
        void handle_expired_sync_tender(tender& t);
        void post_sync_finished_request_to_link_peer(tender& t);
        void post_sync_started_request_to_link_peer(tender& t);
        void handle_kernel_get_uuid_request();
        void send_notification_to_kernel();

        /// @name Disabled special member functions
private:
        /// This class is not copy-constructible
        router(const router&);

        /// This class is not assignable
        router& operator=(const router&);

        /// @name External interface of router
public:
        KERNEL_RX kernel_rx;
        CONFIG_RX config_rx;
        INPUT wallet_rx;
        LINK_RX link_rx;
        KERNEL_RX backup_rx;
        STORAGE_RX storage_rx;
        STORAGE_RX storage_writer_rx;
        STORAGE_RX cache_rx;

        KERNEL_TX kernel_tx;
        CONFIG_TX config_tx;
        OUTPUT wallet_tx;
        LINK_TX link_tx;
        KERNEL_TX backup_tx;

        /// @name Private data
private:

        /// Table of tenders
        tenders m_tenders;

        tenders m_sync_tenders;

        // Router data corresponding to the current incomming tender from link
        router_data m_router_data;

        /// Response table 
        responses m_responses;

        /// Tenders timeline table
        tenders_timeline m_tenders_timeline;

        /// Request & tender mapping
        request_tender_map m_id_tenders;

        /**
         * Request counter domain is defined in the following way:
         * [ROUTER_REQUEST_LOWER_BOUND, ROUTER_REQUEST_UPPER_BOUND]
         *
         * See definition of macros in protocols.hpp
         */
        ID32 m_request_counter;


        /**
         * Tender counter domain is defined in the following way:
         * [0xXXXXXXXXXXXX0000, 0xXXXXXXXXXXXXFFFF]
         * where most significient 48 bits are generated based on the SessionID or
         * UserID(which are 2048bit PGP keys) at the very first launch of the
         * metax and saved as configuration 
         *
         * Hope by this method dublicate tenderIDs will be avoided in the
         * network.
         */

        /**
         * Maximum hop numbers in the route of the tender.
         * The default value is 7.
         * This can be defined in configuration file and neither request form
         * kernel, wallet, backup tasks is processed until the defined max hops
         * is not received from configuration manager.
         */
        ID16 MAX_HOPS;

        /**
         * Response time for 1 hop chain.
         * 
         * It is assumed that minimum response time for each tender is 0.2
         * sec and max is 2 sec.
         *
         * For each tender acceptable idling time is defined based on the
         * tender's hop number  in the current hop and response time
         * units.
         *
         * t.idling_interval = RESPONSE_TIME * t.hop_id;
         *
         * Measured by microsecond.
         */
        Poco::Timestamp::TimeVal RESPONSE_TIME;

        /**
         * Kernel response time for router request.
         * Measured by microsecond.
         */
        const Poco::Timestamp::TimeVal KERNEL_RESPONSE_TIME;
        
        /**
         * Sync tender response time
         */
        const Poco::Timestamp::TimeVal SYNC_TENDER_RESPONSE_TIME;

        /**
         * 1 KB transfer time in avarage. Measured by microsecond
         */
        //const Poco::Timestamp::TimeVal DATA_TRANSFER_TIME;

        /**
         * This timer is used to define idling tenders and clean-up tenders
         * list.
         */
        Poco::Timer m_tenders_timer;

        /**
         * This flag is used to define it is time to make a clean up action in
         * the list of tenders. When timer is elapsed this flag is set to true;
         *
         * As timer is running in a separate thread mutex is required to
         * eliminate race conditions on updating clean_up flag.
         */
        bool m_router_timeout;

        /**
         * This flag indicates whether expired tender exsits and waits for
         * processing.
         *
         * In order not to loop over all expired tenders at once and block
         * router to handle other requestes, this flag is set based on the
         * tenders_timeline entries when handling of the current expired tender
         * is finilized.
         */
        bool m_expired_tender_exists;

        /**
         * Mutex to eliminate race conditions on clean_up flag.
         */
        Poco::Mutex m_timeout_mutex;

        /**
         * Specifies whether router the configuration info is received from
         * configuration manager or not.
         */
        bool m_is_configured;

        /// @name Special member functions
public:
        /// Default constructor
        router();

        /// Destructor
        ~router();
        
}; // class leviathan::metax::router

#endif // LEVIATHAN_METAX_ROUTER_HPP

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:



/**
 * @file src/metax/hop.cpp
 *
 * @brief Implementation of the class @ref
 * leviathan::metax::hop
 *
 * COPYWRITE_TODO
 *
 */

// Headers from this project
#include "hop.hpp"
#include "kernel.hpp"
#include "configuration_manager.hpp"
#include "key_manager.hpp"
#include "router.hpp"
#include "link.hpp"
#include "wallet.hpp"
#include "storage.hpp"
#include "backup.hpp"
#include "user_manager.hpp"

// Headers from other projects
#include <platform/task.hpp>

// Headers from third party libraries

// Headers from standard libraries
#include <string>
#include <sstream>


void leviathan::metax::hop::
connect(leviathan::metax::configuration_manager* c,
        leviathan::metax::kernel* k,
        leviathan::metax::router* r,
        leviathan::metax::link* l,
        leviathan::metax::wallet* w,
        leviathan::metax::storage* s,
        leviathan::metax::storage* sw,
        leviathan::metax::storage* sc,
        leviathan::metax::key_manager* key,
        leviathan::metax::backup* b,
        leviathan::metax::user_manager* u)
{
    namespace CSP = leviathan::platform::csp;
    CSP::connect(c->kernel_tx, k->config_rx);
    CSP::connect(c->kernel_rx, k->config_tx);
    CSP::connect(c->router_tx, r->config_rx);
    CSP::connect(c->router_rx, r->config_tx);
    CSP::connect(c->link_tx, l->config_rx);
    CSP::connect(c->link_rx, l->config_tx);
    CSP::connect(c->wallet_tx, w->config_rx);
    CSP::connect(c->wallet_rx, w->config_tx);
    CSP::connect(c->storage_tx, s->config_rx);
    CSP::connect(c->storage_rx, s->config_tx);
    CSP::connect(c->storage_writer_rx, sw->config_tx);
    CSP::connect(c->storage_writer_tx, sw->config_rx);
    CSP::connect(c->cache_rx, sc->config_tx);
    CSP::connect(c->cache_tx, sc->config_rx);
    CSP::connect(c->key_manager_tx, key->config_rx);
    CSP::connect(c->key_manager_rx, key->config_tx);

    CSP::connect(k->router_tx, r->kernel_rx);
    CSP::connect(k->router_rx, r->kernel_tx);

    CSP::connect(k->key_manager_tx, key->kernel_rx);
    CSP::connect(k->key_manager_rx, key->kernel_tx);
    CSP::connect(l->key_manager_tx, key->link_rx);
    CSP::connect(l->key_manager_rx, key->link_tx);

    CSP::connect(k->storage_tx, s->kernel_rx);
    CSP::connect(k->storage_rx, s->kernel_tx);

    CSP::connect(k->wallet_tx, w->kernel_rx);
    CSP::connect(k->wallet_rx, w->kernel_tx);

    CSP::connect(r->wallet_tx, w->router_rx);
    CSP::connect(r->wallet_rx, w->router_tx);

    CSP::connect(r->link_tx, l->router_rx);
    CSP::connect(r->link_rx, l->router_tx);

    CSP::connect(b->router_tx, r->backup_rx);
    CSP::connect(b->router_rx, r->backup_tx);
    CSP::connect(b->kernel_tx, k->backup_rx);
    CSP::connect(b->kernel_rx, k->backup_tx);
    CSP::connect(b->user_manager_rx, u->backup_tx);

    CSP::connect(k->storage_writer_tx, sw->kernel_rx);
    CSP::connect(k->storage_writer_rx, sw->kernel_tx);
    CSP::connect(k->cache_tx, sc->kernel_rx);
    CSP::connect(k->cache_rx, sc->kernel_tx);

    CSP::connect(c->backup_tx, b->config_rx);
    CSP::connect(c->backup_rx, b->config_tx);
    CSP::connect(s->backup_tx, b->storage_rx);
    CSP::connect(s->backup_rx, b->storage_tx);

    CSP::connect(sw->backup_tx, b->storage_writer_rx);
    CSP::connect(sw->backup_rx, b->storage_writer_tx);
    CSP::connect(sc->backup_tx, b->cache_rx);
    CSP::connect(sc->backup_rx, b->cache_tx);

    CSP::connect(s->router_tx, r->storage_rx);
    CSP::connect(sw->router_tx, r->storage_writer_rx);
    CSP::connect(sc->router_tx, r->cache_rx);

    CSP::connect(k->user_manager_tx, u->kernel_rx);
    CSP::connect(k->user_manager_rx, u->kernel_tx);
    CSP::connect(c->user_manager_tx, u->config_rx);
}

leviathan::metax::hop::
hop(const std::string& cfg_path, const std::string& base_path)
        : rx(0)
        , tx(0)
{
    configuration_manager* c = new configuration_manager(cfg_path, base_path);
    router* r = new router();
    storage* s = new storage("storage");
    storage* sw = new storage("storage_writer");
    storage* sc = new storage("storage_cache", true);
    link* l = new link();
    wallet* w = new wallet();
    key_manager* key = new key_manager();
    backup* b = new backup();
    user_manager* u = new user_manager();
    kernel* k = new kernel(s, sw);

    connect(c, k, r, l, w, s, sw, sc, key, b, u);

    rx = &k->ims_rx;
    tx = &k->ims_tx;
}

leviathan::metax::hop::
~hop()
{
}

// vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:


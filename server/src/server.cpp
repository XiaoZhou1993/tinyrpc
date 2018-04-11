//
// Created by qiguo on 3/17/18.
//


#include <errlist.h>
#include "server.h"

using namespace std;
using namespace tinynet;

namespace tinyrpc
{
    void Server::initApp(int max_client)
    {
        m_app = std::make_shared<App>(max_client);
        m_app->start();
    }

    void Server::stopApp()
    {
        m_app->stop();
    }

    void Server::handleService(std::shared_ptr<TcpConn> client, const std::string &service, const Message &msg)
    {
        // client->loop() = m_app->loop();

        if (m_services.find(service) == m_services.end())
        {
            // TODO no such service. response to client and close connection
        }

        auto func = m_services[service];

        int ret = m_app->makeClient([func, client, msg](){
            Message retval;
            func(msg, retval);
            debug("retval=r%s", retval.data().c_str());
            Server::clientResp(client, retval);
        });
    }

    void Server::bind(const std::string & service, ServiceFunc func)
    {
        m_services[service] = std::move(func);
    }

    void Server::clientResp(std::shared_ptr<tinynet::TcpConn> client, const Message &retval)
    {
        client->onWrite([=](shared_ptr<TcpConn> c){
            retval.sendBy(c);
            c->detach();
        });
        client->readwrite(false, true);
    }

    ServerPool & ServerPool::instance()
    {
        static ServerPool svrPool;
        return svrPool;
    }

    void ServerPool::add(std::shared_ptr<Server> server)
    {
        m_servers.push_back(server);
        auto idx = m_servers.size() - 1; // server's index
        for (auto & it : server->m_services)
        {
            m_services[m_count] = it.first;
            m_svc2svr[m_count] = idx;
            m_count++;
        }
    }

    std::pair< std::shared_ptr<Server>, std::string > ServerPool::locate(uint64_t sid)
    {
        info("sid=%d, servers size=%d, services size=%d", sid, m_servers.size(), m_services.size());
        if (m_services.find(sid) == m_services.end())
        {
            debug("hhhh");
            return std::pair< std::shared_ptr<Server>, std::string> ();
        }

        if (m_svc2svr.find(sid) == m_svc2svr.end())
        {
            debug("hhhh");
            return std::pair< std::shared_ptr<Server>, std::string> ();
        }

        info("servers size=%d", m_servers.size());

        return std::make_pair(m_servers[m_svc2svr.at(sid)], m_services.at(sid));
    }

    App::App(int max_client):
        m_loop(max_client), m_clients((size_t)max_client), m_max_client(max_client)
    {
    }

    void App::start()
    {
        m_main_thread = Thread::create([this](){
            m_loop.start();
        });
    }

    void App::stop()
    {
        m_loop.stop();
    }

    int App::makeClient(const ThreadFunc & func)
    {
        // return m_clients.add(Thread::create(func));
        func();
    }
}

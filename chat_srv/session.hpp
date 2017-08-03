//
//  main.cpp
//  chat_srv
//
//  Created by apple-dev on 02.08.17.
//  Copyright © 2017 my. All rights reserved.
//

#ifndef CHAT_SESSION_HPP
#define CHAT_SESSION_HPP

#include <iostream>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/random.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>


using boost::asio::ip::tcp;

typedef boost::mt19937 RNGType;

typedef unsigned long long int user_id_type;
typedef unsigned long long int room_id_type;


//static RNGType random(static_cast<std::uint32_t>(time(0)));
//static RNGType _random(0);

class chats {
public:
    
    static chats & get() {
        static chats instance;
        return instance;
    }
    
//    void stat() {
//    }
    
    void emplace(room_id_type room_id, user_id_type user_id, tcp::socket * socket) {
        
        rooms_users[room_id].emplace(user_id);
        users_rooms[user_id].emplace(room_id);
        users_connections[user_id].emplace(socket);
        connections_users[socket] = user_id;
        
        return;
    }
    
    void erase_user(user_id_type user_id) {
        
        // find user_rooms
        auto user_rooms_iter = users_rooms.find(user_id);
        if(user_rooms_iter == users_rooms.end()) {
            return;
        }
        
        // delete from all rooms
        for(auto room: (user_rooms_iter->second)) {
            
            // get room user list
            auto room_users_iter = rooms_users.find(room);
            if(room_users_iter == rooms_users.end()) {
                continue;
            }
            
            
            auto & room_users = room_users_iter->second;
            auto room_user_iter = room_users.find(user_id);
            if(room_user_iter == room_users.end()) {
                continue;
            }
            
            room_users.erase(room_user_iter);
        }
        
        // delete from reverse mapping
        users_rooms.erase(user_rooms_iter);
        
        // delete from users_connections
        auto user_connections_iter = users_connections.find(user_id);
        if(user_connections_iter == users_connections.end()) {
            return;
        }
        
        auto & user_connections = user_connections_iter->second;
        for(auto connection: user_connections) {
            
            // delete from connections_users
            auto connection_user_iter = connections_users.find(connection);
            if(connection_user_iter == connections_users.end()) {
                continue;
            }
            
            connections_users.erase(connection_user_iter);
        }
        
        users_connections.erase(user_connections_iter);
        
        return;
    }
    
public:
    std::unordered_map<room_id_type, std::unordered_set<user_id_type>> rooms_users;
    std::unordered_map<user_id_type, std::unordered_set<room_id_type>> users_rooms;
    std::unordered_map<user_id_type, std::unordered_set<tcp::socket *>> users_connections;
    std::unordered_map<tcp::socket *, user_id_type> connections_users;
};

class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket socket)
    : socket_(std::move(socket)),
    timer_(socket_.get_io_service()),
    strand_(socket_.get_io_service())
    {
    }
    
    void go()
    {
        auto self(shared_from_this());
            boost::asio::spawn(strand_,
                           [this, self](boost::asio::yield_context yield)
                           {
                               try
                               {
                                   char data[128];
                                   for (;;)
                                   {
                                       timer_.expires_from_now(std::chrono::seconds(100));
                                       std::size_t n = socket_.async_read_some(boost::asio::buffer(data), yield);
                                       
                                       room_id_type room_id = 1;//_random();
                                       user_id_type user_id = (user_id_type)&socket_;//_random();
                                       
                                       auto & chats_instance = chats::get();
                                       
                                       chats_instance.emplace(room_id, user_id, &socket_);
                                       
                                       // get room users
                                       auto & room_users = chats_instance.rooms_users[room_id];
                                       
                                       for(auto user: room_users) {
                                           
                                           // get each room user connection
                                           auto & user_connections = chats_instance.users_connections[user];
                                           
                                           for(auto user_connection: user_connections) {
                                               boost::asio::async_write(*user_connection, boost::asio::buffer(data, n), yield);
                                           }
                                       }
                                   }
                               }
                               catch (std::exception& e)
                               {
                                   
                                   chats::get().erase_user((user_id_type)&socket_);
                                   
                                   socket_.close();
                                   timer_.cancel();
                               }
                           });
        
        boost::asio::spawn(strand_,
                           [this, self](boost::asio::yield_context yield)
                           {
                               while (socket_.is_open())
                               {
                                   boost::system::error_code ignored_ec;
                                   timer_.async_wait(yield[ignored_ec]);
                                   if (timer_.expires_from_now() <= std::chrono::seconds(0))
                                       socket_.close();
                               }
                           });
    }
    
private:
    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    boost::asio::io_service::strand strand_;
};

#endif // CHAT_SESSION_HPP

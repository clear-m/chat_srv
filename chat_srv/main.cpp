//
//  main.cpp
//  chat_srv
//
//  Created by apple-dev on 02.08.17.
//  Copyright © 2017 my. All rights reserved.
//

#include <stdio.h>

// comply valgrid specific stack checks
// only for tests under valgrind
//#ifndef BOOST_USE_VALGRIND
//    #define BOOST_USE_VALGRIND
//#endif

#include "session.hpp"

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: chat_srv <port>\n";
            return 1;
        }
        
        boost::asio::io_service io_service;
        
        boost::asio::spawn(io_service,
                           [&](boost::asio::yield_context yield)
                           {
                               tcp::acceptor acceptor(io_service,
                                                      tcp::endpoint(tcp::v4(), std::atoi(argv[1])));
                               
                               for (;;)
                               {
                                   boost::system::error_code ec;
                                   tcp::socket socket(io_service);
                                   acceptor.async_accept(socket, yield[ec]);
                                   if (!ec) std::make_shared<session>(std::move(socket))->go();
                               }
                           });
        
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}

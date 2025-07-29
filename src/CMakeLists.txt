set(SRC_LIST 
    Logger.cpp  
    main.cpp 
    Epoll.cpp 
    Thread.cpp    
    CServer.cpp
    HttpParser.cpp
    http_parser.c
)
add_executable(main ${SRC_LIST})

find_package(Threads REQUIRED)
target_link_libraries(main
    PRIVATE
    Threads::Threads
    pthread
)


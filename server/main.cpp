//
//  main.cpp
//  server
//
//  Created by 有澤悠紀 on 2023/09/02.
//

#include <iostream>
#include <sys/socket.h>
#include <string>

//using namespace std;

class Content{
    pthread_mutex_t lock;
    int file;
    Content(string filePath){
        pthread_mutex_init(lock, <#const pthread_mutexattr_t * _Nullable#>);
        file = open(filePath);
    }
    string read(){
        return file.read();
    }
    bool write(string newContent){
        pthread_mutex_lock(lock);
        file.write(newContent);
        pthread_mutex_unlock(lock);
    }
};

int main(int argc, const char * argv[]) {
    // insert code here...
    int server = socket(AF_INET, SOCK_STREAM, 0)
    if((server < 0){
        std::cerr << "Failed to open socket" << endl;
    }
    // bind port
    server = bind("localhost","8080");
    Content content = new Content("/tmp/some/path");
    // listen
    while(true){
        listen(server);
        fd client = accept(server, <#struct sockaddr *#>, <#socklen_t *#>);
        fork(){//process thread
            
        }
        string payload = recv(client);
        if(payload.start_with("read")){
            send("OK:"+content.read());
        }else if(payload.starts_with("write")){
            //read body
            string newContent = payload.readbody()
            content.write(newContent);
            send("OK:");
        }
    }
    // accpect
    
    
    // recv data from client
        // read command
            // send file content
        // write command
            // lock the file
            // read the payload
            // write to the file
            // unlock the file
    // send status
}

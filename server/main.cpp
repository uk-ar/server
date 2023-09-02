//
//  main.cpp
//  server
//
//  Created by 有澤悠紀 on 2023/09/02.
//

#include <iostream>
#include <fstream> // ofstream
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct_in
#include <unistd.h> // for close
#include <string>
#include <stdio.h>
#include <filesystem>
#include <pthread.h>

#define PORT 8080
#define CONTENT_PATH "/tmp/some/path"
#define BACKLOG 5 // number of client
#define READ_CMD  "read \r\n"
#define WRITE_CMD "write\r\n"
#define INSERT_CMD "insert\n"

// ignore null char
#define CMD_LEN (sizeof(READ_CMD)-1)

using namespace std;

class Repository{
    pthread_mutex_t lock;
    string filePath;
public:
    Repository(string file){
        pthread_mutex_init(&lock, NULL);
        filePath = file;
        // create file if not exist
        if(!filesystem::is_regular_file(filePath)){
            ofstream ofs(filePath);
            if(!ofs){
                // TODO:error check
            }
        }
    }
    ~Repository(){
        
    }
    bool read(string &ans){
        ifstream file;// cannot share because stream has state.
        file.open(filePath);
        if(!file){
            return false;
        }
        cout << file.tellg() <<endl;
        file >> ans;
        file.close();
        return true;
    }
    bool write(string newContent){
        pthread_mutex_lock(&lock);
        ofstream file;
        file.open(filePath);
        if(!file){
            pthread_mutex_unlock(&lock);
            return false;
        }
        file << newContent;
        file.flush();
        file.close();
        pthread_mutex_unlock(&lock);
        return true;
    }
    bool insert(string newContent, long long pos){
        //string filePath = "./foo";
        fstream file(filePath,ios_base::in | ios_base::out);
        if(!file){
            //pthread_mutex_unlock(&lock);
            return 1;
        }
        cout << file.tellp() << endl;
        //file.seekp(0,ios::end);
        cout << file.tellp() << endl;
        file.seekp(pos,ios::beg);
        //file.seekp(ios::end);
        cout << file.tellp() << endl;
        file << "newContent";
        //file.write(newContent.c_str(),newContent.size());
        file.flush();
        file.close();
        return true;
        /*
        cout << newContent <<":"<< pos <<":"<<endl;
        pthread_mutex_lock(&lock);
        fstream file(filePath,ios_base::in | ios_base::out);
        if(!file){
            pthread_mutex_unlock(&lock);
            return false;
        }
        cout << file.tellp() << endl;
        //file.seekp(0,ios::end);
        cout << file.tellp() << endl;
        file.seekp(pos,ios::beg);
        //file.seekp(ios::end);
        cout << file.tellp() << endl;
        file << newContent;
        //file.write(newContent.c_str(),newContent.size());
        file.flush();
        file.close();
        string ans;
        read(ans);
        cout <<"ans:"<< ans <<":ans"<< endl;
        
        pthread_mutex_unlock(&lock);
        return true;*/
    }
};

class Server{
public:
    int server_fd;
    Server(){
        // create a socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1){
            cerr << "Socket creation failed: " << strerror(errno) << endl;
            throw runtime_error("foo");
        }
        // enable to launch multiple times
        int reuse_flag = 1;// 1 means true
        if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEPORT,&reuse_flag,sizeof(reuse_flag)) == -1){
            cerr << "Socket cannot reuse: " << strerror(errno) << endl;
            throw runtime_error("foo");
        }
        
        struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(PORT),
            .sin_addr   = {htonl(INADDR_ANY)},
        };// C++20

        // bind port
        if(::bind(server_fd,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
            cerr << "Socket bind failed: " << strerror(errno) << endl;
            throw runtime_error("foo");
        }
        cout << "Waiting for a client..." << endl;

        if(listen(server_fd,BACKLOG) == -1){
            cerr << "Socket listen failed: " << strerror(errno) << endl;
            throw runtime_error("foo");
        }
    }
    virtual ~Server(){
        close(server_fd);
    }
    int respond(int client,Repository *repo){
        char buf[1024];
        ssize_t n = read(client,buf,CMD_LEN);
        if(n < 0){
            close(client);
            return EXIT_FAILURE;
        }
        string cmd(buf,n);
        if(cmd == READ_CMD){
            string content; //TODO: support large file
            repo->read(content);
            content.copy(buf,1024);
            write(client, buf, content.size());
            close(client);
        }else if(cmd == WRITE_CMD){
            n = read(client,buf,1024);
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string cont(buf,n);
            repo->write(cont);
            strncpy(buf,"OK\r\n",1024);
            write(client, buf, strlen(buf));
            close(client);
        }else if(cmd == INSERT_CMD){
            // position is limited to 64 bit integer
            n = read(client,buf,20);
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string spos(buf,n);
            long long pos = stoll(spos);
            n = read(client,buf,1024);
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string cont(buf,n);
            repo->insert(cont,pos);
            strncpy(buf,"OK\r\n",1024);
            write(client, buf, strlen(buf));
            close(client);
        }else{
            cerr << "Command unknown: " << endl;
            close(client);
            return EXIT_FAILURE;
        }
        return 0;
    }
    virtual int run(Repository *repo){
        while(true){
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server_fd, (struct sockaddr *)&client_addr,&client_addr_len);
            if(client == -1){
                cerr << "Socket accept failed: " << strerror(errno) << endl;
                return EXIT_FAILURE;
            }
            respond(client,repo);
            sleep(10);
        }
    }
};
class MultiProcessServer : public Server{
    int run(Repository *repo) override{
        while(true){
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server_fd, (struct sockaddr *)&client_addr,&client_addr_len);
            if(client == -1){
                cerr << "Socket accept failed: " << strerror(errno) << endl;
                return EXIT_FAILURE;
            }
            int child_pid = fork();
            if(child_pid < 0){//error
                return EXIT_FAILURE;
            }else if(child_pid == 0){//child
                respond(client,repo);
                sleep(10);//10 second
                close(server_fd);
                return 0;
            }else{//parent
                close(client);
                continue;
            }
        }
        // TODO: signal handling
        // TODO: wait for children
    }
};

class MultiThreadServer : public Server{
    int run(Repository *repo) override;
};

class Pack{
    int client;
    Repository*repo;
    MultiThreadServer *server;
public:
    Pack(int client_fd,Repository *repository,MultiThreadServer *serv){
        client = client_fd;
        repo = repository;
        server = serv;
    }
    void run(){
        server->respond(client,repo);
    }
};

void *worker(void *arg) {
    char *ret;
    Pack *pack = (Pack*)arg;
    pack->run();
    pthread_exit(&ret);
}

int MultiThreadServer::run(Repository *repo){
    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr *)&client_addr,&client_addr_len);
        if(client == -1){
            cerr << "Socket accept failed: " << strerror(errno) << endl;
            return EXIT_FAILURE;
        }
        pthread_t thid;
        Pack *pack = new Pack(client,repo,this);
        if(pthread_create(&thid, NULL, worker, (void *)pack)){
            
        }
    }
    // TODO: signal handling
    // TODO: wait for children
}

void print_usage(string file_name){
    cout << file_name << " <thread|process>" << endl;
    cout << "example:" << endl;
    cout << "> " << file_name << " thread" << "   'run as multithread'"<< endl;
    cout << "> " << file_name << " process" << "  'run as multiprocess'"<< endl;
}

int main(int argc, const char * argv[]) {
    vector<string> args(argv, argv + argc);
    if(args.size() != 2){
        cerr << "arg number miss match" << endl;
        print_usage(args[0]);
        return EXIT_FAILURE;
    }
    try{
        Repository * repo = new Repository("./foo");

        if(args[1] == "thread"){
            Server *server = new MultiThreadServer();
            server->run(repo);
        }else if(args[1] == "process"){
            Server *server = new MultiProcessServer();
            server->run(repo);
        }else{
            cerr << "command unknown" << endl;
            print_usage(args[0]);
            return EXIT_FAILURE;
        }
    }catch(runtime_error e){
        return EXIT_FAILURE;
    }
    return 0;
}

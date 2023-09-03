//
//  main.cpp
//  server
//
//  Created by 有澤悠紀 on 2023/09/02.
//

#include <iostream>
#include <sstream>
#include <fstream> // ofstream
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct_in
#include <unistd.h> // for close
#include <string>
#include <stdio.h>
#include <filesystem>
#include <pthread.h>
#include <filesystem>
#include <sys/shm.h>

#define PORT 8080
#define BUFF_SIZE 1024
#define CONTENT_PATH "/tmp/some/path"
#define BACKLOG 5 // number of client
#define READ_CMD  "read \r\n"
#define WRITE_CMD "write\r\n"
#define INSERT_CMD "insert\n"

// ignore null char
#define CMD_LEN (sizeof(READ_CMD)-1)

using namespace std;
namespace fs = std::filesystem;

class Repository{
    pthread_mutex_t *mutex;
    string filePath;
public:
    Repository(string file,pthread_mutex_t *lock){
        mutex = lock;
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
    bool read(stringstream &ans){
        ifstream file;// cannot share because stream has state.
        file.open(filePath);
        if(!file){
            return false;
        }
        stringstream buffer;
        ans << file.rdbuf();
        file.close();
        return true;
    }
    bool write(string &newContent){
        pthread_mutex_lock(mutex);
        fstream file(filePath,ios_base::out | ios_base::trunc);
        if(!file){
            pthread_mutex_unlock(mutex);
            return false;
        }
        file << newContent;
        file.flush();
        file.close();
        pthread_mutex_unlock(mutex);
        return true;
    }
    bool insert(string &newContent, long long pos){
        pthread_mutex_lock(mutex);
        fstream file(filePath,ios_base::in | ios_base::out);
        if(!file){
            pthread_mutex_unlock(mutex);
            return false;
        }
        file.seekp(pos,ios::beg);
        file << newContent;
        file.flush();
        file.close();
        pthread_mutex_unlock(mutex);
        return true;
    }
};
//abstract class
class Server{
public:
    int server_fd;
    Repository * repo;
    Server(){
        // create a socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_fd == -1){
            cerr << "Socket creation failed: " << strerror(errno) << endl;
            throw runtime_error("Server init");
        }
        // enable to launch multiple times
        int reuse_flag = 1;// 1 means true
        if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEPORT,&reuse_flag,sizeof(reuse_flag)) == -1){
            cerr << "Socket cannot reuse: " << strerror(errno) << endl;
            throw runtime_error("Server init");
        }
        
        struct sockaddr_in server_addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(PORT),
            .sin_addr   = {htonl(INADDR_ANY)},
        };// C++20

        // bind port
        if(::bind(server_fd,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
            cerr << "Socket bind failed: " << strerror(errno) << endl;
            throw runtime_error("Server init");
        }
        cout << "Waiting for a client..." << endl;

        if(listen(server_fd,BACKLOG) == -1){
            cerr << "Socket listen failed: " << strerror(errno) << endl;
            throw runtime_error("Server init");
        }
    }
    virtual ~Server(){
        close(server_fd);
    }
    int respond(int client,Repository *repo){
        char buf[BUFF_SIZE];
        ssize_t n = read(client,buf,CMD_LEN);
        if(n < 0){
            close(client);
            return EXIT_FAILURE;
        }
        string cmd(buf,n);
        if(cmd == READ_CMD){
            stringstream content; //TODO: support large file
            repo->read(content);
            long n = content.rdbuf()->sgetn(buf,BUFF_SIZE);
            write(client, buf, n);
            close(client);
        }else if(cmd == WRITE_CMD){
            n = read(client,buf,BUFF_SIZE);
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string cont(buf,n);
            repo->write(cont);
            strncpy(buf,"OK\r\n",BUFF_SIZE);
            write(client, buf, strlen(buf));
            close(client);
        }else if(cmd == INSERT_CMD){
            // position is limited to 64 bit integer
            n = read(client,buf,20+2);//+2 for \r\n
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string spos(buf,n);
            long long pos = stoll(spos);
            n = read(client,buf,BUFF_SIZE);
            if(n < 0){
                close(client);
                return EXIT_FAILURE;
            }
            string cont(buf,n);
            repo->insert(cont,pos);
            strncpy(buf,"OK\r\n",BUFF_SIZE);
            write(client, buf, strlen(buf));
            close(client);
        }else{
            cerr << "Command unknown: " << endl;
            close(client);
            return EXIT_FAILURE;
        }
        return 0;
    }
    virtual int run(){
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
public:
    int shmid;
    pthread_mutex_t *lock;
    MultiProcessServer():Server(){
        pthread_mutexattr_t attr;
        // shared memory for mutex
        shmid = shmget(IPC_PRIVATE, sizeof(pthread_mutex_t), 0600);
        if (shmid < 0) {
            perror("Shared memory cannot get:");
            throw runtime_error("Server init");
        }

        // attach shared memory
        lock = (pthread_mutex_t *)shmat(shmid, NULL, 0);
        if (lock == (pthread_mutex_t *)-1) {
            perror("Shared memory cannot get:");
            throw runtime_error("Server init");
        }
        
        // initialize attribute
        if (pthread_mutexattr_init(&attr) != 0) {
            perror("pthread_mutexattr_setpshared");
            throw runtime_error("Server init");
        }
        /* initialize with shared memory setting */
        if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
            perror("pthread_mutexattr_setpshared");
            throw runtime_error("Server init");
        }

        pthread_mutex_init(lock, &attr);
        repo = new Repository("./foo",lock);
    }
    ~MultiProcessServer(){
        // detach shared memory
        if(shmdt((const void*)lock)!= 0) {
            perror("shmdt");
        }
        
        if (shmctl(shmid, IPC_RMID, NULL) != 0) {
            perror("shmctl");
        }
    }
    int run() override{
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
public:
    MultiThreadServer() : Server(){
        pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        if (mutex == NULL) {
            perror("malloc");
            throw runtime_error("Server init");
        }
        if(pthread_mutex_init(mutex, NULL) != 0) {
            perror("pthread_mutex_init");
            throw runtime_error("Server init");
        }
        repo = new Repository("./foo",mutex);
    }
    int run() override;
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



int MultiThreadServer::run(){
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

void print_usage(fs::path path){
    string file_name = path.filename();
    cout << "usage:" << endl;
    cout << file_name << " <thread|process>" << endl << endl;
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
        Server *server = nullptr;
        if(args[1] == "thread"){
            cout << "running multi thread server" << endl;
            server = new MultiThreadServer();
        }else if(args[1] == "process"){
            cout << "running multi process server" << endl;
            server = new MultiProcessServer();
        }else{
            cerr << "command unknown" << endl;
            print_usage(args[0]);
            return EXIT_FAILURE;
        }
        server->run();
    }catch(runtime_error e){
        return EXIT_FAILURE;
    }
    return 0;
}

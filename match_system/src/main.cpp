// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "match_server/Match.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::match_service;
using namespace std;

struct Task{
    User user;
    string type;
};

struct MessageQueue{
    queue<Task> q;
    mutex m;
    condition_variable cv;
}message_queue;

class Pool{
    vector<User> users;
    public:

        void add(User user){
            users.push_back(user);
        }

        void remove(User user){
            for (uint32_t i = 0; i<users.size(); i++)
                if(users[i].id == user.id){
                    users.erase(users.begin()+i);
                    break;
                }
        }

        void save_result(int a,int b){
            printf("Match Resulf: %d %d\n",a,b);
        }

        void match(){
            while(users.size()>1){
                auto a=users[0],b = users[1];
                users.erase(users.begin());
                users.erase(users.begin());
                save_result(a.id,b.id);
            }
        }
}pool;
class MatchHandler : virtual public MatchIf {
    public:
        MatchHandler() {
            // Your initialization goes here
        }

        int32_t add_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("add_user\n");

            unique_lock<mutex> lck(message_queue.m);//此处获取mutex，lck的析构函数中释放mutex
            message_queue.q.push({user,"add"});
            message_queue.cv.notify_all();

            return 0;
        }

        int32_t remove_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("remove_user\n");

            unique_lock<mutex> lck(message_queue.m);
            message_queue.q.push({user,"remove"});
            message_queue.cv.notify_all();

            return 0;
        }

};

void consume_task(){
    while(true){
        unique_lock<mutex> lck(message_queue.m);
        if(message_queue.q.empty()){
            message_queue.cv.wait(lck);//下机等待，释放互斥变量
        }
        else{
            auto task = message_queue.q.front();
            message_queue.q.pop();
            lck.unlock();//提早释放资源为添加用户和删除用户的线程
            if(task.type == "add")pool.add(task.user);
            else if(task.type == "remove")pool.remove(task.user);
            pool.match();
        }
    }
}


//任务有添加用户，删除用户，根据用户池进行匹配，分析一下同步和互斥的关系，发现有互斥和同步（需要添加消息队列）
int main(int argc, char **argv) {
    int port = 9090;
    ::std::shared_ptr<MatchHandler> handler(new MatchHandler());
    ::std::shared_ptr<TProcessor> processor(new MatchProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    cout<<"start Match Server"<<endl;
    
    //add consumer progress
    thread matching_thread(consume_task);//function point
    server.serve();
    return 0;
}


#include <iostream>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <signal.h>
#include <vector>
#include <map>
#include <unistd.h>
#include <fstream>

#include <cstring>

/* Berkeley */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

using namespace std;

static volatile sig_atomic_t sigusr1_caught = 0;
static volatile sig_atomic_t sigusr2_caught = 0;

static mutex theLock;
vector<int> t;
map<int, std::thread> m;

/* TODO: REplace with unix socket */
std::string GID_PIPE_FILE="/tmp/timer";
int LISTEN_PORT = 42000;
bool tcplistener_running;

/* Read gidserver.conf at runtime and reload with SIGHUP */

/* TODO: Implement warn(), error(), log(), debug() and supress output (and code?) in production builds */

/* TODO: What happens if gidserver tries to execute gidserver? ;) */
std::string exec_cmd(string cmd) {
    char buffer[128];
    string result="";
    string f;

    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);

    if (!pipe) throw std::runtime_error("popen failed.");

    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL) {
            result += buffer;
            cout << buffer;
            }
        }

    return result;
    }

std::string execute_if_sane(std::string cmd_to_execute) {
    if(cmd_to_execute=="adb-pull") {
      return exec_cmd("$HOME/gid/adb-pull.sh");
    }
    else if(cmd_to_execute=="adb-push") {
      return exec_cmd("$HOME/gid/adb-push.sh");
    }
    else if(cmd_to_execute=="date") {
      return exec_cmd("date");
    }
    else {
        return "UNDEFINED (for now)";
    }
}

void timeout(int time_in_sex, int thread_id, string cmd_to_execute) {

    cout << "T[" << thread_id << "]: Sleep for : " << time_in_sex << " seconds and execute : " << cmd_to_execute << endl;
    this_thread::sleep_for(chrono::seconds(time_in_sex));

    // Do what we want to do here, like play an alarm or trigger the phone
    // Or call the phone. Each thread should have a final purpose that gid.sh set for it
    // using USR1

    cout << "T[" << thread_id << "] Executing command:" << cmd_to_execute << endl;
    cout << "T[" << thread_id << "] result is : " << execute_if_sane(cmd_to_execute) << endl;

    theLock.lock();
    t.push_back(thread_id);
    theLock.unlock();
    }

/* TODO - Check how the children handle these signals. Should we uninstall the signal handler for children threads? */

void handle_sighup(int signum) {

    /* TODO - gidserver.conf should be placed inside ~/.gid (decided at install time) */

    ifstream conf("$HOME/gid/gidserver.conf");
    std::string filepath;

    if(!conf)
        return;

    getline(conf, filepath);

    GID_PIPE_FILE=filepath;

    conf.close();
    }

void handle_sigusr1(int signum) {

    sigusr1_caught=1;
    }

void handle_sigusr2(int signum) {

  sigusr2_caught=1;

}

void tcp_server_process(int conn_backlog) {

    /* Create the tcp listener if it doesn't exist. */

    theLock.lock();
    if(tcplistener_running == true) {
      /* TODO: Do health checks? */
      theLock.unlock();
      return;
    }

    /* If still here, we haven't released the lock */
    tcplistener_running = true;
    theLock.unlock();

    /* TODO: Extend to multiple listeners on several ports (with configurability) */
    int listenfd=-1;
    struct sockaddr_in listenaddr;

    /* use SEQPACKET instead of STREAM ? */
    listenfd=socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0) {
    cout << "Could not get a socket. Aborting.";
    exit(1);
    }

    /* Clear the struct instance */
    memset(&listenaddr, 0, sizeof(struct sockaddr_in));

    /* Set address specifics of our listener */
    listenaddr.sin_family = AF_INET;
    listenaddr.sin_port   = LISTEN_PORT;
    inet_aton("127.0.0.1", &(listenaddr.sin_addr));
    //listenaddr.sin_addr.s_addr = (127 << 24) + (0 << 16) + (0 << 8) + (1 << 0);

    if(bind(listenfd, (struct sockaddr *) &listenaddr, sizeof(struct sockaddr_in)) < 0) {
      cout << "bind failed. Going down..." << endl;
      exit(2);
    }

    if(listen(listenfd, 1) < 0) {
      cout << "listen failed. Bailing out.." << endl;
      exit(3);
    }

    cout << "Now accepting connections on " << LISTEN_PORT << endl;

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_size = sizeof(struct sockaddr_in);
    int clientfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientaddr_size);

    if(clientfd < 0) {
      cout << "screwed up with accept()...dying.." << endl;
      exit(4);
    }

    std::string msg_hello_client = "Your journey begins here.\n";

    /* Enable a simple hello cipher here : TODO */
    ssize_t bytes_sent = send(clientfd, msg_hello_client.c_str(), msg_hello_client.length(), 0);

    /* TODO: Fix this -Wsign-compare warning to shut up gcc ;-) */
    if(bytes_sent < msg_hello_client.length()) {
      cout << "send() is not reliable...dying.." << endl;
      exit(5);
    }

    cout << "Going down with the ship. Goodbye, Your Captain" << endl;

    theLock.lock();
    /* TODO: Also set this to  false when killing tcp server. */
    tcplistener_running = false;
    cout << "Set listener to false!" << endl;
    theLock.unlock();
}

/* TODO: Replace communication with a nixock instead */
int main() {

    signal(SIGHUP,  handle_sighup);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    std::thread thread_tcp_listener = std::thread(tcp_server_process, 2);

    /* TODO [ADVANCED]: Can we get rid of this polling altogether? */
    int POLL_INTERVAL = 1;
    int tid = 0;
    int number_of_lines_read = 0;

    /* TODO - this wont scale for large and will overflow eventually */
    /* TODO - create gid config so that the filenames in sh and c stay the same for gidpiping */
    /* TODO - nixock */

    std::string inputline;
    std::string str_timeout="", str_cmd="";
    size_t end_of_timeout;

    /* TODO: Remove this infinite loop */
    while (1) {
      this_thread::sleep_for(chrono::seconds(POLL_INTERVAL));

      /* TODO - add a counter for ignoring client file if clients misbehaves */
      if(sigusr1_caught == 1) {

        sigusr1_caught = 0;

        /* TODO: Save offset in the file rather than number of lines. Replace this painfully
                 inefficient way to step through lines in the input. */

        /* very bad way to get this done */
        std::ifstream input(GID_PIPE_FILE);
        if (!input) {
          cout << "[ERROR] Can't open GID_PIPE_FILE: " << GID_PIPE_FILE << endl;
          exit(-2);
        }

        /* cout << "number of lines read: " << number_of_lines_read << endl; */
        for(int i = 0; i < number_of_lines_read && getline(input, inputline); i++);

        /* time and cmd get */
        getline(input, inputline);

        if(inputline.length() == 0) {
          cout << "[WARNING]: SIGUSR1 received but no lines to read." << endl;
          continue;
        }

        number_of_lines_read++;
        end_of_timeout = inputline.find(" ");

        if(end_of_timeout == string::npos) {
            /* No proper timeout = whine and bail */
            cout << "[WARNING]: Failed to parse line : " << inputline << endl;
            continue;
        }
        else {
            /* Make sure that all digits till space are numbers */
            size_t i;
            for(i = 0; i < end_of_timeout; i++)
                if(!isdigit(inputline[i]))
                    break;

            if(i!=end_of_timeout) {
                cout << "[WARNING]: Failed to parse line : " << inputline << endl;
                continue;
            }
            else {
                str_timeout = inputline.substr(0, end_of_timeout);
                str_cmd = inputline.substr(end_of_timeout+1);
            }
        }

        /* Hold onto our socks if we missed something earlier (like negative times) */
        if(str_timeout == "" || str_cmd == "" || str_timeout[0] == '-') {
            cout << "[WARNING]: Failed to parse line : " << inputline << endl;
            continue;
            /* TODO: Curse the client who put this damn line into our input feed */
        }

        /* All set now */
        int time_for_task = atoi(str_timeout.c_str());
        cout << "Creating thread." << endl;
        ++tid;
        m[tid]=std::thread(timeout, time_for_task, tid, str_cmd);
      }

      theLock.lock();
      if(t.size() > 0) {
        m[t[0]].join();
        cout << "T[" << t[0] << "]: Reaped (and sent to hell)" << endl;
        m.erase(t[0]);
        t.erase(t.begin());
      }

      theLock.unlock();

      theLock.lock();
      if(tcplistener_running != true) {
        cout << "The TCP Server has died. Reaping (and sending it to Hell)";
        thread_tcp_listener.join();
        tcplistener_running = true; /* BAD HACK, cant restart! TODO: */
      }
      theLock.unlock();
    }

    return 0;
}

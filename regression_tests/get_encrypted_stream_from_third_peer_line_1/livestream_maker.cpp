#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Exception.h>

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "Usage:\n";
		std::cerr << argv[0] << " <port> <source_file>" << std::endl;
		return 1;
	}
	int p = std::stoi(argv[1]);
        try {
                Poco::Net::ServerSocket srv;
                srv.bind(p, true, false);
                srv.listen();
                Poco::Net::StreamSocket ss = srv.acceptConnection();
                std::ifstream ls(argv[2], std::ios::binary | std::ios::in);
                const int bsize = 30000;
                char buf[bsize];
                while (!ls.eof()) {
                        ls.read(buf, bsize);
                        std::streamsize s = ls.gcount();
                        std::cout << "read chunk size:" << s << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        ss.sendBytes(buf, s);
                }
		std::cout << "reading file with chunks finished" << std::endl;
                ls.close();
        } catch(const Poco::Exception& e) {
                std::cout << e.displayText() << std::endl;
        } catch(...) {
                std::cout << "Uncaught Exception" << std::endl;
        }
	return 0;
}

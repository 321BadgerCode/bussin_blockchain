#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <openssl/sha.h>
#include <pcap.h>
#include <netinet/in.h>

#define VERSION "1.0.0"
#define DIFFICULTY 4

using namespace std;

string sha256(const string str){
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256,str.c_str(),str.size());
	SHA256_Final(hash,&sha256);
	stringstream ss;
	for(int i=0;i<SHA256_DIGEST_LENGTH;i++){
		ss<<hex<<setw(2)<<setfill('0')<<(int)hash[i];
	}
	return ss.str();
}

class Block{
public:
	int index;
	string previousHash;
	string timestamp;
	string data;
	string hash;
	int nonce;
	bool isMined;

	Block(int idx,string prevHash,string data)
		: index(idx),previousHash(prevHash),data(data),nonce(0){
		time_t now=time(0);
		timestamp=ctime(&now);
		hash=calculateHash();
	}

	string calculateHash() const {
		stringstream ss;
		ss<<index<<previousHash<<timestamp<<data<<nonce;
		return sha256(ss.str());
	}

	void mineBlock(int difficulty){
		string str(difficulty,'0');
		while(hash.substr(0,difficulty)!=str){
			nonce++;
			hash=calculateHash();
		}
	}
};

class Blockchain{
public:
	Blockchain(int difficulty):difficulty(difficulty){
		chain.push_back(createGenesisBlock());
	}

	void addBlock(Block newBlock){
		newBlock.previousHash=getLastBlock().hash;
		newBlock.mineBlock(difficulty);
		chain.push_back(newBlock);
	}

	Block getLastBlock() const {
		return chain.back();
	}

	int getChainSize() const {
		return chain.size();
	}

	vector<Block> getChain() const {
		return chain;
	}

private:
	vector<Block> chain;
	int difficulty;

	Block createGenesisBlock(){
		return Block(0,"0","Genesis Block");
	}
};

Blockchain globalBlockchain(DIFFICULTY);
mutex mtx;
condition_variable cv;

void sendBlock(Block block,const char* device){
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* handle=pcap_open_live(device,BUFSIZ,1,1000,errbuf);
	if(handle==nullptr){
		cerr<<"Couldn't open device "<<device<<": "<<errbuf<<endl;
		return;
	}

	string blockData=to_string(block.index)+block.previousHash+block.timestamp+block.data+block.hash+to_string(block.nonce);
	const u_char* packet=reinterpret_cast<const u_char*>(blockData.c_str());

	if(pcap_sendpacket(handle,packet,blockData.size())!=0){
		cerr<<"Error sending packet: "<<pcap_geterr(handle)<<endl;
	}

	pcap_close(handle);
}

// FIXME: Only being called once during program startup or smth? While loop is not looping in another thread in the background properly or smth.
void captureBlock(const char* device){
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* handle=pcap_open_live(device,BUFSIZ,1,1000,errbuf);
	if(handle==nullptr){
		cerr<<"Couldn't open device "<<device<<": "<<errbuf<<endl;
		return;
	}

	while(true){
		struct pcap_pkthdr header;
		const u_char* packet=pcap_next(handle,&header);
		if(packet==nullptr){
			continue;
		}

		string blockData(reinterpret_cast<const char*>(packet),header.len);

		stringstream ss(blockData);
		int index;
		string previousHash,timestamp,data,hash;
		int nonce;

		ss>>index>>previousHash>>timestamp>>data>>hash>>nonce;

		Block newBlock(index,previousHash,data);
		newBlock.timestamp=timestamp;
		newBlock.hash=hash;
		newBlock.nonce=nonce;

		lock_guard<mutex> lock(mtx);
		if(newBlock.index==globalBlockchain.getChainSize()&&newBlock.previousHash==globalBlockchain.getLastBlock().hash){
			globalBlockchain.addBlock(newBlock);
			cout<<"Received block: "<<newBlock.hash<<endl;
		}
	}

	pcap_close(handle);
}

void miner(int id,const char* device){
	while(true){
		Block newBlock(globalBlockchain.getChainSize(),globalBlockchain.getLastBlock().hash,"Block from miner "+to_string(id));
		newBlock.mineBlock(DIFFICULTY);

		{
			lock_guard<mutex> lock(mtx);
			if(!newBlock.isMined){
				cout<<"Miner "<<id<<" mined a block: "<<newBlock.hash<<endl;
				newBlock.isMined=true;
				sendBlock(newBlock,device);
				cv.notify_all();
			}
		}

		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

void startMining(int numMiners,const char* device){
	vector<thread> miners;
	for(int i=0;i<numMiners;++i){
		miners.emplace_back(miner,i,device);
	}

	for(auto& th:miners){
		th.join();
	}
}

int main(int argc,char** argv){
	int miners_amt=10;
	const char* device="enp5s0";
	for(int i=1;i<argc;++i){
		if(strcmp(argv[i],"-h")==0||strcmp(argv[i],"--help")==0){
			cout<<"Usage: "<<argv[0]<<" [options]."<<endl;
			cout<<"Options:"<<endl;
			cout<<"-h, --help\t\tShow this help message."<<endl;
			cout<<"--version\t\tShow the version of the program."<<endl;
			cout<<"-m <amount>\t\tSpecify the amount of miners (default: 10)."<<endl;
			cout<<"-d <device>\t\tSpecify the device to capture/send packets (default: enp5s0)."<<endl;
			cout<<"\t\"eth0\" is also a common device for Ethernet."<<endl;
			return 0;
		}
		else if(strcmp(argv[i],"--version")==0){
			cout<<"Version: "<<VERSION<<endl;
			return 0;
		}
		else if(strcmp(argv[i],"-m")==0){
			i++;
			if(i>=argc){
				cerr<<"No amount of miners specified!"<<endl;
				return 1;
			}
			miners_amt=atoi(argv[i]);
		}
		else if(strcmp(argv[i],"-d")==0){
			i++;
			if(i>=argc){
				cerr<<"No device specified!"<<endl;
				return 1;
			}
			device=argv[i];
		}
	}
	thread captureThread(captureBlock,device);
	captureThread.detach();

	cout<<"Starting mining with "<<miners_amt<<" miners..."<<endl;
	startMining(miners_amt,device);

	return 0;
}
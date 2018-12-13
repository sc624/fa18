#include <iostream>
#include <algorithm>
#include <random>
#include <ctime>
#include <fstream>
#include <vector>
using namespace std;

class node{
public:
    int maxbackoff, backoff, maxpacket, packetleft, numOfCollision;
    node(int s,int L){
      maxbackoff = s;
      backoff = 0;
      maxpacket = L;
      packetleft = L;
      numOfCollision = 0;}
};

vector<int> hasSomeoneToSend(int start, vector<node*>& nodevec){
    vector<int> ans;
    for(int i = start + 1; i < nodevec.size() ; ++i){
        if(nodevec[i]->backoff == 0)
            ans.push_back(i);
    }
    return ans;
}

void collisionOfNode(int i, int M, const vector<int>& Rvec, vector<node*>& nodevec,vector<int>& collisionNum){
    nodevec[i]->numOfCollision++;
    collisionNum[i]++;
    if(nodevec[i]->numOfCollision == M){ //drop the current packet if collision number = M
        nodevec[i]->maxbackoff = Rvec[0];
        nodevec[i]->backoff = rand() % (1 + nodevec[i]->maxbackoff);
        nodevec[i]->numOfCollision = 0;
    }
    else{
        nodevec[i]->maxbackoff *= 2;
        if(nodevec[i]->maxbackoff > Rvec.back())
            nodevec[i]->maxbackoff = Rvec.back();
        nodevec[i]->backoff = rand() % (1 + nodevec[i]->maxbackoff);
    }
}


double variance(vector<int> resultSet){
    double sum = 0;
    for(int i = 0; i < resultSet.size(); ++i)
      sum += resultSet[i];
    double mean = sum / resultSet.size(); //mean

    double accum = 0.0;
    for(int i = 0; i < resultSet.size(); ++i) {
        accum += (resultSet[i] - mean) * (resultSet[i] - mean);
    }
    double stdev = accum / (resultSet.size()); //variance
    return stdev;
}


int main(int argc, char**argv){
  if(argc != 2){
		fprintf(stderr, "usage: %s input.txt\n", argv[0]);
		exit(1);
  }

  ifstream file(argv[1]);
  ofstream fout("output.txt");

  srand(time(NULL));
  int N, L, R, M, T;    //reading in all the needed variables
  vector<int> Rvec;
  char ch;
  string str;
  bool channelOccupied = false;
  int whoOccupied = 0, packetsent = 0, globalTime = 0;

  file >> ch >> N >> ch >> L >> ch; //N,L

  while(file.get() != '\r'){
    file >> R;
    Rvec.push_back(R);
  }

  file >> ch >> M >> ch >> T;

  vector<int> successPacketNum(N,0), collisionNum(N,0);
  int totalCollision = 0;
  int idletime = 0;

  //organizing all the needed nodes for transmission

  vector<node*> nodevec(N);
  for(int i = 0; i < N; ++i){
      nodevec[i] = new node(Rvec[0], L);
  } //initialization of all nodes

  for(globalTime = 0; globalTime < T; ++globalTime){
    vector<int> result = hasSomeoneToSend(-1, nodevec);
    // int nums = result.size();
    if(result.size() == 0)
        idletime++;
    if(result.size() > 1 && channelOccupied == false){   //more than one want to occupy the empty channel
      totalCollision++;
      for(int i = 0; i < result.size(); ++i){
        collisionOfNode(result[i], M, Rvec, nodevec, collisionNum);
      }
      continue;
    }
    for(int i = 0; i < N; ++i){  //every time check the 25 nodes
        if(nodevec[i]->backoff != 0){
          if(channelOccupied){
              continue;
          }
          else if(hasSomeoneToSend(i,nodevec).size() == 0){
              nodevec[i]->backoff--;
              continue;
          }
          else{
              continue;
          }
        }
        else{  //the current node's backoff > 0
          if(!channelOccupied){   //if occupied happens
            whoOccupied = i;
            channelOccupied = true;
            nodevec[i]->packetleft--;
          }
          else{  //the channel is not yet occupied and only you want to transmit
            if(whoOccupied != i){   //occupied by itself
              collisionOfNode(i, M, Rvec, nodevec, collisionNum);
              continue;
            }
            else{                     //occupied by others
              if(nodevec[i]->packetleft != 1){ //in this slot it is all sent and re-initialize
                nodevec[i]->packetleft--;
                continue;
              }
              else{
                packetsent++;
                successPacketNum[i]++;
                channelOccupied = false;
                nodevec[i]->numOfCollision = 0;
                nodevec[i]->backoff = rand() % (1 + nodevec[i]->maxbackoff);
                nodevec[i]->maxbackoff = Rvec[0];
                nodevec[i]->packetleft = L;
                break;
              }
            }
          }
        }
      }
    }


    fout << /*"channel utilization " <<*/ packetsent * L * 1.0 / T * 100 << "%" << endl;
    fout << /*"channel idle fraction " <<*/ idletime * 1.0 / T * 100 << "%" << endl;
    fout << /*"total collision " <<*/ totalCollision << endl;
    fout << "variance of success transmission " << variance(successPacketNum) << endl;
    fout << "variance of collision " << variance(collisionNum) << endl;
    return 0;
}

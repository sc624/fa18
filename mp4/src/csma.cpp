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
      backoff = 0;
      numOfCollision = 0;
      maxbackoff = s;
      maxpacket = L;
      packetleft = L;
    }
};

void NodeCollision(int i, int M, const vector<int>& range_vec, vector<int>& collision_num, vector<node*>& node_v){
    node_v[i]->numOfCollision++;
    collision_num[i]++;
    if(node_v[i]->numOfCollision == M){ //drop the current packet if collision number = M
        node_v[i]->maxbackoff = range_vec[0];
        node_v[i]->numOfCollision = 0;
        node_v[i]->backoff = rand() % (1 + node_v[i]->maxbackoff);
    }
    else{
        node_v[i]->maxbackoff *= 2;
        if(node_v[i]->maxbackoff > range_vec.back())
            node_v[i]->maxbackoff = range_vec.back();
        node_v[i]->backoff = rand() % (1 + node_v[i]->maxbackoff);
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

vector<int> transmit(int start, vector<node*>& node_v){
  vector<int> ans;
  for(int i = start + 1; i < node_v.size() ; ++i){
    if(node_v[i]->backoff == 0)
    ans.push_back(i);
  }
  return ans;
}

int main(int argc, char**argv){
  if(argc != 2){
		fprintf(stderr, "usage: %s input.txt\n", argv[0]);
		exit(1);
  }

  ifstream file(argv[1]);
  ofstream fout("output.txt");

  srand(time(NULL));
  int N, L, R, M, T;    //nodes, packet size, range, retransmission attempt, time
  vector<int> range_vec;
  char ch;
  string str;
  bool channelOccupied = false;
  int whoOccupied = 0, packetsent = 0, time = 0;

  file >> ch >> N >> ch >> L >> ch; //read in values
  while(file.get() != '\r'){      //vector of random vals
    file >> R;
    range_vec.push_back(R);
  }
  file >> ch >> M >> ch >> T;

  vector<int> success_num(N, 0), collision_num(N, 0);
  int totalCollision = 0, idletime = 0;

  vector<node*> node_v(N);
  for(int i = 0; i < N; ++i){
      node_v[i] = new node(range_vec[0], L);
  } //initialize nodes

  for(time = 0; time < T; ++time){
    vector<int> result = transmit(-1, node_v);
    if(result.size() == 0)
        idletime++;
    if(result.size() > 1 && channelOccupied == false){//if more than one want to occupy the empty channel
      totalCollision++;
      for(int i = 0; i < result.size(); ++i){
        NodeCollision(result[i], M, range_vec, collision_num, node_v);
      }
      continue;
    }

    for(int i = 0; i < N; ++i){  //every time check the 25 nodes
        if(node_v[i]->backoff != 0){
          if(channelOccupied){
              continue;
          }
          else if(transmit(i, node_v).size() == 0){
              node_v[i]->backoff--;
              continue;
          }
          else{
              continue;
          }
        }
        else{
          if(!channelOccupied){   //if channel not occupied
            whoOccupied = i;
            channelOccupied = true;
            node_v[i]->packetleft--;
          }
          else{  //the channel is not yet occupied and only you want to transmit
            if(whoOccupied != i){   //occupied by itself
              NodeCollision(i, M, range_vec, collision_num, node_v);
              continue;
            }
            else{                     //occupied by others
              if(node_v[i]->packetleft != 1){ //sent and re-initialize
                node_v[i]->packetleft--;
                continue;
              }
              else{
                success_num[i]++;
                packetsent++;
                node_v[i]->numOfCollision = 0;
                channelOccupied = false;
                node_v[i]->backoff = rand() % (1 + node_v[i]->maxbackoff);
                node_v[i]->maxbackoff = range_vec[0];
                node_v[i]->packetleft = L;
                break;
              }
            }
          }
        }
      }
    }

    fout << "channel utilization " << packetsent * L * 1.0 / T * 100 << endl;
    fout << "channel idle fraction " << idletime * 1.0 / T * 100 << endl;
    fout << "total collision " << totalCollision << endl;
    fout << "variance of success transmission " << variance(success_num) << endl;
    fout << "variance of collision " << variance(collision_num) << endl;
    return 0;
}

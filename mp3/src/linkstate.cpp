#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <fstream>

#define INF 9999
#define REMOVE -999

using namespace std;

ofstream fout("output.txt");

struct message{
  int from, to;
  string sentence;
};

struct three{
    int a, b, c;
    three(){}
    three(int aa, int bb, int cc){
      c = cc;
    	b = bb;
      a = aa;
    }
};

class graph{
    private:
    int vertices, edges;
    struct edgenode{
        int end, weight;
        edgenode* next;
        edgenode(int e, int w, edgenode* n = NULL){
          next = n;
        	end = e;
        	weight = w;
        }
    };

    struct vertnode{
        int ver;
        edgenode* head;
        vertnode(edgenode* h = NULL){
        	head = h;
        }
    };

    vertnode* verlist;

    public:
    graph(int vsize, int d[]){
        vertices = vsize;
        edges = 0;
        verlist = new vertnode[vsize];
        for(int i = 0; i < vertices; ++i){
            verlist[i].ver = d[i];
        }
    }
    ~graph(){
        edgenode* p;
        for(int i = 0; i < vertices; ++i){
            while(verlist[i].head != NULL){
                p=verlist[i].head;
                verlist[i].head = p->next;
                delete p;
            }
        }
        delete []verlist;
    }

    bool insert(int u, int v, int w){
        verlist[u].head = new edgenode(v, w, verlist[u].head);
        ++edges;
        return true;
    }

    bool remove(int u, int v){
        edgenode* p = verlist[u].head,*q;
        if(p == NULL)
        	return false;
        if(p->end == v){
            verlist[u].head = p->next;
            delete p;
            --edges;
            return true;
        }
        if(p->next != NULL && p->next->end != v)
        	p=p->next;
        if(p->next == NULL)
        	return false;
        if(p->next->end == v){
            q=p->next;
            p->next=q->next;
            delete q;
            --edges;
            return true;
        }
        return true;
    }

    bool exist(int u, int v){
        edgenode*p=verlist[u].head;
        while(p!=NULL&&p->end!=v)
        	p=p->next;
        if(p == NULL)
        	return false;
        else
        	return true;
    }

    void printpath(int start, int end, int prev[], vector<int>& result){
        if(end == start){
            result.push_back(verlist[start].ver);
            return;
        }
        printpath(start, prev[end], prev, result);
        result.push_back(verlist[end].ver);
    }

    static bool compare2(const three &aa, const three &bb){
        return aa.a < bb.a;
    }

    void dijkstra(int start){
        int* prev = new int[vertices];
        int* distance = new int[vertices];
        bool* known = new bool[vertices];
        int no, i, j, start_num;
        edgenode* p;

        vector<int> result;
        vector<three> ans;
        three temp_ans;

        for(i = 0; i < vertices; ++i){
            known[i] = false;
            distance[i] = INF;
        }
        for(start_num = 0; start_num < vertices; ++start_num){
            if(verlist[start_num].ver == start)
              break;
        }

        distance[start_num] = 0;
        prev[start_num] = start_num;

        for(i = 1; i < vertices; ++i){        //update for n-1 times is enough
            int min = INF;
            for(j = 0; j < vertices; ++j){
                if(!known[j] && distance[j]<min){
                    min = distance[j];
                    no = j;
                }
            }
            known[no] = true;
            for(p = verlist[no].head; p != NULL; p = p->next){
                if(!known[p->end] && distance[p->end] >= min + p->weight){
                    if(distance[p->end] > min + p->weight)
                      prev[p->end]=no;
                    if(distance[p->end] != INF && distance[p->end] == min + p->weight && prev[p->end] > no)
                      prev[p->end] = no;
                    distance[p->end] = min + p->weight;
                }
            }
        }
        for(i = 0 ; i < vertices; ++i){
          temp_ans.a = verlist[i].ver;
          result.clear();
          if(distance[i] == INF)
            continue;                   //////////////////////////////important!!!
          else
            printpath(start_num, i, prev, result);
          if(result.size() == 1)
            temp_ans.b = result[0];
          else if(result.size() == 0)
            continue;
          else
            temp_ans.b = result[1];
          if(distance[i] == INF)
            temp_ans.c = REMOVE;
          else
            temp_ans.c = distance[i];
          ans.push_back(temp_ans);
        }
        sort(ans.begin(), ans.end(), compare2);   //according to start num (inner loop)
        for(int i = 0; i < ans.size(); ++i){
            if(ans[i].c != REMOVE)
              fout << ans[i].a << ' ' << ans[i].b << ' ' << ans[i].c << endl;
        }
    }

    void dijkstra_all(int start, int to){// both are sequence
        bool* known = new bool[vertices];
        int* distance = new int[vertices];
        int* prev = new int[vertices];

        edgenode* p;
        int no, i, j;
        int start_num = start;
        vector<int> result;

        for(i = 0; i < vertices; ++i){
            known[i] = false;
            distance[i] = INF;
        }

        distance[start_num] = 0;
        prev[start_num] = start_num;

        for(i = 1; i < vertices; ++i){        //update for n-1 times is enough
            int min = INF;
            for(j = 0; j < vertices; ++j){
                if(!known[j] && distance[j] < min){
                    min = distance[j];
                    no = j;
                }
            }
            known[no] = true;
            for(p = verlist[no].head; p != NULL; p = p->next){
                if(!known[p->end] && distance[p->end] >= min + p->weight){
                    if(distance[p->end]>min+p->weight)
                      prev[p->end] = no;
                    if(distance[p->end] != INF && distance[p->end] == min + p->weight && prev[p->end] > no)
                      prev[p->end] = no;
                    distance[p->end] = min + p->weight;
                }
            }
        }

        if(distance[to] == INF){
          fout << "infinite hops unreachable ";
          return;
        }
        else{
          fout << distance[to] << " hops ";
        }

        printpath(start_num, to, prev, result);
        for(int i = 0; i < result.size() - 1; ++i)
          fout << result[i] << ' ';
    }
};

static bool compare(const pair<int,int>&a, const pair<int,int>&b){
    return a.first<b.first;
}


int main(int argc, char** argv){
    if (argc != 4){
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    ifstream topofile(argv[1]);
    ifstream messagefile(argv[3]);
    ifstream changesfile(argv[2]);

    int aa, bb, cc;
    int seq = 0;
    vector<three> vec;
    unordered_map<int,int> map;

    vector<pair<int, int>> dub_pair;
    pair<int,int> temp_pair;


    vector<message> message_vec;
    message msg;
    int ff,tt;
    char ww;
    string s;

    int c1, c2, hmm;

    while(topofile >> aa >> bb >> cc){
        three tmp(aa, bb, cc);
        vec.push_back(tmp);
        if(map.find(bb) == map.end()){
          map[bb] = seq;
          seq++;
        }
        if(map.find(aa) == map.end()){
          map[aa] = seq;
          seq++;
        }
    }

    int num_vertices = seq;
    int* array = new int[num_vertices];

    for(int i = 0; i < num_vertices; ++i){
        int tmp = 0;
        for(auto iter = map.begin(); iter != map.end(); ++iter){
            if(iter->second == i){
              tmp = iter->first;
              break;
            }
        }
        array[i] = tmp;
    }

    graph link(num_vertices, array);

    for(int i = 0; i < vec.size(); ++i){
        if(vec[i].c == REMOVE)
          continue;
        if(!link.exist(map[vec[i].b], map[vec[i].a]))
          link.insert(map[vec[i].b], map[vec[i].a], vec[i].c);
        if(!link.exist(map[vec[i].a], map[vec[i].b]))
          link.insert(map[vec[i].a], map[vec[i].b], vec[i].c);
    }

    for(auto iter = map.begin(); iter != map.end(); ++iter){
        temp_pair.second = iter->second;
        temp_pair.first = iter->first;
        dub_pair.push_back(temp_pair);
    }

    sort(dub_pair.begin(), dub_pair.end(), compare);

    for(int i = 0; i < num_vertices; ++i){
        link.dijkstra(dub_pair[i].first);
    }

    while(changesfile >> ff >> tt){
        changesfile.get(ww);
        getline(changesfile, s);
        msg.from = ff;
        msg.to = tt;
        msg.sentence=s;
        message_vec.push_back(msg);
    }

    for(int i = 0; i < message_vec.size(); ++i){
        fout << "from " << message_vec[i].from << " to " << message_vec[i].to << " cost ";
        link.dijkstra_all(map[message_vec[i].from], map[message_vec[i].to]);
        fout << "message " << message_vec[i].sentence << endl;
    }

    while(messagefile >> c1 >> c2 >> hmm){
        link.remove(map[c1], map[c2]);
        link.remove(map[c2], map[c1]);
        if(hmm != REMOVE){
            if(!link.exist(map[c1], map[c2]))
              link.insert(map[c1], map[c2], hmm);
            if(!link.exist(map[c2],map[c1]))
              link.insert(map[c2], map[c1], hmm);
        }
        for(int i = 0; i < num_vertices; ++i){
            link.dijkstra(dub_pair[i].first);
        }

        for(int i = 0; i < message_vec.size(); ++i){
            fout << "from " << message_vec[i].from << " to " << message_vec[i].to << " cost ";
            link.dijkstra_all(map[message_vec[i].from], map[message_vec[i].to]); //concrete information about hops
            fout << "message " << message_vec[i].sentence << endl;
        }
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <queue>
#define INF	9999

using namespace std;

int total_nodes, total_node_arrays;
string topofile, messagefile, changesfile;
vector<vector<int>> edgeCost;

//basic element of our topological map
struct DVmessage{
	int source, destination, thru, cost;
	vector<int> distance_vector;
};

struct Node{
	int id;
	unordered_map<int, int> neighbors;
	vector<vector<int>> costTable;
	vector<int> path, leastCost;
	bool flag;
	vector<DVmessage> msgs;
};

//forwarding table format is destination, nexthop, pathcost ---> to be printed out
struct forward_entry{
	int destination, nexthop, pathcost;
};


//message to be parsed in and used
struct message_from_file{
	int source;
	int destination;
	string message;
};



ofstream outfile;
map<int, Node*> topo_map, cnodes; //changed from unordered to make it easier to get correct output
queue<message_from_file*> every_message;
vector< vector<forward_entry> >  forward_table;


vector<Node> graph_nodes;
vector<vector<DVmessage>> NodeMessages; //2d vector array holds messages from nodes in DV alrogrithm

void Print_Forward_Table(int cnode){
	Node cur = graph_nodes[cnode];
	forward_table[cnode].resize(total_node_arrays);
	for(int i = 1; i < total_node_arrays; i++){
		outfile << i;
		forward_table[cnode][i].destination = i;
		outfile << " ";
		if(cnode != i){
			outfile << cur.path[i];
			forward_table[cnode][i].nexthop = cur.path[i];
		}
		else{
			outfile << i;
			forward_table[cnode][i].nexthop = i;
		}
		outfile << " ";
		outfile << cur.costTable[cnode][i];
		forward_table[cnode][i].pathcost = cur.costTable[cnode][i];
		outfile << "\n";
	}
}



void send_messages(){
	while(every_message.size() > 0){
		int source, dest;
		int idx = 0;
		queue<int> hop_path; //store current path of hops

		source = every_message.front()->source;
		dest = every_message.front()->destination;
		//initial output
		outfile << "from " << source << " to " << dest << " cost ";
		//calculate next hops

		//need to look at hash map of forward tables, recursively choose nexthop
		hop_path.push(source);
		vector<forward_entry> cur_table = forward_table[source];
		bool isreachable = false;
		while(cur_table[idx].destination != dest && idx < cur_table.size()){
			idx++;
		}
		if(cur_table[idx].nexthop != -1){
				isreachable = true;
		}
		if(isreachable){
			outfile << graph_nodes[source].costTable[source][dest]<<" hops ";
			int nexthop = cur_table[idx].nexthop;
			if(nexthop != dest){
				hop_path.push(nexthop);
			}
			while(nexthop != dest){
				cur_table = forward_table[nexthop];
				idx = 0;
				while(cur_table[idx].destination != dest && idx < cur_table.size())
					idx++;
				nexthop = cur_table[idx].nexthop;
				if(nexthop != dest){
					hop_path.push(nexthop);
				}
			}
			//now have hop path so need to print it out
			while(hop_path.size() > 0){
				outfile << hop_path.front() << " ";
				hop_path.pop();
			}
			outfile << "message" << every_message.front()->message << "\n";
			every_message.pop();
		}
		//case where message cant be sent to destination
		else{
			outfile << "infinite hops ";
			outfile << "unreachable message" << every_message.front()->message << "\n";
			every_message.pop();
		}
	}
}

int get_topology(){
	ifstream topfile(topofile);
	int total_nodes = 0;
	int a, b, c;

	while(topfile >> a >> b >> c){
		//if b not in keys of map
		if(topo_map.find(b) == topo_map.end()){
			Node *dest_node = new Node;
			topo_map[b]= dest_node;
			topo_map[b]->id = b;
			total_nodes++;
		}
		//if a not in keys of map
		if(topo_map.find(a) == topo_map.end()){
			Node *source_node = new Node;
			topo_map[a] = source_node;
			topo_map[a]->id = a; //node ide, probably redundant but might need later
			total_nodes++;
		}
		//node have been mapped for current source/dest, add in neighbor/costs
		if(topo_map[b]->neighbors.find(a) == topo_map[b]->neighbors.end())
			topo_map[b]->neighbors[a] = c;
		//have to do the same for the other node
		if(topo_map[a]->neighbors.find(b) == topo_map[a]->neighbors.end())
			topo_map[a]->neighbors[b] = c; //cost of the link
	}

	//initialize the edgecost matrixes
	edgeCost.resize(total_nodes + 1);
	for(int i = 1; i < total_nodes + 1; i++){
		edgeCost[i].resize(total_nodes + 1);
		for(int j = 1; j < total_nodes + 1; j++){
			if(i != j) //set to inf
				edgeCost[i][j] = INF; // we use 9999 as infinity number
			else //cost to node = 0
				edgeCost[i][j] = 0;
		}
	}

	ifstream topfile_(topofile);
	while(topfile_ >> a >> b >> c){
		if(c < 100  && c > 0){
			edgeCost[b][a] = c;
			edgeCost[a][b] = c;
		}
	}

	topfile.close();
	topfile_.close();
	return total_nodes;
}

void initialize_nodes(){
	graph_nodes.resize(total_node_arrays);
	for(int i = 1 ; i < total_node_arrays; i++){
		graph_nodes[i].id = i;
		graph_nodes[i].flag = false;
		graph_nodes[i].path.resize(total_node_arrays);
		graph_nodes[i].costTable.resize(total_node_arrays);
		graph_nodes[i].leastCost.resize(total_node_arrays);
		for(int j = 1; j < total_node_arrays; j++){
			graph_nodes[i].costTable[j].resize(total_node_arrays);
		}
	}

	for(int i = 1; i < total_node_arrays; i++){//for each node in network
		for(int j = 1; j < total_node_arrays; j++){
			graph_nodes[i].leastCost[j] = edgeCost[i][j];
			if(edgeCost[i][j] < 100 && edgeCost[i][j] > 0)
				graph_nodes[i].path[j]=j;
			else{
				if(i != j)
					graph_nodes[i].path[j] = -1;//not reachable
				else
					graph_nodes[i].path[j] = 0;
			}
		}
		for(int m = 1; m < total_node_arrays; m++){
			for(int n = 1; n < total_node_arrays; n++){
				if(i != m)
					graph_nodes[i].costTable[m][n] = INF;
				else
					graph_nodes[i].costTable[m][n] = edgeCost[m][n];
				if(m == n)
					graph_nodes[i].costTable[m][n] = 0;
			}
		}

		//sending DV messages to its neighbors
		for(unordered_map<int, int>::iterator it = topo_map[i]->neighbors.begin(); it != topo_map[i]->neighbors.end(); it++){
			DVmessage message;
			int dest = it->first;
			message.thru = i;
			message.source = i;
			message.destination = dest;
			message.distance_vector = graph_nodes[i].costTable[i];
			graph_nodes[dest].msgs.push_back(message);
		}
	}
}

void cost_table(int cnode){
	int flag = 0;
	for(vector<DVmessage>::iterator it = graph_nodes[cnode].msgs.begin(); it != graph_nodes[cnode].msgs.end(); it++){
		int thru = it->thru;
		graph_nodes[cnode].costTable[thru] = it->distance_vector;
			for(int j = 1; j < total_node_arrays; j++){
				if(cnode != j && thru != j){
					int costOld = graph_nodes[cnode].costTable[cnode][j];
					int costNew = edgeCost[cnode][thru] + it->distance_vector[j];
					if(costNew == costOld){
						if(thru < graph_nodes[cnode].path[j])
						graph_nodes[cnode].path[j] = thru;
					}
					if(costNew < costOld){
						graph_nodes[cnode].path[j] = thru;
						graph_nodes[cnode].costTable[cnode][j] = costNew;
						if(graph_nodes[cnode].path[thru] != thru)
							graph_nodes[cnode].path[j] = graph_nodes[cnode].path[thru];
						graph_nodes[cnode].leastCost[j] = costNew;
						flag = 1;
					}
				graph_nodes[cnode].costTable[j][cnode] = graph_nodes[cnode].costTable[cnode][j];
				}
			}
	}

	graph_nodes[cnode].msgs.clear();

	for(int m = 1; m < total_node_arrays; m++)
		for(int n = 1; n < total_node_arrays; n++)
			if(graph_nodes[cnode].costTable[m][n] > graph_nodes[cnode].costTable[n][m])
				graph_nodes[cnode].costTable[m][n] = graph_nodes[cnode].costTable[n][m];

	if(flag == 1){
		for(unordered_map<int, int>::iterator it2 = topo_map[cnode]->neighbors.begin(); it2 != topo_map[cnode]->neighbors.end(); it2++){
			DVmessage message;
			int dest = it2->first;
			message.destination = dest;
			message.source = cnode;
			message.thru = cnode;
			message.distance_vector = graph_nodes[cnode].costTable[cnode];
			graph_nodes[dest].msgs.push_back(message);
		}
	}
}

void update(){
	ifstream msgfile(messagefile);
	ifstream changefile(changesfile);
	if(!changefile.is_open())
		cout<<"File opened failed!"<<endl;
	int source, dest, cost;
	int change_time = 0;
	string line, message;
	while(changefile >> source >> dest >> cost){
		change_time++;
		//check for existing link b/w the nodes in file
		if(topo_map[source]->neighbors.find(dest) !=  topo_map[source]->neighbors.end()){
			//remove link if cost is -999
			if(cost == -999){
				topo_map[source]->neighbors.erase(dest); //no longer neighbors
				topo_map[dest]->neighbors.erase(source);
				edgeCost[source][dest] = INF;
				edgeCost[dest][source] = INF;
			}
			//link costs can only be positive, never 0. -999 means remove link
			else if(cost > 0){
				//update path cost
				topo_map[source]->neighbors[dest]  =  cost;
				//have to do for destinations neighbor too
				topo_map[dest]->neighbors[source]  =  cost;
				edgeCost[source][dest] = cost;
				edgeCost[dest][source] = cost;
			}
		}
		//no existing link exists, create one
		else{
			//if no link exists and cost value is 0 or -999 then ignore it
			if(cost > 0){
				topo_map[source]->neighbors[dest] = cost;
				topo_map[dest]->neighbors[source] = cost;
				edgeCost[source][dest] = cost;
				edgeCost[dest][source] = cost;
			}
		}

		for(int i = 1 ; i < total_node_arrays; i++){//for each node in network
			for(int j = 1; j < total_node_arrays; j++){
				graph_nodes[i].leastCost[j] = edgeCost[i][j];
				if(edgeCost[i][j] > 0 && edgeCost[i][j] < 100)
					graph_nodes[i].path[j] = j;
				else
					if(i != j)
						graph_nodes[i].path[j] = -1;//not reachable
					else
						graph_nodes[i].path[j] = 0;
			}

			for(int m = 1; m < total_node_arrays; m++){
				for(int n = 1; n < total_node_arrays; n++){
					if(i != m)
						graph_nodes[i].costTable[m][n] = INF;
					else
						graph_nodes[i].costTable[m][n] = edgeCost[m][n];
					if(m == n)
						graph_nodes[i].costTable[m][n] = 0;
				}
			}

			//sending DV messages to its neighbors
			for(unordered_map<int, int>::iterator it = topo_map[i]->neighbors.begin();it!= topo_map[i]->neighbors.end();it++){
				int dest = it->first;
				DVmessage message;
				message.source = i;
				message.destination = dest;
				message.thru = i;
				message.distance_vector = graph_nodes[i].costTable[i];
				graph_nodes[dest].msgs.push_back(message);
			}
		}

		for(int node = 1; node < total_node_arrays; node++){
			cost_table(node);
			if(node == total_node_arrays - 1)
				for(int m = 1; m < total_node_arrays; m++)
					if(!graph_nodes[m].msgs.empty())
						node = 0;
		}

		for(int node = 1; node < total_node_arrays; node++){
			Print_Forward_Table(node);
		}

		if (msgfile.is_open()){
			while(getline(msgfile, line)){
				int source, destination;
				sscanf(line.c_str(), "%d %d %*s", &source, &destination);
				message = line.substr(line.find(" ")); //want message to start after second space so do twice
				message_from_file *msg = new message_from_file;
				msg->source = source;
				msg->destination = destination;
				msg->message = message.substr(line.find(" ") + 1);	//just the message
				every_message.push(msg);
			}
		}
		send_messages();
	}
	msgfile.close();
	changefile.close();
}

int main(int argc, char** argv){
	if (argc != 4){
			printf("Usage: ./distvec topofile messagefile changesfile\n");
			return -1;
	}

	topofile = argv[1];
	messagefile = argv[2];
	changesfile = argv[3];
	outfile.open("output.txt"); //output file

	total_nodes = get_topology(); //get initial mapping
	total_node_arrays = total_nodes + 1;
	forward_table.resize(total_node_arrays);

	initialize_nodes();
	for(int node = 1; node < total_node_arrays; node++){
		cost_table(node);
		if(node == total_node_arrays - 1){
			for(int m = 1; m < total_node_arrays; m++){
				if(!graph_nodes[m].msgs.empty())
					node = 0;
			}
		}
	}

	for(int node = 1; node < total_node_arrays; node++){
		Print_Forward_Table(node);
	}

	ifstream msgfile(messagefile);
	string line, message;

	if (msgfile.is_open()){
		while(getline(msgfile, line)){
			int source, destination;
			sscanf(line.c_str(), "%d %d %*s", &source, &destination);
			message = line.substr(line.find(" ")); //want message to start after second space so do twice
			message_from_file *msg = new message_from_file;
			msg->source = source;
			msg->destination = destination;
			msg->message = message.substr(line.find(" ") + 1); //just the message
			every_message.push(msg);
		}
	}
	send_messages();
	update();

	msgfile.close();
	outfile.close();
}

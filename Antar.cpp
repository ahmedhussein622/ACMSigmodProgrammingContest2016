
#include<bits/stdc++.h>
#include <pthread.h>

using namespace std;


#define AdjList unordered_map<int, unordered_set<int> * >


int max_running_bfs = 4;

#define operation_add 1
#define operation_delete 2
#define operation_query 3
struct Query {
	int type, source, target;

	Query(int ty, int s, int t) {
		type = ty;
		source = s;
		target = t;
	}
};

struct Graph {
	AdjList origintal_adjlist;
	AdjList reversed_adjlist;
	queue<Query> operations;
	int id;
	pthread_mutex_t * operations_lock;
};

Graph graph;

queue<Graph * > free_graphs;
vector<Graph * > all_graphs;
pthread_mutex_t free_graphs_mutex;

void initialize_graphs() {

	pthread_mutex_init(& free_graphs_mutex, NULL);
	for(int i = 0; i < max_running_bfs; i++) {
		Graph * g = new Graph();
		g->id = i;
		g->operations_lock = new pthread_mutex_t();
		pthread_mutex_init(g->operations_lock, NULL);

		all_graphs.push_back(g);
		free_graphs.push(g);

	}

}



//vector<pair<int, int>>bfs_results;
pthread_mutex_t resutl_mutex;



priority_queue<pair<int, int>> bfs_results;



void add_edge(int s, int t);
void delete_edge(int s, int t);
void add_edge(int s, int t, Graph * g);
void delete_edge(int s, int t, Graph * g);

void scan_graph() {

	string scan;
	int s, t;

	while (true) {

		getline(cin, scan);

		if (scan.length() == 1) {
			break;
		}
		sscanf(scan.c_str(), "%d %d", &s, &t);
		for(int i = 0; i < (int)all_graphs.size(); i++) {
			add_edge(s, t, all_graphs[i]);
		}
	}

}

struct thread_parameters {
	AdjList * adjlist;
	unordered_map<int, int> * visited1;
	unordered_map<int, int> * visited2;
	int s, t, result;
	pthread_mutex_t * lock1;
	pthread_mutex_t * lock2;
	bool forward;
	bool * found;
};




void * bfs_thread(void * ptr) {

	thread_parameters parm = * ((thread_parameters *) ptr);

	AdjList * adjlist = parm.adjlist;
	unordered_map<int, int> * visited1 = parm.visited1;
	unordered_map<int, int> * visited2 = parm.visited2;
	int s = parm.s;
	int t = parm.t;


//	printf("bfs_thread at %d\n", parm.forward);

	queue<pair<int, int>> q;
	q.push(make_pair(s, 1));

	pthread_mutex_lock(parm.lock1);
	(* visited1)[s] = 1;
	pthread_mutex_unlock(parm.lock1);


	int result = INT_MAX;

	int prev_level = 1;
	while (!q.empty()) {

		pair<int, int> deque = q.front();
		q.pop();

		int p = deque.first;
		int d = deque.second;

//		printf("at %d %d\n", parm.forward, p);

		if(d != prev_level) {
			if(result != INT_MAX || (*parm.found))
				break;
			else
				prev_level = d;
		}

		if (p == t) {
			result = min(result, d - 1);
			break;
		}
		d++;

		unordered_set<int> *y = (*adjlist)[p];

		for (auto it = y->begin(); it != y->end(); it++) {
			int x = *it;

			pthread_mutex_lock(parm.lock1);
			int v = (* visited1)[x];
			pthread_mutex_unlock(parm.lock1);

			if (v == 0) {

				pthread_mutex_lock(parm.lock1);
				(* visited1)[x] = d;
				pthread_mutex_unlock(parm.lock1);

				q.push(make_pair(x, d));
			}

			pthread_mutex_lock(parm.lock2);
			v = (* visited2)[x];
			pthread_mutex_unlock(parm.lock2);

			if(v != 0) {
				result = min(result, (int)( d + v - 2));
//				printf("bingo f : %d, x : %d , d1 : %d, d2 : %d , r : %d\n", parm.forward, x, d, v, result);
			}
		}

	}


	if(result == INT_MAX)
		result = -1;

	*parm.found = true;
	((thread_parameters *) ptr)->result = result;
	return NULL;
}


struct bfs_parameters {
	int s, t, id;
	Graph * graph;
};

void perforem_graph_operations(Graph * graph) {

	bool operations_done = false;
	while(!operations_done) {
		pthread_mutex_lock(graph->operations_lock);
		if(graph->operations.size() == 0) {
			operations_done = true;
		}
		else {
			Query q = graph->operations.front();
			graph->operations.pop();

			if(q.type == operation_query) {
				operations_done = true;
			}
			else if(q.type == operation_add) {
				add_edge(q.source, q.target, graph);
			}
			else if(q.type == operation_delete) {
				delete_edge(q.source, q.target, graph);
			}
		}
		pthread_mutex_unlock(graph->operations_lock);
	}
}

void * bfs(void * ptr) {


	bfs_parameters parm = * ((bfs_parameters *) ptr);

	perforem_graph_operations(parm.graph);
//	printf("bfs go %d\n", parm.id);
	AdjList * forward_graph = & parm.graph->origintal_adjlist;
	AdjList *  reverse_graph = & parm.graph->reversed_adjlist;

	int result = -1;

	if(!(forward_graph->find(parm.s) == forward_graph->end()
			|| forward_graph->find(parm.t) == forward_graph->end())) {

//		printf("ggggg %d %d %d\n", parm.id, parm.forward_graph->size(), parm.reverse_graph->size());
		unordered_map<int, int> visited1;
		unordered_map<int, int> visited2;

		pthread_mutex_t lock1;
		pthread_mutex_t lock2;

		pthread_mutex_init(& lock1, NULL);
		pthread_mutex_init(& lock2, NULL);


		thread_parameters forward;
		forward.adjlist = forward_graph;
		forward.visited1 = & visited1;
		forward.visited2 = & visited2;
		forward.s = parm.s;
		forward.t = parm.t;
		forward.forward = true;
		forward.lock1 = & lock1;
		forward.lock2 = & lock2;

		thread_parameters reverse;
		reverse.adjlist = reverse_graph;
		reverse.visited1 = & visited2;
		reverse.visited2 = & visited1;
		reverse.s = parm.t;
		reverse.t = parm.s;
		reverse.forward = false;
		reverse.lock1 = & lock2;
		reverse.lock2 = & lock1;

		bool found = false;

		forward.found = & found;
		reverse.found = & found;

//		printf("here %d %d %d\n", parm.id, parm.s, parm.t);

		pthread_t thread1, thread2;
		pthread_create( &thread1, NULL, bfs_thread, (void*) & forward);
		pthread_create( &thread2, NULL, bfs_thread, (void*) & reverse);


		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);

		pthread_mutex_destroy(& lock1);
		pthread_mutex_destroy(& lock2);

		int d1 = forward.result;
		int d2 = reverse.result;

//		printf("%d %d\n",d1, d2);
		if(d1 == -1 && d2 == -1) {
			result = -1;
		}
		else {
			if(d1 == -1) {
				result = d2;
			}
			else if(d2 == -1) {
				result = d1;
			}
			else {
				result = min(d1, d2);
			}
		}

	}


	pthread_mutex_lock(& free_graphs_mutex);
	free_graphs.push(parm.graph);
	pthread_mutex_unlock(& free_graphs_mutex);

	pthread_mutex_lock(& resutl_mutex);
//	bfs_results.push_back(make_pair(parm.id, result));
	bfs_results.push(make_pair(-parm.id, result));
	pthread_mutex_unlock(& resutl_mutex);



//	printf("done %d %d\n", parm.s, parm.t);


	return NULL;
}



void add_edge(int s, int t,  AdjList & adjlist) {

	if(adjlist.find(s) == adjlist.end()) {
		adjlist[s] = new unordered_set<int>();
	}
	if(adjlist.find(t) == adjlist.end()) {
		adjlist[t] = new unordered_set<int>();
	}

	unordered_set<int> *y = adjlist[s];
	y->insert(t);

}

void add_edge(int s, int t, Graph * g) {
	add_edge(s, t, g->origintal_adjlist);
	add_edge(t, s, g->reversed_adjlist);
}


void add_edge(int s, int t) {

	for(int i = 0; i < (int)all_graphs.size(); i++) {
		Graph * g =  all_graphs[i];
		pthread_mutex_lock(g->operations_lock);
		g->operations.push(Query(operation_add, s, t));
		pthread_mutex_unlock(g->operations_lock);
	}

}



void delete_edge(int s, int t, AdjList & adjlist) {
	if (adjlist.find(s) != adjlist.end() && adjlist.find(t) != adjlist.end()) {
		unordered_set<int> *y = adjlist[s];
		y->erase(t);
	}
}

void delete_edge(int s, int t, Graph * g) {
	delete_edge(s, t, g->origintal_adjlist);
	delete_edge(t, s, g->reversed_adjlist);
}

void delete_edge(int s, int t) {
	for(int i = 0; i < (int)all_graphs.size(); i++) {
		Graph * g =  all_graphs[i];
		pthread_mutex_lock(g->operations_lock);
		g->operations.push(Query(operation_delete, s, t));
		pthread_mutex_unlock(g->operations_lock);
	}
}




int scan_batch() {
	string scan;
	int s, t;

	getline(cin, scan);
	if(scan.length() == 0)
		return EOF;

	pthread_mutex_init(& resutl_mutex, NULL);



	int c = 0;

	while (true) {


		if (scan.length() == 1) {
			break;
		}


		char op;
		sscanf(scan.c_str(), "%c %d %d", &op, &s, &t);



		if(op == 'Q') {
			bfs_parameters * parm = new bfs_parameters();
			parm->s = s;
			parm->t = t;
			parm->id = c++;


			pthread_t t;
			Graph * g;

			bool free_graph_found = false;
			while(!free_graph_found) {
				pthread_mutex_lock(& free_graphs_mutex);
				if(free_graphs.size() > 0) {
					g = free_graphs.front();
					free_graphs.pop();
					free_graph_found = true;
				}
				pthread_mutex_unlock(& free_graphs_mutex);
			}

			parm->graph = g;
			pthread_mutex_lock(g->operations_lock);
			g->operations.push(Query(operation_query, s, t));
			pthread_mutex_unlock(g->operations_lock);

			pthread_create(& t, NULL , bfs, (void *)  parm);


		}
		else if(op == 'A') {
			add_edge(s, t);
		}
		else if(op == 'D'){
			delete_edge(s, t);
		}


		getline(cin, scan);
	}


	int next_result = 0;
	while(next_result < c) {
		pthread_mutex_lock( & resutl_mutex);
		if(!bfs_results.empty()) {
			pair<int, int> r = bfs_results.top();
//			printf("here %d %d %d\n", r.first, r.second, bfs_results.size());
			if(-r.first == next_result) {
				bfs_results.pop();
				next_result++;
				cout<<r.second<<"\n";
			}
		}
		pthread_mutex_unlock(& resutl_mutex);
	}


//	bool batch_processed = false;
//	while(!batch_processed) {
//		pthread_mutex_lock( & resutl_mutex);
//		if(bfs_results.size() == c) {
//			batch_processed = true;
//		}
//		pthread_mutex_unlock(& resutl_mutex);
//	}

//	sort(bfs_results.begin(), bfs_results.end());
//	for(int i = 0; i < (int)bfs_results.size(); i++) {
//		cout<<bfs_results[i].second<<"\n";
//	}
//	bfs_results.clear();


//	while(!bfs_results.empty()) {
//		pair<int, int> r = bfs_results.top();
//		bfs_results.pop();
//		printf("%d %d\n", r.first, r.second);
//	}

	pthread_mutex_destroy(& resutl_mutex);

	return 1;
}



int main() {

//	freopen("input.txt", "r", stdin);

	initialize_graphs();

	scan_graph();
	cout<<"R\n";

	while(scan_batch() != EOF);

	return 0;
}




















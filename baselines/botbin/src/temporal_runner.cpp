#include "Graph.h"
#include "Utility.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

struct Op { int window; long long time; string kind; int u; int v; };
struct Win { long long time = 0; vector<pair<int,int>> del, ins; };

static vector<string> split_tab(const string& s) {
    vector<string> a; string x; stringstream ss(s);
    while (getline(ss, x, '\t')) a.push_back(x);
    return a;
}

static void write_graph(const string& dir, int n, const vector<pair<int,int>>& initial) {
    fs::create_directories(dir);
    vector<vector<int>> adj(n);
    for (auto [u,v] : initial) {
        if (u < 0 || v < 0 || u >= n || v >= n || u == v) continue;
        adj[u].push_back(v); adj[v].push_back(u);
    }
    unsigned long long directed = 0; int maxdeg = 0;
    for (auto& a : adj) {
        sort(a.begin(), a.end());
        a.erase(unique(a.begin(), a.end()), a.end());
        directed += a.size(); maxdeg = max(maxdeg, (int)a.size());
    }
    ofstream dat(dir + "/graph.dat", ios::binary);
    ofstream idx(dir + "/graph.idx", ios::binary);
    long pos = 0;
    for (int u = 0; u < n; ++u) {
        int deg = (int)adj[u].size();
        idx.write(reinterpret_cast<const char*>(&pos), sizeof(long));
        idx.write(reinterpret_cast<const char*>(&deg), sizeof(int));
        for (int v : adj[u]) dat.write(reinterpret_cast<const char*>(&v), sizeof(int));
        pos += deg;
    }
    dat.close(); idx.close();
    ofstream info(dir + "/graph.info", ios::binary);
    unsigned int undirected = (unsigned int)(directed / 2);
    info.write(reinterpret_cast<const char*>(&n), sizeof(int));
    info.write(reinterpret_cast<const char*>(&undirected), sizeof(unsigned int));
    info.write(reinterpret_cast<const char*>(&maxdeg), sizeof(int));
    info.close();
}

static int arg_int(int argc, char** argv, const string& key, int def) {
    for (int i=1;i+1<argc;i++) if (argv[i] == key) return stoi(argv[i+1]);
    return def;
}
static double arg_double(int argc, char** argv, const string& key, double def) {
    for (int i=1;i+1<argc;i++) if (argv[i] == key) return stod(argv[i+1]);
    return def;
}
static string arg_str(int argc, char** argv, const string& key, const string& def) {
    for (int i=1;i+1<argc;i++) if (argv[i] == key) return argv[i+1];
    return def;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "usage: temporal_runner --manifest FILE --run-dir DIR [--max-windows N] [--rho R] [--delta D] [--epsilon E] [--mu M]\n";
        return 2;
    }
    string manifest = arg_str(argc, argv, "--manifest", "");
    string run_dir = arg_str(argc, argv, "--run-dir", "");
    int max_windows = arg_int(argc, argv, "--max-windows", 0);
    int delta = arg_int(argc, argv, "--delta", 100);
    int mu = arg_int(argc, argv, "--mu", 5);
    double rho = arg_double(argc, argv, "--rho", 0.10);
    float epsilon = (float)arg_double(argc, argv, "--epsilon", 0.5);
    if (manifest.empty() || run_dir.empty()) return 2;

    ifstream in(manifest);
    if (!in) { cerr << "cannot open manifest " << manifest << "\n"; return 3; }
    map<int,Win> windows;
    vector<pair<int,int>> initial;
    int vertex_count = -1;
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        vector<string> f = split_tab(line);
        if (f.size() >= 3 && f[0] == "meta" && f[1] == "vertex_count") {
            vertex_count = stoi(f[2]); continue;
        }
        if (f.size() < 5 || f[0] == "format" || f[0] == "columns" || f[0] == "meta" || f[0].empty() || !isdigit((unsigned char)f[0][0])) continue;
        int wid = stoi(f[0]); long long wt = stoll(f[1]);
        string op = f[2];
        if (op == "WINDOW") { windows[wid].time = wt; continue; }
        int u = stoi(f[3]), v = stoi(f[4]);
        if (u > v) swap(u,v);
        windows[wid].time = wt;
        if (op == "INITIAL") initial.push_back({u,v});
        else if (op == "DELETE") windows[wid].del.push_back({u,v});
        else if (op == "INSERT") windows[wid].ins.push_back({u,v});
        if (vertex_count < 0) vertex_count = max(vertex_count, max(u,v)+1);
    }
    if (vertex_count < 0) { cerr << "no vertices\n"; return 4; }
    string graph_dir = run_dir + "/graph";
    string result_dir = run_dir + "/results";
    fs::create_directories(result_dir);
    write_graph(graph_dir, vertex_count, initial);

    cerr << "manifest=" << manifest << " vertices=" << vertex_count
         << " initial_edges=" << initial.size() << " windows=" << windows.size()
         << " rho=" << rho << " delta=" << delta << " epsilon=" << epsilon << " mu=" << mu << "\n";

    auto t0 = chrono::steady_clock::now();
    Graph* g = new Graph(graph_dir + "/", rho);
    g->build_index();
    auto t1 = chrono::steady_clock::now();
    g->load_index(delta);
    auto t2 = chrono::steady_clock::now();
    ofstream csv(run_dir + "/timing.csv");
    csv << "window_id,window_time,delete_count,insert_count,update_ns,query_ns,result_file\n";
    int processed = 0;
    for (auto const& kv : windows) {
        if (max_windows > 0 && processed >= max_windows) break;
        int wid = kv.first; const Win& w = kv.second;
        long long update_ns = 0, query_ns = 0;
        auto us = chrono::steady_clock::now();
        for (auto [u,v] : w.del) g->apply_delete(u,v);
        for (auto [u,v] : w.ins) g->apply_insert(u,v);
        auto ue = chrono::steady_clock::now();
        update_ns = chrono::duration_cast<chrono::nanoseconds>(ue-us).count();
        string result = result_dir + "/window_" + to_string(wid) + ".txt";
        auto qs = chrono::steady_clock::now();
        g->run_query(epsilon, mu, result, wid+1);
        auto qe = chrono::steady_clock::now();
        query_ns = chrono::duration_cast<chrono::nanoseconds>(qe-qs).count();
        csv << wid << "," << w.time << "," << w.del.size() << "," << w.ins.size()
            << "," << update_ns << "," << query_ns << "," << result << "\n";
        csv.flush();
        cerr << "window=" << wid << " deletes=" << w.del.size() << " inserts=" << w.ins.size()
             << " update_ns=" << update_ns << " query_ns=" << query_ns << "\n";
        processed++;
    }
    csv.close();
    auto t3 = chrono::steady_clock::now();
    ofstream meta(run_dir + "/RUN_COMPLETE");
    meta << "processed_windows=" << processed << "\n";
    meta << "index_build_ns=" << chrono::duration_cast<chrono::nanoseconds>(t1-t0).count() << "\n";
    meta << "index_load_ns=" << chrono::duration_cast<chrono::nanoseconds>(t2-t1).count() << "\n";
    meta << "wall_ns=" << chrono::duration_cast<chrono::nanoseconds>(t3-t0).count() << "\n";
    meta.close();
    return 0;
}

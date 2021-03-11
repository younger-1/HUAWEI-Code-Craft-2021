#include <cmath>
#include <iterator>
#include <numeric>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// 服务器信息
// unordered_map<string, vector<int>> serverInfos;

vector<string> bestServers(const unordered_map<string, vector<int>> &serverInfos, const vector<int> &requestCpu,
                           const vector<int> &requestMemory)
{
    int totalCpu = accumulate(requestCpu.begin(), requestCpu.end(), 0);
    int totalMemory = accumulate(requestMemory.begin(), requestMemory.end(), 0);
    double cpuToMemory = (double)totalCpu / totalMemory;
    // todo: priority_queue<pair<string, vector<double>>> a;
    vector<pair<string, vector<double>>> uniformedServers;
    for (auto server : serverInfos)
    {
        vector<double> uniInfo;
        double uniCpu = (double)server.second[0] / server.second[2] * 1000;
        double uniMemory = (double)server.second[1] / server.second[2] * 1000;
        double projection = (uniMemory * 1 + uniCpu * cpuToMemory) / (1 + sqrt(cpuToMemory * cpuToMemory));
        uniInfo.push_back(projection);
        // ? make_pair()
        uniformedServers.emplace_back(server.first, uniInfo);
    }
    sort(uniformedServers.begin(), uniformedServers.end(),
         [](pair<string, vector<double>> a, pair<string, vector<double>> b) { return a.second[2] > b.second[2]; });
    return {uniformedServers.begin(), uniformedServers.begin() + 10};
}

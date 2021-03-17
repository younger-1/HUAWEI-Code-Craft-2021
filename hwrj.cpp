#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <stdio.h>
#include <time.h>
#include <unordered_map>
#include <vector>
//#include<bits/stdc++.h>

using namespace std;

#define TEST

/*************************************************
Name: Structure
Description:
**************************************************/

// 存在的服务器信息
struct ExistServer
{
    int remainCpuA, remainCpuB, remainMemA, remainMemB;
    float CM_Ratio_A, CM_Ratio_B;
    string serverType;
};

// 存在的虚拟机信息
struct ExistVM
{
    string vmType;
    int serverIndex;
    bool inA;
};

// eg. (add, c3.large.4, 5), (del, 2)
struct Request
{
    bool isAdd;
    string vmID;
    string vmType;
    // char id[28];
    // char name[32];
    Request(bool _isAdd, string _id, string _type)
    {
        isAdd = _isAdd;
        vmID = _id;
        vmType = _type;
    }
};

struct ServerRatio
{
    float uCpu;
    float uMemory;
    float ratio;
    float projection;
};

/*************************************************
Name: Global variables
Description:
**************************************************/

#ifdef TEST
int priceSum = 0, dayCostSum = 0;
#endif // TEST

// serverInfo[serverType] = {cpu, memory, price, dayCost};
// eg. (NV603, 8, 32, 53800, 500)
unordered_map<string, vector<int>> serverInfo;

// vmInfo[vmType] = {cpu, memory, isTwoNode};
// eg. (c3.large.4, 4, 16, 0)
unordered_map<string, vector<int>> vmInfo;

// vmID => ExistVM
unordered_map<string, ExistVM> existVM;

// serverIndex => ExistServer
unordered_map<int, ExistServer> existServer;

// 当前队列待选服务器型号（必须包含容量大于任意请求的服务器大小）
vector<string> readyServer = {"NV603"};

// 当次队列购买的服务器
unordered_map<string, int> serverApplyNumber;

// 当次队列购买的服务器总数
int curServerCnt = 0;

// 服务器总数
int serverCnt = 0;

// 当前最匹配服务器
string curNeedServer;

// 当次队列申请的虚拟机与服务器与迁移输出信息
vector<string> vmApplyInfo, serverapplyInfo, moveInfo;

vector<Request> dayRequests;

vector<vector<Request>> allRequests;

/*************************************************
Name: Unused variables
Description:
**************************************************/

// 统计所有虚拟机的CPU，MEMORY需求
// vmID => {CPU, MEM}
unordered_map<string, vector<int>> needList;

// 上两日操作消息
vector<string> infoLog;

/*************************************************
Name: Utils functions
Description:
**************************************************/

const char digit[11] = "0123456789";
string int2str(int num)
{
    if (num == 0)
        return "0";
    string ret = "";
    while (num)
    {
        ret += digit[num % 10];
        num /= 10;
    }
    reverse(ret.begin(), ret.end());
    return ret;
}

// 生成分配信息
void returnInfo(int index, char kind = 'D')
{
    string vmaly = "";
    if (kind == 'A')
    {
        vmaly += "(";
        vmaly += int2str(index);
        vmaly += ", ";
        vmaly += "A)";
        vmApplyInfo.emplace_back(vmaly);
    }
    else if (kind == 'B')
    {
        vmaly += "(";
        vmaly += int2str(index);
        vmaly += ", ";
        vmaly += "B)";
        vmApplyInfo.emplace_back(vmaly);
    }
    else
    {
        vmaly += "(";
        vmaly += int2str(index);
        vmaly += ")";
        vmApplyInfo.emplace_back(vmaly);
    }
}

/*************************************************
Name: Core algorithms
Description: Scheduling and optimization
**************************************************/

// 指定服务器分配
bool addNewServer(string &_vmType, string &_vmID, int _index)
{
    vector<int> &vm = vmInfo[_vmType];
    ExistServer &es = existServer[_index];
    if (vm[2])
    {
        int c = vm[0] / 2, m = vm[1] / 2;
        if (es.remainCpuA >= c && es.remainCpuB >= c && es.remainMemA >= m && es.remainMemB >= m)
        {
            // 更新服务器信息
            es.remainCpuA -= c;
            es.remainCpuB -= c;
            es.remainMemA -= m;
            es.remainMemB -= m;
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
            es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

            // 创建虚拟机信息
            existVM[_vmID].vmType = _vmType;
            existVM[_vmID].serverIndex = _index;

            // 申请虚拟机信息存储
            returnInfo(_index);

            return true;
        }
    }
    // 单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else if (es.remainCpuA >= vm[0] && es.remainMemA >= vm[1])
    {
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        es.remainCpuA -= vm[0];
        es.remainMemA -= vm[1];
        es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;

        existVM[_vmID].vmType = _vmType;
        existVM[_vmID].inA = 1;
        existVM[_vmID].serverIndex = _index;

        returnInfo(_index, 'A');

        return true;
    }
    return false;
}

// 在已有服务器上分配或删除
bool redistribution(Request &r)
{
    // 如果是删除操作
    if (!r.isAdd)
    {
        // 存在的虚拟机信息
        ExistVM &ev = existVM[r.vmID];
        // 存在的服务器
        ExistServer &es = existServer[ev.serverIndex];
        // 存在的虚拟机格式
        const vector<int> &vm = vmInfo[ev.vmType];

        if (vm[2])
        {
            // 更新服务器信息
            es.remainCpuA += vm[0] / 2;
            es.remainCpuB += vm[0] / 2;
            es.remainMemA += vm[1] / 2;
            es.remainMemB += vm[1] / 2;
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
            es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;
        }
        else if (ev.inA)
        {
            es.remainCpuA += vm[0];
            es.remainMemA += vm[1];
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
        }
        else
        {
            es.remainCpuB += vm[0];
            es.remainMemB += vm[1];
            es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;
        }

// 统计日均损耗
#ifdef TEST
        if (serverInfo[es.serverType][0] == es.remainCpuA + es.remainCpuB)
            dayCostSum -= serverInfo[es.serverType][3];
#endif // TEST

        // 删除虚拟机信息
        existVM.erase(r.vmID);
        return true;
    }

    // 如果是添加操作
    // 如果双端
    const vector<int> &vm = vmInfo[r.vmType];
    int c = vm[0] / 2, m = vm[1] / 2;
    if (vm[2])
    {
        for (int i = 0; i < serverCnt; ++i)
        {
            ExistServer &es = existServer[i];
            if (es.remainCpuA >= c && es.remainCpuB >= c && es.remainMemA >= m && es.remainMemB >= m)
            {

// 统计日均损耗
#ifdef TEST
                // todo: ?
                if (serverInfo[es.serverType][0] == es.remainCpuA + es.remainCpuB)
                    dayCostSum += serverInfo[es.serverType][3];
#endif // TEST

                // 更新服务器信息
                es.remainCpuA -= c;
                es.remainCpuB -= c;
                es.remainMemA -= m;
                es.remainMemB -= m;
                es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
                es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

                // 创建虚拟机信息
                existVM[r.vmID].vmType = r.vmType;
                existVM[r.vmID].serverIndex = i;

                // 申请虚拟机信息存储
                returnInfo(i);
                return true;
            }
        }
    }
    // 单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else
    {
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        for (int i = 0; i < serverCnt; ++i)
        {
            ExistServer &es = existServer[i];
            if (es.remainCpuA >= c && es.remainMemA >= m)
            {

// 统计日均损耗
#ifdef TEST
                if (serverInfo[es.serverType][0] == es.remainCpuA + es.remainCpuB)
                    dayCostSum += serverInfo[es.serverType][3];
#endif // TEST

                es.remainCpuA -= c;
                es.remainMemA -= m;
                es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;

                existVM[r.vmID].vmType = r.vmType;
                existVM[r.vmID].inA = 1;
                existVM[r.vmID].serverIndex = i;

                returnInfo(i, 'A');
                return true;
            }
            else if (es.remainCpuB >= c && es.remainMemB >= m)
            {
// 统计日均损耗
#ifdef TEST
                if (serverInfo[es.serverType][0] == es.remainCpuA + es.remainCpuB)
                    dayCostSum += serverInfo[es.serverType][3];
#endif // TEST

                es.remainCpuB -= c;
                es.remainMemB -= m;
                es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

                existVM[r.vmID].vmType = r.vmType;
                existVM[r.vmID].inA = 0;
                existVM[r.vmID].serverIndex = i;

                returnInfo(i, 'B');
                return true;
            }
        }
    }
    return false;
}

// 每个请求队列分配服务器
void applyServer()
{
    int choseServer = 0;
    string curNeedServer;
    for (int i = 0; i < dayRequests.size(); ++i)
    {
        // 如果存在的服务器分配未成功，则再分配
        if (!redistribution(dayRequests[i]))
        {
            do
            {
                curNeedServer = readyServer[choseServer];
                int c = serverInfo[curNeedServer][0] / 2, m = serverInfo[curNeedServer][1] / 2;
                float cm = c * 1.0 / m;
                existServer[serverCnt].serverType = curNeedServer;
                existServer[serverCnt].remainCpuA = c;
                existServer[serverCnt].remainCpuB = c;
                existServer[serverCnt].remainMemA = m;
                existServer[serverCnt].remainMemB = m;
                existServer[serverCnt].CM_Ratio_A = cm;
                existServer[serverCnt].CM_Ratio_B = cm;
                choseServer++;
            } while (!addNewServer(dayRequests[i].vmType, dayRequests[i].vmID, serverCnt));

            serverApplyNumber[curNeedServer]++;
            curServerCnt++;

#ifdef TEST
            dayCostSum += serverInfo[curNeedServer][3];
            priceSum += serverInfo[curNeedServer][2];
#endif // TEST

            // choseServer = 0;
            serverCnt++;
        }
    }
}

void allApplyServer()
{
    string serverType = readyServer[0];
    int serverIndex = 0, _cpu = 0, _mem = 0;
    float _CM = 0.0;
    for (int i = 0; i < allRequests.size(); i++)
    {
        auto &_dayRequests = allRequests[i];
        for (int j = 0; j < _dayRequests.size(); j++)
        {
            if (!redistribution(_dayRequests[j]))
            {
                _cpu = serverInfo[serverType][0] / 2, _mem = serverInfo[serverType][1] / 2;
                _CM = _cpu * 1.0 / _mem;
                existServer[serverIndex].serverType = serverType;
                existServer[serverIndex].remainCpuA = _cpu;
                existServer[serverIndex].remainCpuB = _cpu;
                existServer[serverIndex].remainMemA = _mem;
                existServer[serverIndex].remainMemB = _mem;
                existServer[serverIndex].CM_Ratio_A = _CM;
                existServer[serverIndex].CM_Ratio_B = _CM;

                addNewServer(_dayRequests[j].vmType, _dayRequests[j].vmID, serverIndex);

                serverApplyNumber[serverType]++;
                serverIndex += 1;
                serverCnt += 1;
            }
        }
    }
}

/*************************************************
Name: Pick server
Description:
**************************************************/

// Pair of <serverType, ServerRatio>
vector<pair<string, ServerRatio>> uniformedServers;

void initUniformedServers()
{
    // todo: priority_queue<pair<string, vector<double>>> a;
    for (auto server : serverInfo)
    {
        float uCpu = 1.0 * server.second[0] / server.second[2] * 1000;
        float uMemory = 1.0 * server.second[1] / server.second[2] * 1000;
        ServerRatio tmp = {uCpu, uMemory, uCpu / uMemory, 0};
        // ? move
        uniformedServers.emplace_back(server.first, move(tmp));
    }
    // 按 ratio 从小到大
    sort(uniformedServers.begin(), uniformedServers.end(),
         [](pair<string, ServerRatio> &a, pair<string, ServerRatio> &b) { return a.second.ratio < b.second.ratio; });
}

// 找到最优服务器列表
// todo: dayLoss
vector<string> bestServers(float CM_Ratio, int maxCpu, int maxMemory)
{
    // usage: lower_bound(beg, end, val, comp)
    auto findIter =
        lower_bound(uniformedServers.begin(), uniformedServers.end(), CM_Ratio,
                    [](pair<string, ServerRatio> &server, float CM_R) { return server.second.ratio < CM_R; });

    decltype(findIter) startIter = uniformedServers.begin();
    decltype(findIter) endIter = uniformedServers.end();

    int totalServer = uniformedServers.size();
    int returnSize = totalServer > 50 ? (totalServer / 10) : totalServer < 5 ? totalServer : 5;

    int searchScale = 3;
    if (findIter == uniformedServers.begin() && searchScale * returnSize < totalServer)
    {
        endIter = uniformedServers.begin() + searchScale * returnSize;
    }
    else if (findIter == uniformedServers.end() && searchScale * returnSize < totalServer)
    {
        startIter = uniformedServers.end() - searchScale * returnSize;
    }
    else
    {
        if (findIter - uniformedServers.begin() > returnSize)
            startIter = findIter - returnSize;
        if (uniformedServers.end() - findIter > returnSize)
            endIter = findIter + returnSize;
    }

    for (auto it = startIter; it != endIter; ++it)
    {
        it->second.projection = (it->second.uMemory * 1 + it->second.uCpu * CM_Ratio) / sqrt(1 + CM_Ratio * CM_Ratio);
    }

    vector<size_t> bestServersIndex;
    bestServersIndex.reserve(distance(startIter, endIter));

    for (auto it = startIter; it != endIter; ++it)
    {
        if (serverInfo[it->first][0] < (maxCpu << 1) || serverInfo[it->first][1] < (maxMemory << 1))
            continue;
        bestServersIndex.push_back(it - uniformedServers.begin());
    }
    sort(bestServersIndex.begin(), bestServersIndex.end(),
         [&](int a, int b) { return uniformedServers[a].second.projection > uniformedServers[b].second.projection; });

    vector<string> bestServers;
    bestServers.reserve(bestServersIndex.size());
    for (auto i : bestServersIndex)
    {
        bestServers.push_back(uniformedServers[i].first);
    }
    return bestServers;
}

/*************************************************
Name: Input
Description: Read stdin
**************************************************/

// 读取函数
int readSingleNum()
{
    char c;
    int ret = 0;
    while ((c = getchar()) != '\n')
    {
        ret = (ret << 3) + (ret << 1) + c - '0';
    }
    return ret;
}

int readNum()
{
    char c;
    int ret = 0;
    while ((c = getchar()) != ',')
    {
        ret = (ret << 3) + (ret << 1) + c - '0';
    }
    return ret;
}

int n, m;
// 读入数据临时变量
string serverType, vmType, vmID;
// 读入存储临时变量
int cpu, memory, price, loss, isTwoNode, isAdd;
// 当前决策需要的CPU和Memory
int dayNeedCpu = 0, dayNeedMem = 0, maxC = 0, maxM = 0;

int allNeedCpu = 0, allNeedMem = 0;

// 读取请求
void readRequest()
{
    char c;
    vmType = "";
    vmID = "";
    getchar();
    if ((isAdd = (c = getchar()) == 'a'))
    {
        while ((c = getchar()) != ',')
            ;
        getchar();
        while ((c = getchar()) != ',')
            vmType += c;
        getchar();
        while ((c = getchar()) != ')')
            vmID += c;
        // needList[vmID] = {vmInfo[vmType][0], vmInfo[vmType][1]};
        // dayNeedCpu += needList[vmID][0];
        // dayNeedMem += needList[vmID][1];
        // allNeedCpu += needList[vmID][0];
        // allNeedMem += needList[vmID][1];
        // maxC = max(maxC, needList[vmID][0]);
        // maxM = max(maxM, needList[vmID][1]);

        allNeedCpu += vmInfo[vmType][0];
        allNeedMem += vmInfo[vmType][1];

        // 虚拟机在单个节点占用的最大资源
        maxC = max(maxC, vmInfo[vmType][2] ? vmInfo[vmType][0] / 2 : vmInfo[vmType][0]);
        maxM = max(maxM, vmInfo[vmType][2] ? vmInfo[vmType][1] / 2 : vmInfo[vmType][1]);
    }
    else
    {
        while ((c = getchar()) != ',')
            ;
        getchar();
        while ((c = getchar()) != ')')
            vmID += c;

        // dayNeedCpu -= needList[vmID][0];
        // dayNeedMem -= needList[vmID][1];

        allNeedCpu -= needList[vmID][0];
        allNeedMem -= needList[vmID][1];

        needList.erase(vmID);
    }
    getchar();
    dayRequests.emplace_back(isAdd, vmID, vmType);
}

// 按行读取虚拟机信息
void readVMInfo()
{
    char c;
    vmType = "";
    getchar();
    while ((c = getchar()) != ',')
        vmType += c;
    getchar();
    cpu = readNum();
    getchar();
    memory = readNum();
    getchar();
    isTwoNode = getchar() == '1';
    getchar();
    getchar();
    vmInfo[vmType] = {cpu, memory, isTwoNode};
}

// 按行读取服务器信息
void readServerInfo()
{
    char c;
    serverType = "";
    getchar();
    while ((c = getchar()) != ',')
        serverType += c;
    getchar();
    cpu = readNum();
    getchar();
    memory = readNum();
    getchar();
    price = readNum();
    getchar();
    loss = 0;
    while ((c = getchar()) != ')')
    {
        loss = (loss << 3) + (loss << 1) + c - '0';
    }
    getchar();
    serverInfo[serverType] = {cpu, memory, price, loss};
}

/*************************************************
Name: Output
Description: Write stdout
**************************************************/

// 生成服务器购买信息
void rserverInfo(const string &name, int num)
{
    string rsi = "";
    rsi += '(';
    rsi += name;
    rsi += ", ";
    rsi += int2str(num);
    rsi += ')';
    serverapplyInfo.emplace_back(rsi);
}

// 统计输出的服务器信息
void serverCensus()
{
    string sss = "purchase";
    rserverInfo(sss, serverApplyNumber.size());
    for (auto s : serverApplyNumber)
    {
        rserverInfo(s.first, s.second);
    }
    serverApplyNumber.clear();
    curServerCnt = 0;
}

// 统计迁移信息
void moveCensus()
{
    string ret = "(migration, 0)";
    moveInfo.emplace_back(ret);
}

// 统一输出信息
void infoOut()
{
    for (int i = 0; i < serverapplyInfo.size(); ++i)
        cout << serverapplyInfo[i] << endl;
    serverapplyInfo.clear();
    for (int i = 0; i < moveInfo.size(); ++i)
        cout << moveInfo[i] << endl;
    moveInfo.clear();
    for (int i = 0; i < vmApplyInfo.size(); ++i)
        cout << vmApplyInfo[i] << endl;
    vmApplyInfo.clear();
}

/*************************************************
Name: Main process
Description:
**************************************************/

void processIO()
{
#ifdef TEST
    string inputFile = "training.txt";
    string outputFile = "output.txt";
    freopen(inputFile.c_str(), "rb", stdin);
    freopen(outputFile.c_str(), "wb", stdout);
#endif // TEST

    n = readSingleNum();
    for (int i = 0; i < n; ++i)
    {
        readServerInfo();
    }
    initUniformedServers();

    n = readSingleNum();
    for (int i = 0; i < n; ++i)
    {
        readVMInfo();
    }

    n = readSingleNum();
    for (int i = 0; i < n; ++i)
    {
        m = readSingleNum();

        // 重置需求队列
        // dayNeedCpu = 0;
        // dayNeedMem = 0;
        // maxC = 0;
        // maxM = 0;

        for (int j = 0; j < m; ++j)
        {
            readRequest();
        }
        // float dayNeed_CM_Ratio = dayNeedCpu * 1.0 / dayNeedMem;
        // readyServer = bestServers(dayNeed_CM_Ratio, maxC, maxM);

        // applyServer();

        // serverCensus();
        // moveCensus();
        // infoOut();

        // #ifdef TEST
        //         priceSum += dayCostSum;
        // #endif // TEST

        // ? move()
        allRequests.push_back(dayRequests);
        dayRequests.clear();
    }

    float all_CM_Ratio = allNeedCpu * 1.0 / allNeedMem;
    readyServer = bestServers(all_CM_Ratio, maxC, maxM);

    allApplyServer();

    serverCensus();
    moveCensus();
    infoOut();

    // #ifdef TEST
    //         priceSum += dayCostSum;
    // #endif // TEST

#ifdef TEST
    cout << priceSum << endl;
#endif // TEST
}

int main()
{
    processIO();
    return 0;
}

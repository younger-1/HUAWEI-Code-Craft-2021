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
    string name;
};

// 存在的虚拟机信息
struct ExistVM
{
    bool isA;
    int serverID;
    string name;
};

struct Request
{
    bool isAdd;
    string id;
    string name;
    // char id[28];
    // char name[32];
    Request(bool a, string _id, string _name)
    {
        isAdd = a;
        id = _id;
        name = _name;
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
int priceSum = 0, dayCost = 0;
#endif // TEST

// serverInfo[name] = {cpu, G, price, loss};
unordered_map<string, vector<int>> serverInfo;
unordered_map<string, vector<int>> vmInfo;
unordered_map<string, ExistVM> existVM;
unordered_map<int, ExistServer> existServer;
// 统计当前所需CPU，MEMORY所用map
unordered_map<string, vector<int>> needList;
// 当前队列待选服务器型号（必须包含容量大于任意请求的服务器大小）
vector<string> readyServer = {"NV603"};

// 当次队列购买的服务器
unordered_map<string, int> serverapply;
// 当次队列购买的服务器总数
int serverCurCnt = 0;

// 服务器总数
int serverCnt = 0;
// 当前最匹配服务器
string curNeedServer;
// 当次队列申请的虚拟机与服务器与迁移输出信息
vector<string> VMapplyInfo, serverapplyInfo, moveInfo;

vector<Request> requests;

/*************************************************
Name: Unused variables
Description:
**************************************************/

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
void returnInfo(int id, char kind = 'D')
{
    string vmaly = "";
    if (kind == 'A')
    {
        vmaly += "(";
        vmaly += int2str(id);
        vmaly += ", ";
        vmaly += "A)";
        VMapplyInfo.emplace_back(vmaly);
    }
    else if (kind == 'B')
    {
        vmaly += "(";
        vmaly += int2str(id);
        vmaly += ", ";
        vmaly += "B)";
        VMapplyInfo.emplace_back(vmaly);
    }
    else
    {
        vmaly += "(";
        vmaly += int2str(id);
        vmaly += ")";
        VMapplyInfo.emplace_back(vmaly);
    }
}

/*************************************************
Name: Core algorithms
Description: Scheduling and optimization
**************************************************/

// 指定服务器分配
bool addNewServer(string &vmName, int id, string &vmID)
{
    vector<int> &vm = vmInfo[vmName];
    ExistServer &es = existServer[id];
    if (vm[2])
    {
        int c = vm[0] / 2, m = vm[1] / 2;
        if (es.remainCpuA >= c && es.remainCpuB >= c && es.remainMemA >= m && es.remainMemB >= m)
        {
            es.remainCpuA -= c;
            es.remainCpuB -= c;
            es.remainMemA -= m;
            es.remainMemB -= m;
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
            es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

            existVM[vmID].name = vmName;
            existVM[vmID].isA = 1;
            existVM[vmID].serverID = id;

            // 申请虚拟机信息存储
            returnInfo(id);

            return true;
        }
    }
    // 单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else
    {
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        if (es.remainCpuA >= vm[0] && es.remainMemA >= vm[1])
        {
            es.remainCpuA -= vm[0];
            es.remainMemA -= vm[1];
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;

            existVM[vmID].name = vmName;
            existVM[vmID].isA = 0;
            existVM[vmID].serverID = id;

            returnInfo(id, 'A');

            return true;
        }
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
        ExistVM &ev = existVM[r.id];

        // 存在的服务器
        ExistServer &es = existServer[ev.serverID];
        // 存在的虚拟机格式
        vector<int> &vm = vmInfo[ev.name];
        if (vm[2])
        {
            //更新服务器信息
            es.remainCpuA += vm[0] / 2;
            es.remainCpuB += vm[0] / 2;
            es.remainMemA += vm[1] / 2;
            es.remainMemB += vm[1] / 2;
            es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
            es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;
        }
        else if (ev.isA)
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
        if (serverInfo[es.name][0] == es.remainCpuA + es.remainCpuB)
            dayCost -= serverInfo[es.name][3];
#endif // TEST

        //删除虚拟机信息
        existVM.erase(r.id);
        return true;
    }
    vector<int> &vm = vmInfo[r.name];
    //如果双端
    if (vm[2])
    {
        int c = vm[0] / 2, m = vm[1] / 2;
        for (int i = 0; i < serverCnt; ++i)
        {
            ExistServer &es = existServer[i];
            if (es.remainCpuA >= c && es.remainCpuB >= c && es.remainMemA >= m && es.remainMemB >= m)
            {
// 统计日均损耗
#ifdef TEST
                if (serverInfo[es.name][0] == es.remainCpuA + es.remainCpuB)
                    dayCost += serverInfo[es.name][3];
#endif // TEST

                //更新服务器信息
                es.remainCpuA -= c;
                es.remainCpuB -= c;
                es.remainMemA -= m;
                es.remainMemB -= m;
                es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;
                es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

                //创建虚拟机信息
                existVM[r.id].name = r.name;
                // existVM[r.id].isA = ;
                existVM[r.id].serverID = i;

                //输出信息
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
        int c = vm[0], m = vm[1];
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        for (int i = 0; i < serverCnt; ++i)
        {
            ExistServer &es = existServer[i];
            if (es.remainCpuA >= c && es.remainMemA >= m)
            {
// 统计日均损耗
#ifdef TEST
                if (serverInfo[es.name][0] == es.remainCpuA + es.remainCpuB)
                    dayCost += serverInfo[es.name][3];
#endif // TEST

                es.remainCpuA -= c;
                es.remainMemA -= m;
                es.CM_Ratio_A = es.remainCpuA * 1.0 / es.remainMemA;

                existVM[r.id].name = r.name;
                existVM[r.id].isA = 1;
                existVM[r.id].serverID = i;

                returnInfo(i, 'A');
                return true;
            }
            else if (es.remainCpuB >= c && es.remainMemB >= m)
            {
// 统计日均损耗
#ifdef TEST
                if (serverInfo[es.name][0] == es.remainCpuA + es.remainCpuB)
                    dayCost += serverInfo[es.name][3];
#endif // TEST

                es.remainCpuB -= c;
                es.remainMemB -= m;
                es.CM_Ratio_B = es.remainCpuB * 1.0 / es.remainMemB;

                existVM[r.id].name = r.name;
                existVM[r.id].isA = 0;
                existVM[r.id].serverID = i;

                returnInfo(i, 'B');
                return true;
            }
        }
    }
    return false;
}

// 每个请求队列分配服务器
void applyServer(vector<Request> &requests)
{
    int choseServer = 0;
    string curNeedServer;
    unordered_map<string, int> serverLastId;
    for (int i = 0; i < requests.size(); ++i)
    {
        // 如果存在的服务器分配未成功，则再分配
        if (!redistribution(requests[i]))
        {
            do
            {
                curNeedServer = readyServer[choseServer];
                int c = serverInfo[curNeedServer][0] / 2, m = serverInfo[curNeedServer][1] / 2;
                float cm = c * 1.0 / m;
                existServer[serverCnt].name = curNeedServer;
                existServer[serverCnt].remainCpuA = c;
                existServer[serverCnt].remainCpuB = c;
                existServer[serverCnt].remainMemA = m;
                existServer[serverCnt].remainMemB = m;
                existServer[serverCnt].CM_Ratio_A = cm;
                existServer[serverCnt].CM_Ratio_B = cm;
                choseServer++;
            } while (!addNewServer(requests[i].name, serverCnt, requests[i].id));

            serverapply[curNeedServer]++;
            serverCurCnt++;

#ifdef TEST
            dayCost += serverInfo[curNeedServer][3];
            priceSum += serverInfo[curNeedServer][2];
#endif // TEST

            // choseServer = 0;
            serverCnt++;
        }
    }
    requests.clear();
}

/*************************************************
Name: Pick server
Description:
**************************************************/

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
    // lower_bound(beg, end, val, comp)
    auto findIter =
        lower_bound(uniformedServers.begin(), uniformedServers.end(), CM_Ratio,
                    [](pair<string, ServerRatio> &server, float cm_R) { return server.second.ratio < cm_R; });
    int serverSize = uniformedServers.size();
    int sizeOfServers = serverSize > 70 ? (serverSize << 3) : serverSize < 10 ? serverSize : 10;
    decltype(findIter) startIter = uniformedServers.begin();
    decltype(findIter) endIter = uniformedServers.end();
    int searchScale = 3;
    if (findIter == uniformedServers.begin() && searchScale * sizeOfServers < serverSize)
    {
        endIter = uniformedServers.begin() + searchScale * sizeOfServers;
    }
    else if (findIter == uniformedServers.end() && searchScale * sizeOfServers < serverSize)
    {
        startIter = uniformedServers.end() - searchScale * sizeOfServers;
    }
    else
    {
        if (findIter - uniformedServers.begin() > sizeOfServers)
            startIter = findIter - sizeOfServers;
        if (uniformedServers.end() - findIter > sizeOfServers)
            endIter = findIter + sizeOfServers;
    }

    double mean = 0;
    for (auto it = startIter; it != endIter; ++it)
    {
        it->second.projection = (it->second.uMemory * 1 + it->second.uCpu * CM_Ratio) / (1 + sqrt(CM_Ratio * CM_Ratio));
        mean += it->second.projection;
    }
    mean /= (endIter - startIter);

    vector<string> bestServers;
    bestServers.reserve(sizeOfServers);
    float meanScale = 1.0;
    while (bestServers.size() < sizeOfServers)
    {
        meanScale /= 2;
        for (auto it = startIter; it != endIter; ++it)
        {
            if (serverInfo[it->first][0] < (maxCpu << 1) || serverInfo[it->first][1] < (maxMemory << 1))
                continue;
            if (it->second.projection > mean * (1 + meanScale) && bestServers.size() != sizeOfServers)
            {
                bestServers.push_back(it->first);
            }
        }
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
string name, vmID;
// 读入存储临时变量
int cpu, memory, price, loss, isdouble;
// 当前决策需要的CPU和Memory
int needC = 0, needM = 0, maxC = 0, maxM = 0;

// 读取请求
void readRequest()
{
    char c;
    // name代表型号
    name = "";
    // 这里vmID代表编号
    vmID = "";
    getchar();
    if ((isdouble = (c = getchar()) == 'a'))
    {
        while ((c = getchar()) != ',')
            ;
        getchar();
        while ((c = getchar()) != ',')
            name += c;
        getchar();
        while ((c = getchar()) != ')')
            vmID += c;
        needList[vmID] = {vmInfo[name][0], vmInfo[name][1]};
        needC += needList[vmID][0];
        needM += needList[vmID][1];
        maxC = max(maxC, needList[vmID][0]);
        maxM = max(maxM, needList[vmID][1]);
    }
    else
    {
        while ((c = getchar()) != ',')
            ;
        getchar();
        while ((c = getchar()) != ')')
            vmID += c;
        needC -= needList[vmID][0];
        needM -= needList[vmID][1];
        needList.erase(vmID);
    }
    getchar();
    requests.emplace_back(isdouble, vmID, name);
}

// 按行读取虚拟机信息
void readVMInfo()
{
    char c;
    name = "";
    getchar();
    while ((c = getchar()) != ',')
        name += c;
    getchar();
    cpu = readNum();
    getchar();
    memory = readNum();
    getchar();
    isdouble = getchar() == '1';
    getchar();
    getchar();
    vmInfo[name] = {cpu, memory, isdouble};
}

// 按行读取服务器信息
void readServerInfo()
{
    char c;
    name = "";
    getchar();
    while ((c = getchar()) != ',')
        name += c;
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
    serverInfo[name] = {cpu, memory, price, loss};
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
    rserverInfo(sss, serverapply.size());
    for (auto s : serverapply)
    {
        rserverInfo(s.first, s.second);
    }
    serverapply.clear();
    serverCurCnt = 0;
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
    for (int i = 0; i < VMapplyInfo.size(); ++i)
        cout << VMapplyInfo[i] << endl;
    VMapplyInfo.clear();
}

/*************************************************
Name: Main process
Description:
**************************************************/

void processIO()
{
#ifdef TEST
    string inputFile = "training-data/training-2.txt";
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
        //重置需求队列
        needC = 0;
        needM = 0;
        maxC = 0;
        maxM = 0;
        for (int j = 0; j < m; ++j)
        {
            readRequest();
        }
        float need_CM_Ratio = needC * 1.0 / needM;
        readyServer = bestServers(need_CM_Ratio, maxC, maxM);

        applyServer(requests);

        serverCensus();
        moveCensus();
        infoOut();

#ifdef TEST
        priceSum += dayCost;
#endif // TEST
    }
#ifdef TEST
    cout << priceSum << endl;
#endif // TEST
}

int main()
{
    processIO();
    return 0;
}

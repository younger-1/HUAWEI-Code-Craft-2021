#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

// #include <bits/stdc++.h>
using namespace std;

#define TEST

#ifdef TEST
int priceSum = 0, lossSum = 0;
#endif // TEST

int n, m;
// 读入数据临时变量
string name, ncpu, nG, nprice, nloss, nisdouble;
// 读入存储临时变量
int cpu, G, price, loss, isdouble;
// 当前决策需要的CPU和Memory
int needC = 0, needM = 0, maxC = 0, maxM = 0;
// 服务器总数
int serverCnt = 0;
string curNeedServer;

// 用户对虚拟机的请求
struct req
{
    bool isadd;
    string id;
    string name;
    // char id[28];
    // char name[32];
    req(bool isa, string i, string nam)
    {
        isadd = isa;
        id = i;
        name = nam;
    }
};

// 存在的服务器信息
struct eServer
{
    int resCpuA, resCpuB, resMermoryA, resMermoryB;
#ifdef TEST
    int perLoss;
#endif // TEST
    float CM_Ratio_A, CM_Ratio_B;
    string name;
    /*
    eServer(string &nam, int Cpu, int Mermory)
    {
        name = nam;
        resCpuA = resCpuB = Cpu/2;
        resMermoryA = resMermoryB = Mermory/2;
        CM_Ratio_A = resCpuA*1.0/resMermoryA;
        CM_Ratio_B = resCpuB*1.0/resMermoryB;
    }
    */
};

// 存在的虚拟机信息
struct eVM
{
    bool isA;
    int serverID;
    string name;
    /*
    eVM(string nam, int id, bool A)
    {
        name = nam;
        serverID = id;
        isA = A;
    }
    */
};

unordered_map<string, vector<int>> serverInfo;
unordered_map<string, vector<int>> vmInfo;
unordered_map<string, eVM> existVM;
unordered_map<int, eServer> existServer;
// 统计当前所需CPU，MEMORY所用map
unordered_map<string, vector<int>> needList;
// 当前队列待选服务器型号（必须包含容量大于任意请求的服务器大小）
vector<string> readyServer = {"NV603"};
vector<req> requests;
// 上两日操作消息
vector<string> infoLog;

int str2int(string &str)
{
    int ret = 0;
    for (int i = 0; i < str.size(); ++i)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            ret = ret * 10 + str[i] - '0';
        }
        else
            return ret;
    }
    return ret;
}

// 找到最优服务器列表
vector<string> bestServers(float CM_Radio, int maxCpu, int maxMemory);

// 指定服务器分配
bool Specify_Resdist(vector<int> &vm, int id)
{
    eServer &es = existServer[id];
    if (vm[2])
    {
        int c = vm[0] / 2, m = vm[1] / 2;
        if (es.resCpuA >= c && es.resCpuB >= c && es.resMermoryA >= m && es.resMermoryB >= m)
        {
            es.resCpuA -= c;
            es.resCpuB -= c;
            es.resMermoryA -= m;
            es.resMermoryB -= m;
            es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;
            es.CM_Ratio_B = es.resCpuB * 1.0 / es.resMermoryB;
            return true;
        }
    }
    //单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else
    {
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        if (es.resCpuA >= vm[0] && es.resMermoryA >= vm[1])
        {
            es.resCpuA -= vm[0];
            es.resMermoryA -= vm[1];
            es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;
            return true;
        }
    }
    return false;
}

// 在已有服务器上分配或删除
bool Redistribution(req &r)
{
    // 如果是删除操作
    if (!r.isadd)
    {
        // 存在的虚拟机信息
        eVM &ev = existVM[r.id];
        // 存在的服务器
        eServer &es = existServer[ev.serverID];
        // 存在的虚拟机格式
        vector<int> &vm = vmInfo[ev.name];
        if (vm[2])
        {
            //更新服务器信息
            es.resCpuA += vm[0] / 2;
            es.resCpuB += vm[0] / 2;
            es.resMermoryA += vm[1] / 2;
            es.resMermoryB += vm[1] / 2;
            es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;
            es.CM_Ratio_B = es.resCpuB * 1.0 / es.resMermoryB;
        }
        else if (ev.isA)
        {
            es.resCpuA += vm[0];
            es.resMermoryA += vm[1];
            es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;
        }
        else
        {
            es.resCpuB += vm[0];
            es.resMermoryB += vm[1];
            es.CM_Ratio_B = es.resCpuB * 1.0 / es.resMermoryB;
        }
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
            eServer &es = existServer[i];
            if (es.resCpuA >= c && es.resCpuB >= c && es.resMermoryA >= m && es.resMermoryB >= m)
            {
                //更新服务器信息
                es.resCpuA -= c;
                es.resCpuB -= c;
                es.resMermoryA -= m;
                es.resMermoryB -= m;
                es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;
                es.CM_Ratio_B = es.resCpuB * 1.0 / es.resMermoryB;

                //创建虚拟机信息
                existVM[r.id].name = r.name;
                // existVM[r.id].isA = ;
                existVM[r.id].serverID = i;
                return true;
            }
        }
    }
    //单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else
    {
        int c = vm[0], m = vm[1];
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        for (int i = 0; i < serverCnt; ++i)
        {
            eServer &es = existServer[i];
            if (es.resCpuA >= c && es.resMermoryA >= m)
            {
                es.resCpuA -= c;
                es.resMermoryA -= m;
                es.CM_Ratio_A = es.resCpuA * 1.0 / es.resMermoryA;

                existVM[r.id].name = r.name;
                existVM[r.id].isA = 1;
                existVM[r.id].serverID = i;
                return true;
            }
            else if (es.resCpuB >= c && es.resMermoryB >= m)
            {
                es.resCpuB -= c;
                es.resMermoryB -= m;
                es.CM_Ratio_B = es.resCpuB * 1.0 / es.resMermoryB;

                existVM[r.id].name = r.name;
                existVM[r.id].isA = 0;
                existVM[r.id].serverID = i;
                return true;
            }
        }
    }
    return false;
}

// 每个请求队列分配服务器
void applyServer(vector<req> &requests)
{
    int choseServer = 0;
    string curNeedServer;
    for (int i = 0; i < requests.size(); ++i)
    {
        // 如果存在的服务器分配未成功，则再分配
        if (!Redistribution(requests[i]))
        {
            do
            {
                curNeedServer = readyServer[choseServer];
                int c = serverInfo[curNeedServer][0] / 2, m = serverInfo[curNeedServer][1] / 2;
                float cm = c * 1.0 / m;
                existServer[serverCnt].name = curNeedServer;
                existServer[serverCnt].resCpuA = c;
                existServer[serverCnt].resCpuB = c;
                existServer[serverCnt].resMermoryA = m;
                existServer[serverCnt].resMermoryB = m;
                existServer[serverCnt].CM_Ratio_A = cm;
                existServer[serverCnt].CM_Ratio_B = cm;
                choseServer++;
            } while (!Specify_Resdist(vmInfo[requests[i].name], serverCnt));
#ifdef TEST
            priceSum += serverInfo[curNeedServer][2];
#endif // TEST
            choseServer = 0;
            serverCnt++;
        }
    }
}

void dataIO()
{
#ifdef TEST
    string inputFile = "test.txt";
    ifstream cin(inputFile);
#endif // TEST
    cin >> n;
    for (int i = 0; i < n; ++i)
    {
        cin >> name >> ncpu >> nG >> nprice >> nloss;
        name = name.substr(1, name.size() - 2);
        cpu = str2int(ncpu);
        G = str2int(nG);
        price = str2int(nprice);
        loss = str2int(nloss);
        serverInfo[name] = {cpu, G, price, loss};
    }

    cin >> n;
    for (int i = 0; i < n; ++i)
    {
        cin >> name >> ncpu >> nG >> nisdouble;
        name = name.substr(1, name.size() - 1);
        cpu = str2int(ncpu);
        G = str2int(nG);
        isdouble = nisdouble[0] - '0';
        vmInfo[name] = {cpu, G, isdouble};
    }

    cin >> n;
    for (int i = 0; i < n; ++i)
    {
        cin >> m;
        //重置当天需求
        needC = 0;
        needM = 0;
        maxC = 0;
        maxM = 0;
        for (int j = 0; j < m; ++j)
        {
            cin >> name;
            if (isdouble = name[1] == 'a')
            {
                cin >> ncpu >> nG;
                requests.emplace_back(isdouble, nG, ncpu);
                needList[nG] = {vmInfo[ncpu][0], vmInfo[ncpu][1]};
                needC += needList[nG][0];
                needM += needList[nG][1];
                maxC = max(maxC, needList[nG][0]);
                maxM = max(maxM, needList[nG][1]);
            }
            else
            {
                cin >> nG;
                requests.emplace_back(isdouble, nG, "");
                //有些虚拟机可能是当天创建当天删除

                needC -= needList[nG][0];
                needM -= needList[nG][1];
                needList.erase(nG);
            }
        }
        float need_CM_Radio = needC * 1.0 / needM;
        // readyServer = bestServers(need_CM_Radio, maxC, maxM);
        applyServer(requests);
    }
#ifdef TEST
    cout << priceSum << endl;
    cin.close();
#endif // TEST
}

int main()
{
    dataIO();
    return 0;
}

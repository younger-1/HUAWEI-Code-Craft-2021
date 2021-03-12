
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <queue>
#include <time.h>

//#include<bits/stdc++.h>

#define TEST

using namespace std;

#ifdef TEST
int priceSum = 0, lossSum = 0;
#endif // TEST

struct req
{
    bool isadd;
    string id;
    string name;
    //char id[28];
    //char name[32];
    req(bool isa, string i, string nam)
    {
        isadd = isa;
        id = i;
        name = nam;
    }
};
//存在的服务器信息
struct eServer
{
    int resCpuA, resCpuB, resMermoryA, resMermoryB;
    #ifdef TEST
    int perLoss;
    #endif // TEST
    float CM_Ratio_A, CM_Ratio_B;
    string name;
};
//存在的虚拟机信息
struct eVM
{
    bool isA;
    int serverID;
    string name;
};
unordered_map<string, vector<int>> serverInfo;
unordered_map<string, vector<int>> vmInfo;
unordered_map<string, eVM> existVM;
unordered_map<int, eServer> existServer;
//统计当前所需CPU，MEMORY所用map
unordered_map<string, vector<int>> needList;
// 当前队列待选服务器型号（必须包含容量大于任意请求的服务器大小）
vector<string> readyServer = {"NV603"};

// 当次队列购买的服务器
unordered_map<string, int> serverapply;
// 当次队列购买的服务器总数
int serverCurCnt = 0;
int n, m;
//读入数据临时变量
string name, vmID;
//读入存储临时变量
int cpu, memory, price, loss, isdouble;
// 当前决策需要的CPU和Memory
int needC = 0, needM = 0, maxC = 0, maxM = 0;
//服务器总数
int serverCnt = 0;
// 当前最匹配服务器
string curNeedServer;
// 当次队列申请的虚拟机与服务器与迁移输出信息
vector<string> VMapplyInfo, serverapplyInfo, moveInfo;

vector<req> requests;
//上两日操作消息
vector<string> infoLog;
const char digit[11] = "0123456789";
string int2str(int num)
{
    if (num==0)
        return "0";
    string ret = "";
    while(num)
    {
        ret += digit[num%10];
        num/=10;
    }
    reverse(ret.begin(), ret.end());
    return ret;
}
///////////////////////////////////////////////
// 读取函数
int getSingleNum()
{
    char c;
    int ret = 0;
    while((c = getchar())!='\n')
    {
        ret = (ret<<3) + (ret<<1) + c - '0';
    }
    return ret;
}
int getNum()
{
    char c;
    int ret = 0;
    while((c=getchar())!=',')
    {
        ret = (ret<<3) + (ret<<1) + c - '0';
    }
    return ret;
}
// 读取请求
void getRequest()
{
    char c;
    // name代表型号
    name = "";
    // 这里vmID代表编号
    vmID = "";
    getchar();
    if(isdouble = (c = getchar()) == 'a')
    {
        while((c = getchar())!=',');
        getchar();
        while((c = getchar())!=',')
            name += c;
        getchar();
        while((c = getchar())!=')')
            vmID += c;
        needList[vmID] = {vmInfo[name][0], vmInfo[name][1]};
        needC += needList[vmID][0];
        needM += needList[vmID][1];
        maxC = max(maxC, needList[vmID][0]);
        maxM = max(maxM, needList[vmID][1]);
    }
    else
    {
        while((c = getchar())!=',');
        getchar();
        while((c = getchar())!=')')
            vmID += c;
        needC -= needList[vmID][0];
        needM -= needList[vmID][1];
        needList.erase(vmID);
    }
    getchar();
    requests.emplace_back(isdouble, vmID, name);
}
// 按行读取虚拟机信息
void getVMInfo()
{
    char c;
    name = "";
    getchar();
    while((c = getchar())!=',')
        name += c;
    getchar();
    cpu = getNum();
    getchar();
    memory = getNum();
    getchar();
    isdouble = getchar() == '1';
    getchar();
    getchar();
    vmInfo[name] = {cpu, memory, isdouble};
}
// 按行读取服务器信息
void getServerInfo()
{
    char c;
    name = "";
    getchar();
    while((c = getchar())!=',')
        name += c;
    getchar();
    cpu = getNum();
    getchar();
    memory = getNum();
    getchar();
    price = getNum();
    getchar();
    loss = 0;
    while((c=getchar())!=')')
    {
        loss = loss<<3 + loss<<1 + c - '0';
    }
    getchar();
    serverInfo[name] = {cpu, memory, price, loss};
}

// 生成分配信息
void returnInfo(int id, char kind = 'D')
{
    string vmaly = "";
    if(kind == 'A')
    {
        vmaly += "(";
        vmaly += int2str(id);
        vmaly += ", ";
        vmaly += "A)";
        VMapplyInfo.emplace_back(vmaly);
    }
    else if(kind == 'B')
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
    rserverInfo(sss, serverCurCnt);
    for(auto &[k, v]:serverapply)
    {
        rserverInfo(k, v);
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
    for(int i=0;i<serverapplyInfo.size();++i)
        cout<<serverapplyInfo[i]<<endl;
    serverapplyInfo.clear();
    for(int i=0;i<moveInfo.size();++i)
        cout<<moveInfo[i]<<endl;
    moveInfo.clear();
    for(int i=0;i<VMapplyInfo.size();++i)
        cout<<VMapplyInfo[i]<<endl;
    VMapplyInfo.clear();
}
//找到最优服务器列表
vector<string> bestServers(float CM_Radio, int maxCpu, int maxMemory);
//指定服务器分配
bool Specify_Resdist(vector<int> &vm, int id)
{
    eServer &es = existServer[id];
    if(vm[2])
    {
        int c = vm[0]/2, m = vm[1]/2;
        if(es.resCpuA>=c&&es.resCpuB>=c&&es.resMermoryA>=m&&es.resMermoryB>=m)
        {
            es.resCpuA -= c;
            es.resCpuB -= c;
            es.resMermoryA -= m;
            es.resMermoryB -= m;
            es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;
            es.CM_Ratio_B = es.resCpuB*1.0/es.resMermoryB;

            // 申请虚拟机信息存储
            returnInfo(id);

            return true;
        }
    }
    //单端分配情况，优化点
    //????????????????????????
    //????????????????????????
    else
    {
        // 后续希望建立红黑树，按CM_Ratio参数选择合适的服务器分配
        if(es.resCpuA>=vm[0]&&es.resMermoryA>=vm[1])
        {
            es.resCpuA -= vm[0];
            es.resMermoryA -= vm[1];
            es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;

            returnInfo(id, 'A');

            return true;
        }
    }
    return false;
}
//在已有服务器上分配或删除
bool Redistribution(req &r)
{
    // 如果是删除操作
    if(!r.isadd)
    {
        // 存在的虚拟机信息
        eVM &ev = existVM[r.id];
        // 存在的服务器
        eServer &es = existServer[ev.serverID];
        // 存在的虚拟机格式
        vector<int> &vm = vmInfo[ev.name];
        if(vm[2])
        {
            //更新服务器信息
            es.resCpuA += vm[0]/2;
            es.resCpuB += vm[0]/2;
            es.resMermoryA += vm[1]/2;
            es.resMermoryB += vm[1]/2;
            es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;
            es.CM_Ratio_B = es.resCpuB*1.0/es.resMermoryB;

        }
        else if(ev.isA)
        {
            es.resCpuA += vm[0];
            es.resMermoryA += vm[1];
            es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;
        }
        else
        {
            es.resCpuB += vm[0];
            es.resMermoryB += vm[1];
            es.CM_Ratio_B = es.resCpuB*1.0/es.resMermoryB;
        }
        //删除虚拟机信息
        existVM.erase(r.id);
        return true;
    }
    vector<int> &vm = vmInfo[r.name];
    //如果双端
    if(vm[2])
    {
        int c = vm[0]/2, m = vm[1]/2;
        for(int i=0;i<serverCnt;++i)
        {
            eServer &es = existServer[i];
            if(es.resCpuA>=c&&es.resCpuB>=c&&es.resMermoryA>=m&&es.resMermoryB>=m)
            {
                //更新服务器信息
                es.resCpuA -= c;
                es.resCpuB -= c;
                es.resMermoryA -= m;
                es.resMermoryB -= m;
                es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;
                es.CM_Ratio_B = es.resCpuB*1.0/es.resMermoryB;

                //创建虚拟机信息
                existVM[r.id].name = r.name;
                //existVM[r.id].isA = ;
                existVM[r.id].serverID = i;

                //输出信息
                returnInfo(i);
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
        for(int i=0;i<serverCnt;++i)
        {
            eServer &es = existServer[i];
            if(es.resCpuA>=c&&es.resMermoryA>=m)
            {
                es.resCpuA -= c;
                es.resMermoryA -= m;
                es.CM_Ratio_A = es.resCpuA*1.0/es.resMermoryA;

                existVM[r.id].name = r.name;
                existVM[r.id].isA = 1;
                existVM[r.id].serverID = i;

                returnInfo(i, 'A');
                return true;
            }
            else if(es.resCpuB>=c&&es.resMermoryB>=m)
            {
                es.resCpuB -= c;
                es.resMermoryB -= m;
                es.CM_Ratio_B = es.resCpuB*1.0/es.resMermoryB;

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
void applyServer(vector<req> &requests)
{
    int choseServer = 0;
    string curNeedServer;
    for(int i=0;i<requests.size();++i)
    {
        // 如果存在的服务器分配未成功，则再分配
        if(!Redistribution(requests[i]))
        {
            do
            {
                curNeedServer = readyServer[choseServer];
                int c = serverInfo[curNeedServer][0]/2, m = serverInfo[curNeedServer][1]/2;
                float cm = c*1.0/m;
                existServer[serverCnt].name = curNeedServer;
                existServer[serverCnt].resCpuA = c;
                existServer[serverCnt].resCpuB = c;
                existServer[serverCnt].resMermoryA = m;
                existServer[serverCnt].resMermoryB = m;
                existServer[serverCnt].CM_Ratio_A = cm;
                existServer[serverCnt].CM_Ratio_B = cm;
                choseServer++;
            }while(!Specify_Resdist(vmInfo[requests[i].name], serverCnt));

            serverapply[curNeedServer]++;
            serverCurCnt++;

            #ifdef TEST
            priceSum += serverInfo[curNeedServer][2];
            #endif // TEST

            choseServer = 0;
            serverCnt++;
        }
    }
    requests.clear();
}
void dataIO()
{
    #ifdef TEST
    string inputFile = "test.txt";
    freopen(inputFile.c_str(), "rb", stdin);
    #endif // TEST

    n = getSingleNum();
    for(int i=0;i<n;++i)
    {
        getServerInfo();
    }

    n = getSingleNum();
    for(int i=0;i<n;++i)
    {
        getVMInfo();
    }

    n = getSingleNum();
    for(int i=0;i<n;++i)
    {
        m = getSingleNum();
        //重置需求队列
        needC = 0;
        needM = 0;
        maxC = 0;
        maxM = 0;
        for(int j=0;j<m;++j)
        {
            getRequest();
        }
        float need_CM_Radio = needC*1.0/needM;
        //readyServer = bestServers(need_CM_Radio, maxC, maxM);
        applyServer(requests);
        serverCensus();
        moveCensus();
        infoOut();
    }
    #ifdef TEST
    cout<<priceSum<<endl;
    #endif // TEST
}
int main()
{
    dataIO();
    return 0;
}

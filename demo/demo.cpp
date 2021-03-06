// https://paste.ubuntu.com/p/jx4PjcK7fN/

#include <assert.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// #define TEST

// 服务器信息
unordered_map<string, vector<int>> serverInfos;
// 虚拟机信息
unordered_map<string, vector<int>> vmInfos;
// 请求信息
vector<vector<string>> requestInfos;

// 购买的服务器信息
int serverNumber = 0;
unordered_map<int, vector<int>> sysServerResource;
// unordered_map<int,vector<string>> serverRunVms;

// 当前开机服务器
vector<int> serverRunVms;
// 记录虚拟机运行在那个服务器上
unordered_map<string, vector<int>> vmOnServer;

//#ifdef TEST
//    const string filePath = "../training-data/training-1.txt";
//#endif
const string filePath = "../training-data/training-2.txt";
// 成本
long long SERVERCOST = 0, POWERCOST = 0, TOTALCOST = 0;

// unordered_map<int,vector<string>> requestAddInfos;
// vector<int> requestDelInfos;

void generateServer(string &serverType, string &cpuCores, string &memorySize, string &serverCost, string &powerCost)
{
    string _serverType = "";
    for (int i = 1; i < serverType.size() - 1; i++)
    {
        _serverType += serverType[i];
    }
    int _cpuCores = 0, _memorySize = 0, _serverCost = 0, _powerCost = 0;

    for (int i = 0; i < cpuCores.size() - 1; i++)
    {
        _cpuCores = 10 * _cpuCores + cpuCores[i] - '0';
    }
    for (int i = 0; i < memorySize.size() - 1; i++)
    {
        _memorySize = 10 * _memorySize + memorySize[i] - '0';
    }
    for (int i = 0; i < serverCost.size() - 1; i++)
    {
        _serverCost = 10 * _serverCost + serverCost[i] - '0';
    }
    for (int i = 0; i < powerCost.size() - 1; i++)
    {
        _powerCost = 10 * _powerCost + powerCost[i] - '0';
    }
    serverInfos[_serverType] =
        vector<int>{_cpuCores / 2, _cpuCores / 2, _memorySize / 2, _memorySize / 2, _serverCost, _powerCost};
}

void generateVm(string &vmType, string &vmCpuCores, string &vmMemory, string &vmTwoNodes)
{
    string _vmType = "";

    for (int i = 1; i < vmType.size() - 1; i++)
    {
        _vmType += vmType[i];
    }

    int _vmCpuCores = 0, _vmMemory = 0, _vmTwoNodes = 0;
    for (int i = 0; i < vmCpuCores.size() - 1; i++)
    {
        _vmCpuCores = _vmCpuCores * 10 + vmCpuCores[i] - '0';
    }
    for (int i = 0; i < vmMemory.size() - 1; i++)
    {
        _vmMemory = _vmMemory * 10 + vmMemory[i] - '0';
    }
    if (vmTwoNodes[0] == '1')
    {
        _vmTwoNodes = 1;
    }
    else
    {
        _vmTwoNodes = 0;
    }
    vmInfos[_vmType] = vector<int>{_vmCpuCores, _vmMemory, _vmTwoNodes};
}
// 解析用户添加请求
void generateRequest(string &op, string &reqVmType, string &reqId)
{
    string _op, _reqVmType, _reqId;
    _op = op.substr(1, op.size() - 1);
    _reqVmType = reqVmType.substr(0, reqVmType.size() - 1);
    _reqId = reqId.substr(0, reqId.size() - 1);
    requestInfos.push_back(vector<string>{_op, _reqVmType, _reqId});
}

// 解析用户删除请求
void generateRequest(string &op, string &reqId)
{
    //    int _reqId = 0;
    //    for(int i=0;i<reqId.size() - 1;i++){
    //        _reqId =_reqId*10 + reqId[i] - '0';
    //    }
    string _op, _reqId;
    _reqId = reqId.substr(0, reqId.size() - 1);
    _op = op.substr(1, op.size() - 1);
    requestInfos.push_back(vector<string>{_op, _reqId});
    //    requestDelInfos.push_back(_reqId);

    //    cout<<_op<<" "<<_reqId<<endl;
    //    for(int i=0;i<reqVmType.size();i++){
    //        _reqVmType =reqVmType[i];
    //    }
}

// 尝试在服务器上分配虚拟机资源
bool choseServer(vector<int> &server, vector<int> &vm, int serverId, string vmId)
{
    int vmCores = vm[0], vmMemory = vm[1], vmTwoNodes = vm[2];
    int &serverCoreA = server[0], &serverCoreB = server[1], &serverMemoryA = server[2], &serverMemoryB = server[3];
    if (vmTwoNodes)
    {
        int needCores = vmCores / 2, needMemory = vmMemory / 2;
        if (serverCoreA >= needCores && serverCoreB >= needCores && serverMemoryA >= needMemory &&
            serverMemoryB >= needMemory)
        {
            serverCoreA -= needCores;
            serverCoreB -= needCores;
            serverMemoryA -= needMemory;
            serverMemoryB -= needMemory;
            vmOnServer[vmId] = vector<int>{serverId, 1, 2};
            cout << "(" << serverId << ")\n";
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (serverCoreA >= vmCores && serverMemoryA >= vmMemory)
    {
        serverCoreA -= vmCores;
        serverMemoryA -= vmMemory;
        vmOnServer[vmId] = vector<int>{serverId, 1};
        cout << "(" << serverId << ", A)\n";
        return true;
    }
    else if (serverCoreB >= vmCores && serverMemoryB >= vmMemory)
    {
        serverCoreB -= vmCores;
        serverMemoryB -= vmMemory;
        vmOnServer[vmId] = vector<int>{serverId, 2};
        cout << "(" << serverId << ", B)\n";
        return true;
    }

    return false;
}

// 处理创建虚拟机操作
int createVM(vector<string> &createVmInfo)
{
    string _reqVmType = createVmInfo[1], _reqId = createVmInfo[2];
    vector<int> vm = vmInfos[_reqVmType];
    int success = -1;
    for (int i = 0; i < serverNumber; i++)
    {
        auto &server = sysServerResource[i];
        if (choseServer(server, vm, i, _reqId))
        {
            //            serverRunVms[i].push_back(_reqId);
            serverRunVms[i]++;
            success = 1;
            break;
        }
        assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
    }
    return success;
}

// 处理删除虚拟机操作
void delVM(vector<string> &delVmInfo)
{
    string _vmId = delVmInfo[1];
    auto _vmInfo = vmOnServer[_vmId];
    vector<int> _serverInfo = vmOnServer[_vmId];
    int _serverId = _serverInfo[0];

    //    auto iter = std::find(std::begin(serverRunVms[_serverId]),
    //    std::end(serverRunVms[_serverId]), _vmId);
    //    serverRunVms[_serverId].erase(iter);
    serverRunVms[_serverId]--;

    vector<int> &server = sysServerResource[_serverId];
    if (_serverInfo.size() == 3)
    {
        int cores = _vmInfo[0] / 2, memory = _vmInfo[1] / 2;
        server[0] += cores;
        server[1] += cores;
        server[2] += memory;
        server[3] += memory;
    }
    else
    {
        int cores = _vmInfo[0], memory = _vmInfo[1];
        if (_serverInfo[1] == 1)
        {
            server[0] += cores;
            server[2] += memory;
        }
        else
        {
            server[1] += cores;
            server[3] += memory;
        }
    }
}

// 初始化server，如何初始化购买的服务器是一个大的优化
void bugServer()
{
    string serverType = "hostUY41I";
    int n = 10000;
    serverRunVms.resize(n, 0);
    cout << "(purchase, " << 2 << ")\n";
    string pauseInfo = "(" + serverType + ", ";
    pauseInfo += std::to_string(n / 2) + ")";
    cout << pauseInfo << endl;

    for (int i = 0; i < n / 2; i++)
    {
        sysServerResource[serverNumber++] = serverInfos[serverType];
        SERVERCOST += serverInfos[serverType][4];
    }
    serverType = "host78BMY";
    pauseInfo = "(" + serverType + ", ";
    pauseInfo += std::to_string(serverNumber) + ")";
    cout << pauseInfo << endl;
    for (int i = 0; i < n / 2; i++)
    {
        sysServerResource[serverNumber++] = serverInfos[serverType];
        SERVERCOST += serverInfos[serverType][4];
    }
}

// 扩容服务器策略
void expansion()
{
    cout << "(purchase, 0)\n";
}

// 迁移虚拟机策略
void migrate()
{
    cout << "(migration, 0)\n";
}

// 分配策略
void match(int day)
{
    if (day != 0)
        expansion();
    migrate();
    //    printf("There are %d requests waiting to matching
    //    !!\n",requestInfos.size());
    for (auto req : requestInfos)
    {
        // 创建虚拟机 还是 删除虚拟机
        int opType = req.size() == 3 ? 1 : 0;
        if (opType)
        {
            //#ifdef TEST
            //            cout<<"createing VMing !!"<<endl;
            //#endif
            if (createVM(req) == -1)
            {
#ifdef TEST
                cout << "No enough Resource" << endl;
#endif
            };
        }
        else
        {
            continue;
            // 释放虚拟机后，资源不是立即释放
            delVM(req);
        }
    }
}

void serverPower()
{
    for (int i = 0; i < serverNumber; i++)
    {
        //        if(serverRunVms[i].size() != 0){
        if (serverRunVms[i] != 0)
        {
            POWERCOST += sysServerResource[i][5];
        }
    }
}

int main()
{
    //    cin.tie(0);
    //    ios::sync_with_stdio(0);
    clock_t start, finish;
    start = clock();
    //#ifdef TEST
    //    std::freopen(filePath.c_str(),"rb",stdin);
    //#endif
    std::freopen(filePath.c_str(), "rb", stdin);
    int serverNum;
    string serverType, cpuCores, memorySize, serverCost, powerCost;
    scanf("%d", &serverNum);

    for (int i = 0; i < serverNum; i++)
    {
        cin >> serverType >> cpuCores >> memorySize >> serverCost >> powerCost;
        generateServer(serverType, cpuCores, memorySize, serverCost, powerCost);
    }

    int vmNumber = 0;
    scanf("%d", &vmNumber);

    string vmType, vmCpuCores, vmMemory, vmTwoNodes;
    for (int i = 0; i < vmNumber; i++)
    {
        cin >> vmType >> vmCpuCores >> vmMemory >> vmTwoNodes;
        generateVm(vmType, vmCpuCores, vmMemory, vmTwoNodes);
    }

    int requestdays = 0, dayRequestNumber = 0;
    scanf("%d", &requestdays);
    string op, reqVmType, reqId;

    // 开始处理请求
    bugServer();
    for (int day = 0; day < requestdays; day++)
    {
        scanf("%d", &dayRequestNumber);
        requestInfos.clear();
        for (int i = 0; i < dayRequestNumber; i++)
        {
            cin >> op;
            if (op[1] == 'a')
            {
                cin >> reqVmType >> reqId;
                generateRequest(op, reqVmType, reqId);
            }
            else
            {
                cin >> reqId;
                generateRequest(op, reqId);
            }
        }
#ifdef TEST
        if (day == 0 || (day + 1) % 100 == 0)
        {
            printf("The %d day begin matching!!!\n", day + 1);
        }
#endif
        match(day);
        serverPower();
        //        break;
    }

    fclose(stdin);
    finish = clock();
    TOTALCOST = SERVERCOST + POWERCOST;
#ifdef TEST
    printf("\nusr time: %f s \n", double(finish - start) / CLOCKS_PER_SEC);
    printf("server cost: %lld \npower cost: %lld \ntotal cost: %lld \n", SERVERCOST, POWERCOST, TOTALCOST);
#endif
    return 0;
}
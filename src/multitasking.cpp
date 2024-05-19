
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

common::uint32_t Task::pIdCounter=1;

void printf(char* str);
void printNum(int num);

Task::Task()
{
    taskState=FINISHED;
    pPid=0;
    waitPid=0;
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = 0;
    cpustate -> cs = 0;
    cpustate -> eflags = 0x202;
    
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    waitPid=0;
    pPid=0;
    pId=pIdCounter++;
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;
    
}

common::uint32_t Task::getId(){
    return pId;
}
Task::~Task()
{
}
        
TaskManager::TaskManager(GlobalDescriptorTable *gdtImport)
{
    gdt=gdtImport;
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{
}

common::uint32_t TaskManager::ForkTask(CPUState* cpustate)
{
    if(numTasks >= 256)
        return 0;
 
    tasks[numTasks].taskState=READY;
    tasks[numTasks].pPid=tasks[currentTask].pId;
    tasks[numTasks].pId=Task::pIdCounter++;
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i]=tasks[currentTask].stack[i];
    }
    //Stackten yer alında cpustate'in konumu değişiyor bu nedenle şuanki taskın offsetini hesaplayıp yeni oluşan process'in cpu statenin konumunu ona göre düzenliyorum. Bu işlemi yapmazsam process düzgün şekilde devam etmiyor.
    common::uint32_t currentTaskOffset=(((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate=(CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);
 
    //Burada ECX' yeni taskın process id'sini atıyorum. Syscall'a return edebilmek için.
    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;
    return tasks[numTasks-1].pId;
}

bool TaskManager::AddTask(Task *task)
{
    if(numTasks >= 256)
        return false;

    tasks[numTasks].taskState=READY;
    tasks[numTasks].pId=task->pId;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = task->cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate -> ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task->cpustate->edx;

    tasks[numTasks].cpustate -> esi = task->cpustate->esi;
    tasks[numTasks].cpustate -> edi = task->cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task->cpustate->ebp;
    
    tasks[numTasks].cpustate -> eip = task->cpustate->eip;
    tasks[numTasks].cpustate -> cs = task->cpustate->cs;
    
    tasks[numTasks].cpustate -> eflags = task->cpustate->eflags;
    numTasks++;
    return true;
}

common::uint32_t TaskManager::AddTask(void entrypoint())
{
    tasks[numTasks].taskState=READY;
    tasks[numTasks].pId=Task::pIdCounter++;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = 0;
    tasks[numTasks].cpustate -> ebx = 0;
    tasks[numTasks].cpustate -> ecx = 0;
    tasks[numTasks].cpustate -> edx = 0;

    tasks[numTasks].cpustate -> esi = 0;
    tasks[numTasks].cpustate -> edi = 0;
    tasks[numTasks].cpustate -> ebp = 0;
    
    tasks[numTasks].cpustate -> eip = (uint32_t)entrypoint;
    tasks[numTasks].cpustate -> cs = gdt->CodeSegmentSelector();
    
    tasks[numTasks].cpustate -> eflags = 0x202;
    numTasks++;
    return tasks[numTasks-1].pId;
}

common::uint32_t TaskManager::GetPId(){
    return tasks[currentTask].pId;
}

common::uint32_t TaskManager::ExecTask(void entrypoint())
{
    tasks[currentTask].taskState=READY;
    tasks[currentTask].cpustate = (CPUState*)(tasks[currentTask].stack + 4096 - sizeof(CPUState));
    
    tasks[currentTask].cpustate -> eax = 0;
    tasks[currentTask].cpustate -> ebx = 0;
    tasks[currentTask].cpustate -> ecx = tasks[currentTask].pId;
    tasks[currentTask].cpustate -> edx = 0;

    tasks[currentTask].cpustate -> esi = 0;
    tasks[currentTask].cpustate -> edi = 0;
    tasks[currentTask].cpustate -> ebp = 0;
    
    tasks[currentTask].cpustate -> eip = (uint32_t)entrypoint;
    tasks[currentTask].cpustate -> cs = gdt->CodeSegmentSelector();
    tasks[currentTask].cpustate -> eflags = 0x202;
    return (uint32_t)tasks[currentTask].cpustate;
}

bool TaskManager::ExitCurrentTask(){
    tasks[currentTask].taskState=FINISHED;
    //PrintProcessTable();
    return true;
}

int TaskManager::getIndex(common::uint32_t pid){
    int index=-1;
    for (int i = 0; i < numTasks; i++)
    {
        if(tasks[i].pId==pid){
            index=i;
            break;
        }
    }
    return index;
}

bool TaskManager::WaitTask(common::uint32_t esp){
    CPUState *cpustate=(CPUState*)esp;
    common::uint32_t pid=cpustate->ebx;

    if(tasks[currentTask].pId==pid || pid==0) // prevention self waiting
        return false;

    int index=getIndex(pid);
    if(index==-1)
        return false;

    if(numTasks<=index || tasks[index].taskState==FINISHED)
        return false;
        
    tasks[currentTask].cpustate=cpustate;
    tasks[currentTask].waitPid=pid;
    tasks[currentTask].taskState=WAITING;
    return true;
}

void TaskManager::PrintProcessTable(){
    printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("PID\tPPID\tSTATE\n");
    for (int i = 0; i < numTasks; i++)
    {
        printNum(tasks[i].pId);
        printf("\t  ");
        printNum(tasks[i].pPid);
        printf("\t   ");
        if(tasks[i].taskState==TaskState::READY){
            if(i==currentTask)
                printf("RUNNING");
            else
                printf("READY");
        }else if(tasks[i].taskState==TaskState::WAITING){
            printf("WAITING");
        }else if(tasks[i].taskState==TaskState::FINISHED){
            printf("FINISHED");
        }
        printf("\n");
    }
    printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    for (int i = 0; i < 10000000; i++)
    {
        printf("");
    }
    
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    for (int i = 0; i < 10000000; i++)
    {

    }
    if(numTasks <= 0)
    {
        return cpustate;
    }
        
    if(currentTask >= 0)
    {
        tasks[currentTask].cpustate = cpustate;
    }
        
    
    int findTask=(currentTask+1)%numTasks;
    while (tasks[findTask].taskState!=READY)
    {
        if(tasks[findTask].taskState==WAITING && tasks[findTask].waitPid>0){

            int waitTaskIndex=0;
            waitTaskIndex=getIndex(tasks[findTask].waitPid);
            if(waitTaskIndex>-1 && tasks[waitTaskIndex].taskState!=WAITING){
                if (tasks[waitTaskIndex].taskState==FINISHED)
                {
                    tasks[findTask].waitPid=0;
                    tasks[findTask].taskState=READY;
                    continue;
                }else if (tasks[waitTaskIndex].taskState==READY)
                {
                    findTask=waitTaskIndex;
                    continue;
                }
                
            }
            
        }
        findTask= (++findTask) % numTasks;
    }

    currentTask=findTask;
    PrintProcessTable();
    return tasks[currentTask].cpustate;
}


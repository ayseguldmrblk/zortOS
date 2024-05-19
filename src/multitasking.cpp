
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
    // Check if we have reached the maximum number of tasks
    if (numTasks >= 256)
    {
        return 0;
    }

    // Set the new task's state and parent PID
    tasks[numTasks].taskState = READY;
    tasks[numTasks].pPid = tasks[currentTask].pId;
    tasks[numTasks].pId = Task::pIdCounter++;

    // Copy the stack of the current task to the new task
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }

    // Calculate the offset of the CPU state within the stack
    common::uint32_t currentTaskOffset = (common::uint32_t)cpustate - (common::uint32_t)tasks[currentTask].stack;

    // Set the new task's CPU state pointer to the correct position within its stack
    tasks[numTasks].cpustate = (CPUState*)((common::uint32_t)tasks[numTasks].stack + currentTaskOffset);

    // Set the new task's ECX register to 0, as required by the fork convention
    tasks[numTasks].cpustate->ecx = 0;

    // Increment the number of tasks
    numTasks++;

    // Return the new task's PID to the parent process
    return tasks[numTasks - 1].pId;
}

bool TaskManager::AddTask(Task* task)
{
    // Check if we have reached the maximum number of tasks
    if (numTasks >= 256)
    {
        return false;
    }

    // Set the new task's state to READY
    tasks[numTasks].taskState = READY;

    // Assign the provided PID to the new task
    tasks[numTasks].pId = task->pId;

    // Initialize the new task's CPU state pointer within its stack
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));

    // Copy the CPU state registers from the provided task to the new task
    tasks[numTasks].cpustate->eax = task->cpustate->eax;
    tasks[numTasks].cpustate->ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate->ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate->edx = task->cpustate->edx;
    tasks[numTasks].cpustate->esi = task->cpustate->esi;
    tasks[numTasks].cpustate->edi = task->cpustate->edi;
    tasks[numTasks].cpustate->ebp = task->cpustate->ebp;
    tasks[numTasks].cpustate->eip = task->cpustate->eip;
    tasks[numTasks].cpustate->cs = task->cpustate->cs;
    tasks[numTasks].cpustate->eflags = task->cpustate->eflags;

    // Increment the number of tasks
    numTasks++;

    return true;
}

common::uint32_t TaskManager::AddTask(void entrypoint())
{
    // Set the new task's state to READY
    tasks[numTasks].taskState = READY;

    // Assign a new PID to the new task
    tasks[numTasks].pId = Task::pIdCounter++;

    // Initialize the new task's CPU state pointer within its stack
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));

    // Initialize the CPU state registers
    tasks[numTasks].cpustate->eax = 0;
    tasks[numTasks].cpustate->ebx = 0;
    tasks[numTasks].cpustate->ecx = 0;
    tasks[numTasks].cpustate->edx = 0;
    tasks[numTasks].cpustate->esi = 0;
    tasks[numTasks].cpustate->edi = 0;
    tasks[numTasks].cpustate->ebp = 0;

    // Set the instruction pointer to the entry point
    tasks[numTasks].cpustate->eip = (uint32_t)entrypoint;
    tasks[numTasks].cpustate->cs = gdt->CodeSegmentSelector();
    tasks[numTasks].cpustate->eflags = 0x202;

    // Increment the number of tasks
    numTasks++;

    // Return the new task's PID
    return tasks[numTasks - 1].pId;
}

common::uint32_t TaskManager::GetPId()
{
    return tasks[currentTask].pId;
}

common::uint32_t TaskManager::ExecTask(void entrypoint())
{
    // Set the current task's state to READY
    tasks[currentTask].taskState = READY;

    // Initialize the current task's CPU state pointer within its stack
    tasks[currentTask].cpustate = (CPUState*)(tasks[currentTask].stack + 4096 - sizeof(CPUState));

    // Initialize the CPU state registers
    tasks[currentTask].cpustate->eax = 0;
    tasks[currentTask].cpustate->ebx = 0;
    tasks[currentTask].cpustate->ecx = tasks[currentTask].pId;
    tasks[currentTask].cpustate->edx = 0;
    tasks[currentTask].cpustate->esi = 0;
    tasks[currentTask].cpustate->edi = 0;
    tasks[currentTask].cpustate->ebp = 0;

    // Set the instruction pointer to the entry point
    tasks[currentTask].cpustate->eip = (uint32_t)entrypoint;
    tasks[currentTask].cpustate->cs = gdt->CodeSegmentSelector();
    tasks[currentTask].cpustate->eflags = 0x202;

    // Return the CPU state pointer
    return (uint32_t)tasks[currentTask].cpustate;
}

bool TaskManager::ExitCurrentTask()
{
    // Mark the current task as finished
    tasks[currentTask].taskState = FINISHED;

    // Optionally, print the process table
    // PrintProcessTable();

    return true;
}

int TaskManager::getIndex(common::uint32_t pid)
{
    for (int i = 0; i < numTasks; i++)
    {
        if (tasks[i].pId == pid)
        {
            return i;
        }
    }
    return -1; // PID not found
}

bool TaskManager::WaitTask(common::uint32_t esp)
{
    CPUState* cpustate = (CPUState*)esp;
    common::uint32_t pid = cpustate->ebx;

    // Prevent self-waiting
    if (tasks[currentTask].pId == pid || pid == 0)
        return false;

    int index = getIndex(pid);
    if (index == -1)
        return false;

    if (tasks[index].taskState == FINISHED)
        return false;

    // Set the current task to wait for the specified PID
    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].waitPid = pid;
    tasks[currentTask].taskState = WAITING;

    return true;
}

void TaskManager::PrintProcessTable()
{
    printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("PID\tPPID\tSTATE\n");
    for (int i = 0; i < numTasks; i++)
    {
        printNum(tasks[i].pId);
        printf("\t  ");
        printNum(tasks[i].pPid);
        printf("\t   ");
        if (tasks[i].taskState == TaskState::READY)
        {
            if (i == currentTask)
                printf("RUNNING");
            else
                printf("READY");
        }
        else if (tasks[i].taskState == TaskState::WAITING)
        {
            printf("WAITING");
        }
        else if (tasks[i].taskState == TaskState::FINISHED)
        {
            printf("FINISHED");
        }
        printf("\n");
    }
    printf("\n^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    // Simple delay loop for demonstration purposes
    for (int i = 0; i < 10000000; i++) {}

    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    // Find the next READY task
    int findTask = (currentTask + 1) % numTasks;
    while (tasks[findTask].taskState != READY)
    {
        if (tasks[findTask].taskState == WAITING && tasks[findTask].waitPid > 0)
        {
            int waitTaskIndex = getIndex(tasks[findTask].waitPid);
            if (waitTaskIndex > -1 && tasks[waitTaskIndex].taskState != WAITING)
            {
                if (tasks[waitTaskIndex].taskState == FINISHED)
                {
                    tasks[findTask].waitPid = 0;
                    tasks[findTask].taskState = READY;
                }
                else if (tasks[waitTaskIndex].taskState == READY)
                {
                    findTask = waitTaskIndex;
                    continue;
                }
            }
        }
        findTask = (findTask + 1) % numTasks;
    }

    currentTask = findTask;
    PrintProcessTable();
    return tasks[currentTask].cpustate;
}



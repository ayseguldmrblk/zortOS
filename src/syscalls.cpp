
#include <syscalls.h>

using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

// Function declarations
void printf(char*);
void printNum(int num);

// Enum for system call identifiers
enum SYSCALLS { EXIT, GETPID, WAITPID, FORK, EXEC, PRINTF, ADDTASK };

// Function to get the process ID
int myos::getPid()
{
    int pId = -1;
    asm("int $0x80" : "=c" (pId) : "a" (SYSCALLS::GETPID));
    return pId;
}

// Function to wait for a specific process to finish
void myos::waitpid(common::uint8_t wPid)
{
    asm("int $0x80" : : "a" (SYSCALLS::WAITPID), "b" (wPid));
}

// Function to exit the current process
void myos::sys_exit()
{
    asm("int $0x80" : : "a" (SYSCALLS::EXIT));
}

// Function to print a string using a system call
void myos::sysprintf(char* str)
{
    asm("int $0x80" : : "a" (SYSCALLS::PRINTF), "b" (str));
}

// Function to create a new process (fork)
void myos::fork()
{
    asm("int $0x80" :: "a" (SYSCALLS::FORK));
}

// Function to create a new process and get the child PID
int myos::fork_with_pid(int pid)
{
    asm("int $0x80" : "=c" (pid) : "a" (SYSCALLS::FORK));
    return pid;
}

// Function to execute a new program in the current process
int myos::exec(void entrypoint())
{
    int result;
    asm("int $0x80" : "=c" (result) : "a" (SYSCALLS::EXEC), "b" ((uint32_t)entrypoint));
    return result;
}

// Function to add a new task to the task manager
int myos::addTask(void entrypoint())
{
    int result;
    asm("int $0x80" : "=c" (result) : "a" (SYSCALLS::ADDTASK), "b" ((uint32_t)entrypoint));
    return result;
}

// SyscallHandler constructor to initialize the interrupt handler for system calls
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset())
{
}

// SyscallHandler destructor
SyscallHandler::~SyscallHandler()
{
}

// Function to handle system call interrupts
uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    switch(cpu->eax)
    {
        case SYSCALLS::EXEC:
            esp = InterruptHandler::sys_exec(cpu->ebx);
            break;
        case SYSCALLS::FORK:
            cpu->ecx = InterruptHandler::sys_fork(cpu);
            return InterruptHandler::HandleInterrupt(esp);
            break;
        case SYSCALLS::PRINTF:
            printf((char*)cpu->ebx);
            break;
        case SYSCALLS::EXIT:
            if (InterruptHandler::sys_exit())
            {
                return InterruptHandler::HandleInterrupt(esp);
            }
            break;
        case SYSCALLS::WAITPID:
            if (InterruptHandler::sys_waitpid(esp))
            {
                return InterruptHandler::HandleInterrupt(esp);
            }
            break;
        case SYSCALLS::GETPID:
            cpu->ecx = InterruptHandler::sys_getpid();
            break;
        case SYSCALLS::ADDTASK:
            cpu->ecx = InterruptHandler::sys_addTask(cpu->ebx);
            break;
        default:
            break;
    }
    
    return esp;
}

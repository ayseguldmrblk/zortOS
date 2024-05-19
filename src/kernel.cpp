
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <syscalls.h>

// #define GRAPHICSMODE
using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;


char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            case '\t':
                x = x+5;
                if(x>=80){
                    x=0;
                }
            break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printNum(int num){
    char numberStr[10];
    itoa(num,numberStr,10);
    printf(numberStr);
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}

void printArray(int arr[],int n){
    printf("Input: {");
    for(int i=0;i<n;i++){
        printNum(arr[i]);
        if(i+1!=n){
            printf(",");
        }
    }
    printf("} ");
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};

unsigned int xorshift32(unsigned int *state) {
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

common::uint32_t long_running_program(int n)
{
    int result = 0;
    for(int i = 0; i < n; ++i)
    {   
        for(int j = 0; j < n; ++j)
        {
            result += i*j;
        }
    }
    return result;
}

void collatz(int n) 
{   printf("Collatz with parameter: ");
    printNum(n);
    printf("\n");
    printf("Result");
    printf(": ");
    while (n != 1) 
    {
        if (n % 2 == 0) 
        {
            n /= 2;
        }
        else 
        {
            n = 3 * n + 1;
        }
        printNum(n);
        printf(" ");
    }
    printf("\n");
}

void forkTestExample()
{
    //printf("{{{{{{{{{ Test Fork }}}}}}}}}");
    //printf("\n");
    int parentPID=getPid();
    printf("Starting with parent pid: ");
    printNum(parentPID);
    printf("\n");

    int childPID = fork_with_pid(getPid());
    if(childPID==0)
    {
        printf("Forked child pid ");
        printNum(getPid());
        printf("\n");
        sys_exit();
    }
    else
    {
        printf("Parent pid which should be the same as before: ");
        printNum(getPid());
        printf("\n");
    }
    sys_exit();
}

void strategyOne()
{
    int childPID[5];
    int childCounter = 1;
    printf("{{{{{{{{{ Test Strategy 1 }}}}}}}}}");
    printf("\n");
    int initialPid = getPid();
    printf("Initial Process PID: ");
    printNum(initialPid);
    printf("\n");
    int collatzTest, longTest;
    int numForks = 3; // Number of times to fork to create 6 programs
    for (int i = 0; i < numForks; ++i)
    {
        int childPid;
        int result = fork_with_pid(childPid);

        if (result == 0) 
        {
            // This is the child process
            printNum(childCounter);  
            printf(". Child Process with");
            printf(" PID: ");
            printNum(getPid());
            printf(" runs collatz");
            printf("\n");
            collatzTest = getPid()+5;
            collatz(collatzTest);

            sys_exit();
        } 
        else 
        {
            childPID[i] = childCounter++;
        }
    }


    for(int i = 0; i < numForks; ++i)
    {
        int childPid;
        int result = fork_with_pid(childPid);

        if (result == 0) 
        {
            // This is the child process
            printNum(childCounter);  
            printf(". Child Process with");
            printf(" PID: ");
            printNum(getPid());
            printf(" runs long running program with ");
            longTest = 1000 + i;
            printNum(longTest);
            printf("\n");
            printf("Result:");
            printNum(long_running_program(longTest));
            printf("\n");

            sys_exit();
        } 
        else 
        {
            childPID[i] = childCounter++;
        }
    }

    for(int i = 0; i < 6; ++i)
    {
        waitpid(childPID[i]);
    }

    printf("All child processes are ended.");
}


void executeThisFunction()
{
    printf("Exec System Call Runned Properly!");
    printf("\n");
    sys_exit();
}

void execTestExamle()
{
    //printf("{{{{{{{{{ Test Exec }}}}}}}}}");
    //printf("\n");
    printf("Exec syscall is testing with PID: "); 
    printNum(getPid()); 
    printf("\n");
    int exec1 = exec(executeThisFunction);
    sys_exit();
}


typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("-----------------Welcome to the ZortOS--------------\n");

    GlobalDescriptorTable gdt;
    TaskManager taskManager(&gdt);

    //Task taskTestExec(&gdt,execTestExamle);
    //taskManager.AddTask(&taskTestExec);

    //Task taskTestFork(&gdt,forkTestExample);
    //taskManager.AddTask(&taskTestFork);

    Task taskStrategyOne(&gdt,strategyOne);
    taskManager.AddTask(&taskStrategyOne);
    
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);


    // printf("Initializing Hardware, Stage 1\n");
    
    // #ifdef GRAPHICSMODE
    //     Desktop desktop(320,200, 0x00,0x00,0xA8);
    // #endif
    
    // DriverManager drvManager;
    
    //     #ifdef GRAPHICSMODE
    //         KeyboardDriver keyboard(&interrupts, &desktop);
    //     #else
    //         PrintfKeyboardEventHandler kbhandler;
    //         KeyboardDriver keyboard(&interrupts, &kbhandler);
    //     #endif
    //     drvManager.AddDriver(&keyboard);
        
    
    //     #ifdef GRAPHICSMODE
    //         MouseDriver mouse(&interrupts, &desktop);
    //     #else
    //         MouseToConsole mousehandler;
    //         MouseDriver mouse(&interrupts, &mousehandler);
    //     #endif
    //     drvManager.AddDriver(&mouse);
        
    //     PeripheralComponentInterconnectController PCIController;
    //     PCIController.SelectDrivers(&drvManager, &interrupts);

    //     VideoGraphicsArray vga;
        
    // printf("Initializing Hardware, Stage 2\n");
    //     // drvManager.ActivateAll();
        
    // printf("Initializing Hardware, Stage 3\n");

    // #ifdef GRAPHICSMODE
    //     vga.SetMode(320,200,8);
    //     Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
    //     desktop.AddChild(&win1);
    //     Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
    //     desktop.AddChild(&win2);
    // #endif


    interrupts.Activate();
    
    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
    }
}

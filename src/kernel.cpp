
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

int32_t generateRandomNumber(int min, int max)
{
    uint64_t counter;
    int32_t num;
    /* Read the clock counter */
    asm("rdtsc": "=A"(counter));

    /* Use the clock counter as a source of randomness */
    counter = counter * 1103515245 + 12345;
    num = (int)(counter / 65536) % (max - min);
    if (num<0)
        num+=max;
    return num+min;
}

void collatz(int n) {
    printNum(n);
    printf(" : ");
    while (n != 1) {
        if (n % 2 == 0) {
            n /= 2;
        }
        else {
            n = 3 * n + 1;
        }
        printNum(n);
        printf(" ");
    }

    printf("\n");
}

void TaskCollatz(){
    int n = 7;
    collatz(n);
    sys_exit();
}

int linear_search(int arr[], int n, int key) {
    for (int i = 0; i < n; i++) {
        if (arr[i] == key) {
            return i;
        }
    }
    return -1;
}

void TaskLinearSearch(){
    int arr[] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170};
    int n = sizeof(arr) / sizeof(arr[0]);
    int key = 175;
    int result = linear_search(arr, n, key);
    printArray(arr,n);
    printf("x = ");
    printNum(key);
    printf("; Output: ");
    printNum(result);
    printf("\n");
    sys_exit();
}

int binary_search(int arr[], int left, int right, int key) {
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid] == key) {
            return mid;
        }
        else if (arr[mid] < key) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }

    return -1;
}

void TaskBinarySearch(){
    int arr[] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170};
    int n = sizeof(arr) / sizeof(arr[0]);
    int key = 110;
    int result = binary_search(arr, 0, n - 1, key);
    
    printArray(arr,n);
    printf("x = ");
    printNum(key);
    printf("; Output: ");
    printNum(result);
    printf("\n");

    sys_exit();
}

void initKernelA()
{
    uint32_t pid1=0;
    uint32_t pid2=0;
    uint32_t pid3=0;
    pid1=addTask(TaskBinarySearch);
    pid2=addTask(TaskLinearSearch);
    pid3=addTask(TaskCollatz);
    waitpid(pid3);
    sys_exit();
}

void initKernelB(){
    int randomNumber = generateRandomNumber(1,3);
    uint32_t pid=0;
    for(int i=0;i<10;i++){
        if(randomNumber==1){
            pid=addTask(TaskBinarySearch);
        }else if (randomNumber==2)
        {
            pid=addTask(TaskLinearSearch);
        }else{
            pid=addTask(TaskCollatz);
        }
        
    }

    waitpid(pid);
    sys_exit();
}

void initKernelC(){
    int randomNumber = generateRandomNumber(1,3);
    int randomNumber2=-1;
    while((randomNumber2=generateRandomNumber(1,3)) == randomNumber);
    uint32_t pid=0;

    uint32_t pid2=0;

    for(int i=0;i<3;i++){
        switch (randomNumber)
        {
            case 1:
                pid2=addTask(TaskBinarySearch);
            break;
            case 2:
                pid2=addTask(TaskLinearSearch);
            break;
            case 3:
                pid2=addTask(TaskCollatz);
            break;
            default: break;
        }
        
    }
    for(int i=0;i<3;i++){
        switch (randomNumber2)
        {
            case 1:
                pid2=addTask(TaskBinarySearch);
            break;
            case 2:
                pid2=addTask(TaskLinearSearch);
            break;
            case 3:
                pid2=addTask(TaskCollatz);
            break;
            default: break;
        }
    }

    waitpid(pid);
    waitpid(pid2);
    sys_exit();
}

void forkTestExample()
{


    int parentPID=getPid();
    printf("Parent Pid:");
    printNum(parentPID);
    printf("\n");

    int childPID = fork_with_pid(getPid());
    if(childPID==0)
    {
        printf("Child Task ");
        printNum(parentPID);
        printf("\n");
    }
    else
    {
        printf("Parent Task ");
        printNum(getPid());
        printf("\n");
    }
    sys_exit();
}

void execTestExamle1(){
    printf("Buradan devam edecek\n");
    printf("execTestExamle1 "); printNum(getPid()); printf(" finished.\n");
    sys_exit();
}

void execTestExamle()
{
    printf("execTestExamle "); printNum(getPid()); printf(" start\n");
    int exec1=exec(execTestExamle1);
    printf("execTestExamle "); printNum(getPid()); printf(" finished.\n");
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
    printf("-----------------Welcome the ZortOS--------------\n");

    GlobalDescriptorTable gdt;
    TaskManager taskManager(&gdt);

    //Task task1(&gdt,initKernelA);
    //taskManager.AddTask(&task1);

    // Task task2(&gdt,initKernelB);
    // taskManager.AddTask(&task2);

    // Task task3(&gdt,initKernelC);
    // taskManager.AddTask(&task3);
    

    //Extra Tasks

    // Task task4(&gdt,execTestExamle);
    // taskManager.AddTask(&task4);

    Task task5(&gdt,forkTestExample);
    taskManager.AddTask(&task5);
    
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

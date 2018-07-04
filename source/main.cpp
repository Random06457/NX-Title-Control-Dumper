#include <stdio.h>
#include <fstream>
#include <string.h>
#include <dirent.h>
#include <switch.h>

const char* SD_PATH = "controls/";


void printInfo()
{
    printf("Title Control Dumper\n\n");
    
    printf("A : Dump full control data\n");
    printf("X : Dump icons only\n");
    printf("Y : List applications\n");
    printf("+ : Exit\n");
}

void WaitBackMenu()
{
    printf("\nPress B to return to menu.");
    while(appletMainLoop())
    {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_B) break;
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }
    consoleClear();
    printInfo();
}

void dumpAppControl(u64 titleId, bool full)
{
    NsApplicationControlData *buff = new NsApplicationControlData;
    
    size_t outsize = 0;
    Result rc;

    rc = nsGetApplicationControlData(1, titleId, buff, sizeof(NsApplicationControlData), &outsize);
    if (R_FAILED(rc))
    {
        printf("nsGetApplicationControlData() failed: 0x%x\n", rc);
        return;
    }

    if (outsize < sizeof(buff->nacp))
    {
        printf("Outsize is too small: 0x%lx.\n", outsize);
        return;
    }
    
    size_t nacpSize = sizeof(buff->nacp);
    size_t iconSize = sizeof(NsApplicationControlData) - nacpSize;
    
    printf("icon size: 0x%lx\n", iconSize);
    printf("nacp size: 0x%lx\n", nacpSize);
    
    char* path = new char[255];
    
    mkdir(SD_PATH, 0700);
    
    if (full)
    {
        snprintf(path, 255, "%sfull", SD_PATH);
        mkdir(path, 0700);
        
        snprintf(path, 255, "%sfull/%016lx", SD_PATH, titleId);
        mkdir(path, 0700);
        
        //nacp
        snprintf(path, 255, "%sfull/%016lx/control.nacp", SD_PATH, titleId);
        printf("writing %s...\n", path);
        std::ofstream outnacp (path, std::ofstream::binary);
        outnacp.write((char*)buff, nacpSize);
        outnacp.close();
        
        //icon
        snprintf(path, 255, "%sfull/%016lx/icon.jpg", SD_PATH, titleId);
        printf("writing %s...\n", path);
        std::ofstream outicon (path, std::ofstream::binary);
        outicon.write(((char*)buff) + nacpSize, iconSize);
        outicon.close();
        
        printf("%016lx Done !\n\n", titleId);
    }
    else 
    {
        snprintf(path,255, "%sicons", SD_PATH);
        mkdir(path, 0700);
        
        snprintf(path, 255, "%sicons/%016lx.jpg", SD_PATH, titleId);
        printf("writing %s...\n", path);
        
        std::ofstream outfile (path,std::ofstream::binary);
        outfile.write(((char*)buff)+nacpSize, iconSize);
        outfile.close();
        printf("%016lx Done !\n\n", titleId);
        
    }

    delete buff;
    buff = NULL;
}


void dumpControls(bool full)
{
    consoleClear();
    
    NsApplicationRecordList* buff;
    Result rc;
    
    buff = new NsApplicationRecordList;
    rc=nsListApplicationRecord(buff, sizeof(NsApplicationRecordList));
    
    if (R_FAILED(rc))
    {
        printf("ListApplicationRecord() failed.\n");
        return;
    }
    
    int i = 0;
    while(buff->records[i].title_id != 0)
    {
        printf("Dumping %016lx Control data\n", buff->records[i].title_id);
        dumpAppControl(buff->records[i].title_id, full);
        i++;
    }
    

    delete buff;
    buff = NULL;
    
    WaitBackMenu();
}


void ListApplications()
{
    consoleClear();
    
    NsApplicationRecordList *buff;
    buff = new NsApplicationRecordList;
    
    char path[255];
    Result rc;
    
    rc = nsListApplicationRecord(buff, sizeof(NsApplicationRecordList));
    
    if (R_FAILED(rc))
    {
        printf("ListApplicationRecord() failed: 0x%x\n", rc);
        return;
    }
    
    printf("Titles:\n\n");
    
    int i = 0;
    while(buff->records[i].title_id != 0)
    {
        printf("%016lx\n", buff->records[i].title_id);
        i++;
    }
    
    printf("\n");
    
    mkdir(SD_PATH, 0700);
    
    snprintf(path, 255, "%slist.bin", SD_PATH);
    printf("Writing %s...\n", path);
    
    std::ofstream outfile (path, std::ofstream::binary);
    outfile.write((const char*)buff, sizeof(NsApplicationRecordList));
    outfile.close();
    
    delete buff;
    
    WaitBackMenu();
}


int main(int argc, char **argv)
{
    gfxInitDefault();
    consoleInit(NULL);
    
    Result rc = nsInitialize();
    if (R_FAILED(rc))
        printf("nsInitialize() failed.\n");
    
    printInfo();
    
    while(appletMainLoop())
    {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break;
        if (kDown & KEY_A) dumpControls(true);
        if (kDown & KEY_X) dumpControls(false);
        if (kDown & KEY_Y) ListApplications();

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }
    gfxExit();
    nsExit();
    return 0;
}
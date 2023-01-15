/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

// MP4
int SysCreate(char *filename, int size)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->fileSystem->Create(filename, size);
}
// MP4
OpenFileId SysOpen(char *name)
{
    return kernel->fileSystem->OpenAFile(name);
}
// MP4
int SysWrite(char *buffer, int size, OpenFileId id)
{
    return kernel->fileSystem->Write(buffer, size, id);
}
// MP4
int SysRead(char *buffer, int size, OpenFileId id)
{
    return kernel->fileSystem->Read(buffer, size, id);
}
// MP4
int SysClose(OpenFileId id)
{
    return kernel->fileSystem->Close(id);
}


#ifdef FILESYS_STUB
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}
#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */

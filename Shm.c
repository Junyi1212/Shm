/*
linux中的两种共享内存。
一种是我们的IPC通信System V版本的共享内存，
另外的一种就是我们今天提到的存储映射I/O（mmap函数）

总结mmap和shm:
1、mmap是在磁盘上建立一个文件，每个进程地址空间中开辟出一块空间进行映射。
而对于shm而言，shm每个进程最终会映射到同一块物理内存。
shm保存在物理内存，这样读写的速度要比磁盘要快，但是存储量不是特别大。
2、相对于shm来说，mmap更加简单，调用更加方便，所以这也是大家都喜欢用的原因。
3、另外mmap有一个好处是当机器重启，因为mmap把文件保存在磁盘上，这个文件还保存了操作系统同步的映像，
所以mmap不会丢失，但是shmget就会丢失。

*/

//===================mmap=========================================//
void *CreateAndMapSharedObj(const char *const name, const size_t size)
{
    void *shmptr = NULL;
    int   shmdes = -1;
    char  shmName[128];

    snprintf(shmName,sizeof(shmName),"%s-%d",name,getuid());
    if ((shmdes = shm_open(shmName, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG)) == -1)
    {
        printf("opening shared memory '%s' failed, errno %d",shmName,errno);
        goto error_out;
    }

    if (ftruncate(shmdes, size) == -1)
    {
        printf("resizing shared memory '%s' to %zu failed, errno %d",shmName, size, errno);
        shm_unlink(shmName);
        goto error_out;
    }

    if ((shmptr = mmap(NULL,size,PROT_WRITE | PROT_READ, MAP_SHARED,shmdes,0)) == (void *)(-1))
    {
        printf("mapping shared memory '%s' failed, errno %d",shmName,errno);
        shm_unlink(shmName);
        goto error_out;
    }

    (void)close(shmdes);
    return shmptr;

error_out:
    if (shmdes >= 0)
    {
        (void)close(shmdes);
    }
    return NULL;
}

bool UnmapShmObj(void *ptr, const size_t size)
{
    if (munmap(ptr, size) == -1)
    {
        printf("unmapping shared memory at %p failed", ptr);

        return false;
    }

    return true;
}

bool CleanupShmObj(char const *name)
{
    (void)shm_unlink(name);

    return true;
}

//===================shm=========================================//
void *CreateAndMapSystemVShm(const char *const name, const size_t size)
{
    int shmdes;
    void *shmptr;
    /* Create shared memory object */
    if ((shmdes = shmget((getuid() + 0xE5500000 + name),
                         size,
                         IPC_CREAT | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) == -1)
    {
        LOG(Error, "opening shared memory '%c' failed", name);

        goto error_handling;
    }

    /* Mmap the object to the address space of the current EE */
    if ((shmptr = shmat(shmdes, NULL, 0)) == (caddr_t) - 1)
    {
        LOG(Error, "mapping shared memory '%c' failed", name);

        goto error_handling;
    }

    return shmptr;

error_handling:

    return NULL;
}

bool UnmapSystemVShm(void *ptr, const size_t size)
{
    (void)size;

    /* Munmap the object from the address space of the current EE */
    if (shmdt(ptr) == -1)
    {
        printf("unmapping shared memory at %p failed", ptr);

        goto error_handling;
    }

    return true;

error_handling:

    return false;
}

bool CleanupSystemVShm(const char name, const size_t size)
{
    int shmdes;

    /* Open shared memory object */
    if ((shmdes = shmget((getuid() + 0xE5500000 + name),
                         size,
                         S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) == -1)
    {
        printf("opening shared memory '%c' failed", name);

        goto error_handling;
    }

    if (shmctl(shmdes, IPC_RMID, NULL) == -1)
    {
        printf("removing shared memory '%c' failed", name);

        goto error_handling;
    }

    return true;

error_handling:

    return false;
}


/*
linux�е����ֹ����ڴ档
һ�������ǵ�IPCͨ��System V�汾�Ĺ����ڴ棬
�����һ�־������ǽ����ᵽ�Ĵ洢ӳ��I/O��mmap������

�ܽ�mmap��shm:
1��mmap���ڴ����Ͻ���һ���ļ���ÿ�����̵�ַ�ռ��п��ٳ�һ��ռ����ӳ�䡣
������shm���ԣ�shmÿ���������ջ�ӳ�䵽ͬһ�������ڴ档
shm�����������ڴ棬������д���ٶ�Ҫ�ȴ���Ҫ�죬���Ǵ洢�������ر��
2�������shm��˵��mmap���Ӽ򵥣����ø��ӷ��㣬������Ҳ�Ǵ�Ҷ�ϲ���õ�ԭ��
3������mmap��һ���ô��ǵ�������������Ϊmmap���ļ������ڴ����ϣ�����ļ��������˲���ϵͳͬ����ӳ��
����mmap���ᶪʧ������shmget�ͻᶪʧ��

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


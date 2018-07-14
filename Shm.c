void *CreateAndMapSharedMemory(const char *const name, const size_t size)
{
    void *shmptr = GLO_NULL;
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

    if ((shmptr = mmap(GLO_NULL,size,PROT_WRITE | PROT_READ, MAP_SHARED,shmdes,0)) == (void *)(-1))
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
    return GLO_NULL;
}


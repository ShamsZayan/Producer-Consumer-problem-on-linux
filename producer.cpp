#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/shm.h>
#include <random>
#include <signal.h>
#include <sys/time.h>
using namespace std;
struct buffer
{
  char name[20];
  float price;
};
struct buffer *_data;
int mutex_sem, empty_sem, full_sem, buff_sem;
void inthand(int signum)
{
  shmdt(_data);
  // remove semaphores
  if (semctl(mutex_sem, 0, IPC_RMID) == -1)
  {
    perror("semctl IPC_RMID");
    exit(1);
  }
  if (semctl(empty_sem, 0, IPC_RMID) == -1)
  {
    perror("semctl IPC_RMID");
    exit(1);
  }
  if (semctl(full_sem, 0, IPC_RMID) == -1)
  {
    perror("semctl IPC_RMID");
    exit(1);
  }
  if (semctl(buff_sem, 0, IPC_RMID) == -1)
  {
    perror("semctl IPC_RMID");
    exit(1);
  }
}


#define SEM_MUTEX_KEY 0x54321
#define SEM_BUFFER_KEY 0x4321
#define SEM_BUFFER_COUNT_KEY 0x56781
#define SEM_SPOOL_SIGNAL_KEY 0x32145
int buffer_index, buffer_size;


int main(int argc, char *argv[])
{
  struct timespec start;
  if (argc < 6)
  {
    printf("arguments not enough\n");
    exit(0);
  }
  buffer_size = stoi(argv[5]);
  signal(SIGINT, inthand);
  /* for semaphore */
  key_t s_key;
  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort array[1];
  } sem_attr;
  mutex_sem = semget(SEM_MUTEX_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
  if (mutex_sem >= 0)
  {
    printf("First Process\n");
    sem_attr.val = 1;
    if (semctl(mutex_sem, 0, SETVAL, sem_attr) == -1)
    {
      perror("semctl SETVAL");
      exit(1);
    }
  }
  else if (errno == EEXIST)
  { // Already other process got it
    printf("Second Process\n");
    mutex_sem = semget(SEM_MUTEX_KEY, 1, 0);
    if (mutex_sem < 0)
    {
      perror("Semaphore GET: ");
      exit(1);
    }
  }
  empty_sem = semget(SEM_BUFFER_COUNT_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
  if (empty_sem >= 0)
  {

    sem_attr.val = buffer_size;
    if (semctl(empty_sem, 0, SETVAL, sem_attr) == -1)
    {
      perror("semctl SETVAL");
      exit(1);
    }
  }
  else if (errno == EEXIST)
  { // Already other process got it

    empty_sem = semget(SEM_BUFFER_COUNT_KEY, 1, 0);
    if (empty_sem < 0)
    {
      perror("Semaphore GET: ");
      exit(1);
    }
  }
  // counting semaphore, indicating the number of strings to be printed.
  /* generate a key for creating semaphore  */
  full_sem = semget(SEM_SPOOL_SIGNAL_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
  if (full_sem >= 0)
  {

    sem_attr.val = 0;
    if (semctl(full_sem, 0, SETVAL, sem_attr) == -1)
    {
      perror("semctl SETVAL");
      exit(1);
    }
  }
  else if (errno == EEXIST)
  { // Already other process got it

    full_sem = semget(SEM_SPOOL_SIGNAL_KEY, 1, 0);
    if (full_sem < 0)
    {
      perror("Semaphore GET: ");
      exit(1);
    }
  }
  buff_sem = semget(SEM_BUFFER_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
  if (buff_sem >= 0)
  {

    sem_attr.val = 0;
    if (semctl(buff_sem, 0, SETVAL, sem_attr) == -1)
    {
      perror("semctl SETVAL");
      exit(1);
    }
  }
  else if (errno == EEXIST)
  { // Already other process got it

    buff_sem = semget(SEM_BUFFER_KEY, 1, 0);
    if (buff_sem < 0)
    {
      perror("Semaphore GET: ");
      exit(1);
    }
  }
  struct sembuf asem[1];
  asem[0].sem_num = 0;
  asem[0].sem_op = 0;
  asem[0].sem_flg = 0;

  // key_t key = ftok("shmfile",65);
  // shmget returns an identifier in shmid
  int shmid = shmget(5000, sizeof(struct buffer) * buffer_size, 0666 | IPC_CREAT);
  _data = (struct buffer *)shmat(shmid, (void *)0, 0);
  // shmat to attach to shared memory
  float info[3];
  info[0] = stof(argv[2]);
  info[1] = stof(argv[3]);
  info[2] = stoi(argv[4]);
  std::default_random_engine generator;
  std::normal_distribution<double> distribution(info[0], info[1]);
  while (true)
  {

    // get a buffer: P (buffer_count_sem);
    float generate = distribution(generator);
    // if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
    //   perror( "clock gettime" );
    //   exit( EXIT_FAILURE );
    // }

   timeval curTime;
gettimeofday(&curTime, NULL);
int milli = curTime.tv_usec ;
char buffer [80];
strftime(buffer, 80, "%Y/%m/%d %H:%M:%S", localtime(&curTime.tv_sec));
char currentTime[84] = "";
sprintf(currentTime, "%s.%03d", buffer, milli);
    cerr<<"["<<currentTime<<"]"<<" "<<argv[1]<<": generating a new value "<< generate<<endl;
    asem[0].sem_op = -1;
    if (semop(empty_sem, asem, 1) == -1)
    {
      perror("semop: buffer_count_sem");
      exit(1);
    }

    /* There might be multiple producers. We must ensure that
        only one producer uses buffer_index at a time.  */
    // P (mutex_sem);
        
   gettimeofday(&curTime, NULL);
   milli = curTime.tv_usec;
   strftime(buffer, 80, "%Y/%m/%d %H:%M:%S", localtime(&curTime.tv_sec));

sprintf(currentTime, "%s.%03d", buffer, milli);

    cerr<<"["<<currentTime<<"]"<<" "<<argv[1]<<": trying to get mutex on shared buffer"<<endl;
    asem[0].sem_op = -1;
    if (semop(mutex_sem, asem, 1) == -1)
    {
      perror("semop: mutex_sem");
      exit(1);
    }

    // Critical section
    buffer_index = semctl(buff_sem, 0, GETVAL, sem_attr);
    strcpy(_data[buffer_index].name, argv[1]);
    _data[buffer_index].price = generate;
    
gettimeofday(&curTime, NULL);
milli = curTime.tv_usec ;


strftime(buffer, 80, "%Y/%m/%d %H:%M:%S", localtime(&curTime.tv_sec));


sprintf(currentTime, "%s.%03d", buffer, milli);

    cerr<<"["<<currentTime<<"]"<<" "<<argv[1]<<": placing "<< _data[buffer_index].price<<" on shared buffer "<<endl;
    //cout << buffer_index << endl;
    asem[0].sem_op = 1;
    if (semop(buff_sem, asem, 1) == -1)
    {
      perror("semop: buffer_sem");
      exit(1);
    }
    buffer_index = semctl(buff_sem, 0, GETVAL, sem_attr);
    if (buffer_index == buffer_size)
    {
      sem_attr.val = 0;
      if (semctl(buff_sem, 0, SETVAL, sem_attr) == -1)
      {
        perror("semctl SETVAL");
        exit(1);
      }
    }

    // Release mutex sem: V (mutex_sem)
    asem[0].sem_op = 1;
    if (semop(mutex_sem, asem, 1) == -1)
    {
      perror("semop: mutex_sem");
      exit(1);
    }
    // Tell spooler that there is a string to print: V (spool_signal_sem);
    asem[0].sem_op = 1;
    if (semop(full_sem, asem, 1) == -1)
    {
      perror("semop: spool_signal_sem");
      exit(1);
    }

    // Take a nap
    sleep(info[2] / 1000);
  }
  exit(0);
}

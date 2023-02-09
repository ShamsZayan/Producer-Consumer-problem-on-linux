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
#include <vector>
#include <numeric>
#include <math.h>
#include <signal.h>
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
int buffer_index = 0, buffer_size;

template <typename T>
double getAverage(vector<T> const &v)
{
  if (v.empty())
  {
    return 0;
  }
  else if (v.size() < 5)
  {
    return reduce(v.begin(), v.end(), 0.0) / 5;
  }

  return reduce(v.end() - 4, v.end(), 0.0) / 5;
}
template <typename S>
double getprevAverage(vector<S> const &v)
{
  if (v.empty())
  {
    return 0;
  }
  else if (v.size() == 1)
  {
    return 0;
  }
  else if (v.size() < 6)
  {
    return reduce(v.begin(), v.end() - 1, 0.0) / 5;
  }

  return reduce(v.end() - 5, v.end() - 1, 0.0) / 5;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("arguments not enough\n");
    exit(0);
  }
  buffer_size = stoi(argv[1]);
  vector<float> GOLD;
  vector<float> SILVER;
  vector<float> CRUDEOIL;
  vector<float> NATURALGAS;
  vector<float> ALUMINIUM;
  vector<float> COPPER;
  vector<float> NICKEL;
  vector<float> LEAD;
  vector<float> ZINC;
  vector<float> MENTHAOIL;
  vector<float> COTTON;

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
  // shmat to attach to shared memory
  _data = (struct buffer *)shmat(shmid, (void *)0, 0);
  while (true)
  {
    // get a buffer: P (buffer_count_sem);
    asem[0].sem_op = -1;
    if (semop(full_sem, asem, 1) == -1)
    {
      perror("semop: buffer_count_sem");
      exit(1);
    }

    /* There might be multiple producers. We must ensure that
        only one producer uses buffer_index at a time.  */
    // P (mutex_sem);
    asem[0].sem_op = -1;
    if (semop(mutex_sem, asem, 1) == -1)
    {
      perror("semop: mutex_sem");
      exit(1);
    }
    // cout<<data[buffer_index].name<<endl;
    // Critical section
    if (strcmp(_data[buffer_index].name, "GOLD") == 0)
    {
      GOLD.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "SILVER") == 0)
    {
      SILVER.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "CRUDEOIL") == 0)
    {
      CRUDEOIL.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "NATURALGAS") == 0)
    {
      NATURALGAS.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "ALUMINIUM") == 0)
    {
      ALUMINIUM.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "COPPER") == 0)
    {
      COPPER.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "NICKEL") == 0)
    {
      NICKEL.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "LEAD") == 0)
    {
      LEAD.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "ZINC") == 0)
    {
      ZINC.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "MENTHAOIL") == 0)
    {
      MENTHAOIL.push_back(_data[buffer_index].price);
    }
    else if (strcmp(_data[buffer_index].name, "COTTON") == 0)
    {
      COTTON.push_back(_data[buffer_index].price);
    }
    buffer_index++;
    if (buffer_index == buffer_size)
      buffer_index = 0;

    // Release mutex sem: V (mutex_sem)
    asem[0].sem_op = 1;
    if (semop(mutex_sem, asem, 1) == -1)
    {
      perror("semop: mutex_sem");
      exit(1);
    }
    // Tell spooler that there is a string to print: V (spool_signal_sem);
    asem[0].sem_op = 1;
    if (semop(empty_sem, asem, 1) == -1)
    {
      perror("semop: spool_signal_sem");
      exit(1);
    }

    cout << "+-------------------------------------+" << endl;
    cout << "| Currency      |  Price   | AvgPrice |" << endl;
    cout << "+-------------------------------------+" << endl;

    if (GOLD.empty())
    {

      cout << "| GOLD          |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(GOLD) > getprevAverage(GOLD))
      {
        printf("| GOLD          | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", GOLD.back(), getAverage(GOLD));
      }
      else
      {
        printf("| GOLD          | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", GOLD.back(), getAverage(GOLD));
      }
    }
    if (SILVER.empty())
    {
      cout << "| SILVER        |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(SILVER) > getprevAverage(SILVER))
      {
        printf("| SILVER        | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", SILVER.back(), getAverage(SILVER));
      }
      else
      {
        printf("| SILVER        | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", SILVER.back(), getAverage(SILVER));
      }
    }
    if (CRUDEOIL.empty())
    {
      cout << "| CRUDEOIL      |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(CRUDEOIL) > getprevAverage(CRUDEOIL))
      {
        printf("| CRUDEOIL      | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", CRUDEOIL.back(), getAverage(CRUDEOIL));
      }
      else
      {
        printf("| CRUDEOIL      | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", CRUDEOIL.back(), getAverage(CRUDEOIL));
      }
    }
    if (NATURALGAS.empty())
    {
      cout << "| NATURALGAS    |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(NATURALGAS) > getprevAverage(NATURALGAS))
      {
        printf("| NATURALGAS    | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", NATURALGAS.back(), getAverage(NATURALGAS));
      }
      else
      {
        printf("| NATURALGAS    | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", NATURALGAS.back(), getAverage(NATURALGAS));
      }
    }
    if (ALUMINIUM.empty())
    {
      cout << "| ALUMINIUM     |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(ALUMINIUM) > getprevAverage(ALUMINIUM))
      {
        printf("| ALUMINIUM     | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", ALUMINIUM.back(), getAverage(ALUMINIUM));
      }
      else
      {
        printf("| ALUMINIUM     | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", ALUMINIUM.back(), getAverage(ALUMINIUM));
      }
    }
    if (COPPER.empty())
    {
      cout << "| COPPER        |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(COPPER) > getprevAverage(COPPER))
      {
        printf("| COPPER        | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", COPPER.back(), getAverage(COPPER));
      }
      else
      {
        printf("| COPPER        | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", COPPER.back(), getAverage(COPPER));
      }
    }
    if (NICKEL.empty())
    {
      cout << "| NICKEL        |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(NICKEL) > getprevAverage(NICKEL))
      {
        printf("| NICKEL        | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", NICKEL.back(), getAverage(NICKEL));
      }
      else
      {
        printf("| NICKEL        | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", NICKEL.back(), getAverage(NICKEL));
      }
    }
    if (LEAD.empty())
    {
      cout << "| LEAD          |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(LEAD) > getprevAverage(LEAD))
      {

        printf("| LEAD          | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", LEAD.back(), getAverage(LEAD));
      }
      else
      {
        printf("| LEAD          | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", LEAD.back(), getAverage(LEAD));
      }
    }
    if (ZINC.empty())
    {
      cout << "| ZINC          |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(ZINC) > getprevAverage(ZINC))
      {
        printf("| ZINC          | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", ZINC.back(), getAverage(ZINC));
      }
      else
      {
        printf("| ZINC          | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", ZINC.back(), getAverage(ZINC));
      }
    }
    if (MENTHAOIL.empty())
    {
      cout << "| MENTHAOIL     |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(MENTHAOIL) > getprevAverage(MENTHAOIL))
      {
        printf("| MENTHAOIL     | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", MENTHAOIL.back(), getAverage(MENTHAOIL));
      }
      else
      {
        printf("| MENTHAOIL     | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", MENTHAOIL.back(), getAverage(MENTHAOIL));
      }
    }
    if (COTTON.empty())
    {
      cout << "| COTTON        |    \033[;34m0.00\033[0m  |    \033[;34m0.00\033[0m  |" << endl;
    }
    else
    {
      if (getAverage(COTTON) > getprevAverage(NATURALGAS))
      {
        printf("| COTTON        | \033[;32m%7.2lf↑\033[0m | \033[;32m%7.2lf↑\033[0m |\n", COTTON.back(), getAverage(COTTON));
      }
      else
      {
        printf("| COTTON        | \033[;31m%7.2lf↓\033[0m | \033[;31m%7.2lf↓\033[0m |\n", COTTON.back(), getAverage(COTTON));
      }
    }
    cout << "+-------------------------------------+" << endl;

    printf("\e[1;1H\e[2J");
  }

  exit(0);
}

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <getopt.h>
#ifdef __MACH__
#include <unistd.h>
#elif __linux__
#include <time.h>
#endif

static __inline__ unsigned long long rdtscp(unsigned *cpuid)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(*cpuid));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

void
localsleep(int amount)
{
#ifdef __MACH__
  usleep(amount);
#elif __linux__
  struct timespec res;
  res.tv_sec = 0;
  res.tv_nsec = amount;

  clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &res, NULL);
#endif
}

int
main(int argc, char **argv)
{
  int ntimes;
  int size;
  int noise;
  int rank;
  int i, j;
  float *buf;
  int bufsize;
  int nwork;
  static int debug;
  unsigned long long finish, start;
  unsigned oldcpuid, cpuid;

  noise = 0;
  ntimes = 10;
  bufsize = 4096;
  nwork = 1000;
  for(;;) {
    int c;
    static struct option long_options[] =
      {
        /* These options set a flag. */
        {"debug", no_argument, &debug, 1},
        /* These options don’t set a flag.
           We distinguish them by their indices. */
        {"noise",  required_argument, 0, 'n'},
        {"bufsize",  required_argument, 0, 'z'},
        {"work",  required_argument, 0, 'w'},
        {"times", required_argument, 0, 't'},
        {0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "n:z:",
                     long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
        printf (" with arg %s", optarg);
      printf ("\n");
      break;

    case 'w':
      nwork = atoi(optarg);
      break;

    case 'n':
      noise = atoi(optarg);
      break;

    case 'z':
      bufsize = atoi(optarg);
      break;

    case 't':
      ntimes = atoi(optarg);
      break;

    case '?':
      /* getopt_long already printed an error message. */
      break;

    default:
      abort ();
    }
  }
  if(debug)
    printf("allocating buffer\n");
  buf = malloc(sizeof(float)*bufsize);
  MPI_Init(&argc, &argv);
  MPI_Comm_rank( MPI_COMM_WORLD, &rank);
  MPI_Comm_size( MPI_COMM_WORLD, &size);
  // need the buffer size
  if(debug)
    printf("size %d\n", size);
  for(i = 0; i < ntimes; i++) {
    if(debug)
      printf("rank %d size/2 %d\n", rank, size/2);
    start = rdtscp(&oldcpuid);
    localsleep(nwork);
    finish = rdtscp(&cpuid);
    if(oldcpuid == cpuid)
      printf("%d %d work %d %d %d %llu\n", rank, i, size, bufsize, noise, finish-start);

    start = rdtscp(&cpuid);
    if(rank < size/2) {
      if(noise)
        localsleep(noise*rank);
      if(debug)
        printf("rank %d starting send to %d\n", rank, size/2+rank);
      MPI_Send(buf, bufsize, MPI_FLOAT, size/2+rank,  0, MPI_COMM_WORLD);
    } else {
      if(debug)
        printf("rank %d starting recv from %d\n", rank, rank - size/2);
      MPI_Recv(buf, bufsize, MPI_FLOAT, rank - size/2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
      finish = rdtscp(&cpuid);
      if(oldcpuid == cpuid)
        printf("%d %d comm %d %d %d %llu\n", rank, i, size, bufsize, noise, finish-start);
      else
        if(debug)
          fprintf(stderr, "warning: rank %d iteration %d migrated cpus from %d to %d\n", rank, i, oldcpuid, cpuid);
    }
      free(buf);
      MPI_Finalize();
      exit(0);
    }

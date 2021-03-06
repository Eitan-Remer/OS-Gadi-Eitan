/* 
 * tsh - A tiny shell program with job control
 * 
 * <Gadi Polster: 800553989 and Eitan Remer: 800615624>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
/*
*Usefull methods:
* sigemptyset
* sigaddset
* sigprocmask
* sigsuspend
* waitpid
* open
* dup2
* setpgid
* kill
*/

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */
int counterToFix = 0;
pid_t stoppedPID;
int stoppedI;
int stoppedSIG;
int imTiredOfLife = 0;


struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);   // (70 lines) More or less 
int builtin_cmd(char **argv);   // (25 lines) More or less
void do_bgfg(char **argv);  // (50 lines) NEED TO DO
void waitfg(pid_t pid); // (25 lines) NEED TO DO

void sigchld_handler(int sig);  // (80 lines) NEED TO DO
void sigtstp_handler(int sig);  // (15 lines) NEED TO DO
void sigint_handler(int sig);   // (15 lines) NEED TO DO

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	    default:
            usage();
	    }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/
//now theres a change
void eval(char *cmdline) 
{
    
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    counterToFix++;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    
    strcpy(buf, cmdline);
    //moved the loop again, put in these print statements to see where/how many times it entered 
    //printf("  start  ");
    bg = parseline(buf, argv); 
    // if ((pid = fork()) == 0) {
    // } else {
        
    // }
    
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */

    if (!builtin_cmd(argv)) { 
        //sigprocmask(SIG_BLOCK, &mask, 0);
       // printf("  not a buitin command ");
        if ((pid = fork()) == 0) {  
            //printf(" child\n"); /* Child runs user job */
            //addjob(jobs,pid, bg+1, cmdline);
            setpgid(0, 0);
            sigprocmask(SIG_UNBLOCK, &mask, 0);
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                // exit(0);
                return;
                // exit(0);
            }
            //printf(" after\n");
        } else {
            //addjob(jobs,pid, bg+1, buf);
            //printf(" parent\n");
            
        }
    
	/* Parent waits for foreground job to terminate */
            
        // int i;
        // for (i = 0; i < MAXJOBS; i++) {
        //     if (jobs[i].pid == 0) {
                // if (jobs[i].state == BG && jobs[i].jid == nextjid) {
        //printf("%d, pid", pid);
            // printf("hello\n");

            if(counterToFix % 2 == 0){
                // printf("hello\n");
                addjob(jobs,pid, bg+1, buf);
            } 
        
        //sigprocmask(SIG_UNBLOCK, &mask, 0);
                //listjobs
        //     }
        // }
        if (!bg) {
            waitfg(pid);
            //int status;
            //if (waitpid(pid, &status, 0) < 0)
                //unix_error("waitfg: waitpid error");
            

        }else{
            // addjob(jobs,pid, bg+1, buf);
        //printf("hi");
        //this is the code that we moved, prints a two where it should print a one, 
        //still not sure if its the right place, but i think it is
            do_bgfg(argv);
            printf("%s", cmdline);
        }
    } else {
        if(strcmp(argv[0], "bg") == 0) {
            if (jobs[stoppedI].state == ST && jobs[stoppedI].jid == stoppedPID ){
                jobs[stoppedI].state = BG; 
                kill(-jobs[stoppedI].pid, SIGCONT);
                printf("[%d] (%d) %s", jobs[stoppedI].jid, jobs[stoppedI].pid, jobs[stoppedI].cmdline);
            }
            //pthread_kill(jobs[stoppedI].pid, SIGCONT);  
        }                            // Send SIGCONT signal to entire group of the given job
    }
        //printf("potato");
        
    
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'') {
            buf++;
            delim = strchr(buf, '\'');
        }
        else {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
/*
* If its a builtin command, return 1, otherwise 0
*/
int builtin_cmd(char **argv) {
    
    //printf(" in builtin cmd ");
    //I believe that executing immediately means 0, its also possible that bg and fg should have different return values
    //not sure if bg means it shoukd never be terminated, or it should wait until the end
    //pretty sure it probably should be run immediately, and just never terminated
    if(!strcmp(argv[0],"bg")){
        
        //almost positive that this should be in eval, the textbook/website code is only for a shell that has 2 commands
        //so we will likely have to adjust eval and that will include using this method there
        //built in command just tells us what we should do, but other methods will actually do it
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(argv[0],"fg")){
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(argv[0],"jobs")){
        //printf("IN JOBS");
        listjobs(jobs);
        // int i;
        // //printf("hi");
        // for (i = 0; i < MAXJOBS; i++) {
        //     if (jobs[i].pid != 0) {
        //         if (jobs[i].state == BG) {
        //             printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
        //         }
        //         //listjobs
        //     }
        // }
        return 1;
        
    } 
    if (!strcmp(argv[0], "quit")){ /* quit command */
        // sigchld_handler(0); //NEED TO FIGURE OUT WHAT TO PASS IN
	    exit(0);  
    }
    if (!strcmp(argv[0], "&")) {    /* Ignore singleton & */
	    return 1;
    }
    
    return 0;       /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
//check page 790
//not sure if this is where, but the basic idea/purpose of execve is to run a program
//this would be done by forking, the child would execute and the parent would continue being a shell (minute 17 in recitation)
void do_bgfg(char **argv) {
    
    int i;
    //int status;
    int is_job_id = 0;

    if (argv[1] == NULL) {                                     
        //printf("%s command requires PID or %jobid argument\n", argv[0]);
        return;
    }
    if((argv[1][0] == '%')) {
        // if(atoi(&argv[1][0]) > maxjid(jobs)){
        //     printf("(%s): No such job\n", argv[1]);
        //     return;
        // }
        is_job_id = 1;
    } else if (!isdigit(argv[1][0])) {                                     
        //printf("%s: argument must be a PID or %jobid\n", argv[0]);
        return;
    }
    // if(atoi(&argv[1][0]) > maxjid(jobs)){
    //     printf("(%s): No such process\n", argv[1]);
    //     return;
    // }
    // if(getjobjid(jobs,atoi(&argv[1][1])) == NULL){
    //     printf("%s: No such job\n", argv[1]);
    //     return;
    // }
    // if((argv[1][0] == '%') && atoi(&argv[1][1]) <= maxjid(jobs)){
    //     struct job_t* j = getjobjid(jobs, atoi(&argv[1][1]));
    //     if(j->state ==ST){
    //         printf("Job [%d] (%d) Stopped by signal %d\n", j->jid, j->pid, stoppedSIG);
    //     }
    // }
    
    if (is_job_id && strcmp(argv[0],"bg")) {
        // if (jobs[atoi(&argv[1][1])].state != FG)
        //     jobs[atoi(&argv[1][1])].state = FG;
        // else
        //     jobs[atoi(&argv[1][1])].state = ST; 
        struct job_t* j = getjobjid(jobs, atoi(&argv[1][1]));
        if (j->state != FG){
            j->state = FG;
            
        }else{
            // j->state = ST; 
            //listjobs(jobs);
            // struct job_t* j = getjobjid(jobs, atoi(&argv[1][1]));
            // printf("Job [%d] (%d) Stopped by signal %d\n", j->jid, j->pid, stoppedSIG);
            }
        waitfg(j->pid);
        
    }

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            
            if (jobs[i].state == BG && jobs[i].jid == nextjid -1) {
                //printf("   %c   \n", jobs[i]);
                printf("[%d] (%d) %s", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            //listjobs
        }
    }
    
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    
    while(fgpid(jobs) == pid){ //runs a endless loop till fg job is done then exits
    sleep(1);
  }	     
   
  return;
    //printf("ENTERED WAIT FG");
    // int status;
    // if (waitpid(pid, &status, 0) > 1){
    //     if (WIFEXITED(status)){
    //         //printf("child %d terminated normally with exit status=%d\n", pid, WEXITSTATUS(status));
    //     } else{
    //         //printf("child %d terminated abnormally\n", pid);
    //     } 
    // } else {
    //     //printf("not terminated\n");
    // }
    // // waitpid(pid,NULL,0);
    // //while(1){
    //     // if(pid != ){

    //     // }else{
    //         //waitpid(pid,&status,0);
    //     // }
    // //}
    // return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) {
    
    int status;  
    pid_t pid;  
    //printf("sigchild\n");
    //pid = waitpid(fgpid(jobs), &status, WNOHANG|WUNTRACED);
    //printf("%d\n", fgpid(jobs));
    // deletejob(jobs, fgpid(jobs));

    while ((pid = waitpid(fgpid(jobs), &status, WNOHANG|WUNTRACED)) > 0) {  
        //printf("enters while loop\n");
        if (WIFSTOPPED(status)){ 
            //change state if stopped
            getjobpid(jobs, pid)->state = ST;
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) Stopped by signal %d\n", jid, pid, sig);
        }  
        else if (WIFSIGNALED(status)){
            //delete is signaled
            int jid = pid2jid(pid);  
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, sig);
            deletejob(jobs, pid);
        }  
        else if (WIFEXITED(status)){  
            //exited
            deletejob(jobs, pid);  
        }  
    }  
    
    return; 
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
//check page 799/800
void sigint_handler(int sig) {
    
    kill(-fgpid(jobs), sig);
    
    // int i;
    // pid_t temp;
    // for (i = 0; i < MAXJOBS; i++) {
    //     if (jobs[i].pid != 0) {
            
    //         if (jobs[i].state == FG) {
    //             //printf("   %c   \n", jobs[i]);
    //             //printf("Job [%d] (%d) ", jobs[i].jid, jobs[i].pid);
    //             kill(-jobs[i].pid, sig);
    //             deletejob(jobs, jobs[i].pid);
    //             //printf("terminated by signal %d\n", sig);
    //             // temp = jobs[i].pid;
                
    //         }
    //         //listjobs
    //     }
    // }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    //struct job* job = fgpid(jobs);
    
    stoppedPID = fgpid(jobs);
    stoppedI = 0;
    stoppedSIG = sig;
    kill(-fgpid(jobs), sig);
    
    return;
    // return;
    //printf("STOPPED WAS CALLED\n");
    // int i;
    // // pid_t temp;
    // for (i = 0; i < MAXJOBS; i++) {
    //     if (jobs[i].pid != 0) {
    //         //printf("IN THE STOPPED METHOD FIRST IF\n");

    //         if (jobs[i].state == FG) {
    //             //printf("STOP SHOULD BE CALLED NOW\n");
    //             jobs[i].state = ST;
    //             //printf("   %c   \n", jobs[i]);
    //             //kill(-jobs[i].pid, sig);

    //             stoppedPID = jobs[i].jid;
    //             stoppedI = i;
    //             stoppedSIG = sig;
    //             //printf("Job [%d] (%d) ", jobs[i].jid, jobs[i].pid);
    //             kill(-jobs[i].pid, sig);
    //             //deletejob(jobs, jobs[i].pid);
    //             //printf("stopped by signal %d\n", sig);
    //             // temp = jobs[i].pid;
                
    //         } 
    //         //listjobs
    //     } else if(jobs[i].state == BG) {
    //         //printf("Job [%d] (%d) ", jobs[i].jid, jobs[i].pid);
    //     } 
    // }
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    //printf("added job");
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
            nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            //printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            if(verbose){
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        } else {
            //printf("not zero");
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs)+1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
pid_t Fork1(void) {
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(-pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

void Pause() 
{
    (void)pause();
    return;
}

unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * Wrappers for Unix signal functions 
 ***********************************/

/* $begin sigaction */
// handler_t *Signal(int signum, handler_t *handler) 
// {
//     struct sigaction action, old_action;

//     action.sa_handler = handler;  
//     sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
//     action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

//     if (sigaction(signum, &action, &old_action) < 0)
// 	unix_error("Signal error");
//     return (old_action.sa_handler);
// }
/* $end sigaction */

// void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
// {
//     if (sigprocmask(how, set, oldset) < 0)
// 	unix_error("Sigprocmask error");
//     return;
// }

// void Sigemptyset(sigset_t *set)
// {
//     if (sigemptyset(set) < 0)
// 	unix_error("Sigemptyset error");
//     return;
// }

void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

// void Sigaddset(sigset_t *set, int signum)
// {
//     if (sigaddset(set, signum) < 0)
// 	unix_error("Sigaddset error");
//     return;
// }

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}
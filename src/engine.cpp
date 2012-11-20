#include <boost/thread.hpp>
#include <stdio.h>
#include "engine.h"
#include "entity.h"
#include "utils.h"
#include "common.h"

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>
#include <rtai_shm.h>

#define PRIORITY        0           /* 0 is the maximum */
#define STACK_SIZE      0           /* use default value */
#define MSG_SIZE        0           /* use default value */
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
#include <native/task.h>
#include <native/timer.h>
#endif // HAVE_LIBANALOGY

#ifdef HAVE_LIBRT
#include <errno.h>
#endif // HAVE_LIBRT

namespace dynclamp {

double globalT;
double globalDt = SetGlobalDt(1.0/20e3);
double runTime;
#ifdef REALTIME_ENGINE
double globalTimeOffset = 0.0;
#endif
#ifdef HAVE_LIBLXRT
double realtimeDt;
#endif

////// SIGNAL HANDLING CODE - START /////

bool programRun = true; 
bool trialRun = false; 
boost::mutex programRunMutex;
boost::mutex trialRunMutex;

void TerminationHandler(int signum)
{
        if (signum == SIGINT || signum == SIGHUP) {
                Logger(Critical, "Terminating the program.\n");
                KillProgram();
        }
}

bool SetupSignalCatching()
{
        struct sigaction oldAction, newAction;
        int i, sig[] = {SIGINT, SIGHUP, -1};
        // set up the structure to specify the new action.
        newAction.sa_handler = TerminationHandler;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
        i = 0;
        while (sig[i] != -1) {
                if (sigaction(sig[i], NULL, &oldAction) != 0) {
                        perror("Error on sigaction:");
                        return false;
                }
                if (oldAction.sa_handler != SIG_IGN) {
                        if (sigaction(sig[i], &newAction, NULL) != 0) {
                             perror("Error on sigaction:");
                             return false;
                        }
                }
                i++;
        }
        return true;
}

////// SIGNAL HANDLING CODE - END /////

void SetTrialRun(bool value)
{
        Logger(Debug, "SetTrialRun(bool)\n");
        boost::mutex::scoped_lock lock(trialRunMutex);
        trialRun = value;
}

void KillProgram()
{
        Logger(Debug, "KillProgram()\n");
        TerminateTrial();
        boost::mutex::scoped_lock lock(programRunMutex);
        programRun = false;
}

void TerminateTrial()
{
        Logger(Debug, "TerminateTrial()\n");
        SetTrialRun(false);
}

double SetGlobalDt(double dt)
{
        assert(dt > 0.0);
        globalDt = dt;

#ifdef HAVE_LIBLXRT
        Logger(Debug, "Starting RT timer.\n");
        RTIME period = start_rt_timer(sec2count(dt));
        realtimeDt = count2sec(period);
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", realtimeDt*1e3, 1./realtimeDt);
#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Stopping RT timer.\n");
        stop_rt_timer();
#endif // NO_STOP_RT_TIMER
#endif // HAVE_LIBLXRT

#ifdef HAVE_LIBANALOGY
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", globalDt*1e3, 1./globalDt);
#endif // HAVE_LIBANALOGY

#ifdef HAVE_LIBRT
        struct timespec ts;
        clock_getres(CLOCK_REALTIME, &ts);
        if (ts.tv_nsec != 1) {
                long cycles = round(globalDt * NSEC_PER_SEC / ts.tv_nsec);
                globalDt = cycles * ts.tv_nsec / NSEC_PER_SEC;
        }
        Logger(Debug, "The real time period is %g ms (f = %g Hz).\n", globalDt*1e3, 1./globalDt);
#endif // HAVE_LIBRT

        assert(globalDt > 0.0);
        return globalDt;
}


#if defined(HAVE_LIBLXRT)

void RTSimulation(const std::vector<Entity*>& entities, double tend, bool *retval)
{
        RT_TASK *task;
        RTIME tickPeriod;
        RTIME currentTime, previousTime;
        RTIME start, stop;
        int preambleIterations, flag, i;
        unsigned long taskName;
        size_t nEntities = entities.size();

        *retval = false;

        Logger(Debug, "\n>>>>> REAL TIME THREAD STARTED <<<<<\n\n");

#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Setting periodic mode.\n");
        rt_set_periodic_mode();
#endif

        Logger(Debug, "Starting the RT timer.\n");
        tickPeriod = start_rt_timer(sec2count(globalDt));
        
        Logger(Debug, "Initialising task.\n");
        taskName = nam2num("hybrid_simulator");
        task = rt_task_init(taskName, RT_SCHED_HIGHEST_PRIORITY+1, STACK_SIZE, MSG_SIZE);
        if(task == NULL) {
                Logger(Critical, "Error: cannot initialise real-time task.\n");
                return;
        }

        Logger(Info, "The period is %.6g ms (f = %.6g Hz).\n", count2ms(tickPeriod), 1./count2sec(tickPeriod));

        Logger(Debug, "Switching to hard real time.\n");
        rt_make_hard_real_time();

        Logger(Debug, "Making the task periodic.\n");
        flag = rt_task_make_periodic_relative_ns(task, count2nano(5*tickPeriod), count2nano(tickPeriod));
        if(flag != 0) {
                Logger(Critical, "Error while making the task periodic.\n");
                goto stopRT;
        }
        
        // some sort of preamble
        currentTime = rt_get_time();
        preambleIterations = 0;
        do {
                previousTime = currentTime;
                rt_task_wait_period();
                currentTime = rt_get_time();
                preambleIterations++;
        } while (currentTime == previousTime);
        Logger(Debug, "Performed %d iteration%s in the ``preamble''.\n",
                preambleIterations, (preambleIterations == 1 ? "" : "s"));

        SetGlobalTimeOffset();
        ResetGlobalTime();
        Logger(Debug, "Initialising all the entities.\n");
        for (i=0; i<nEntities; i++) {
                if (!entities[i]->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities[i]->id());
                        return;
                }
        }

        Logger(Important, "Expected duration: %g seconds.\n", tend);
        Logger(Debug, "Starting the main loop.\n");

        start = rt_timer_read();
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
                rt_task_wait_period();
        }
        stop = rt_timer_read();

        Logger(Important, "Elapsed time: %ld.%03ld ms\n",
                (long)(stop - start) / 1000000, (long)(stop - start) % 1000000);

        Logger(Debug, "Finished the main loop.\n");
        Logger(Debug, "Terminating all entities.\n");

        for (i=0; i<nEntities; i++)
                entities[i]->terminate();

stopRT:
        Logger(Debug, "Deleting the task.\n");
        rt_task_delete(task);

#ifndef NO_STOP_RT_TIMER
        Logger(Debug, "Stopping the RT timer.\n");
        stop_rt_timer();
#endif

        Logger(Debug, "\n>>>>> REAL TIME THREAD ENDED <<<<<\n\n");

        *retval = true;
}

#elif defined(HAVE_LIBANALOGY)

RT_TASK dynclamp_rt_task;

class TaskArgs {
public:
        TaskArgs(const std::vector<Entity*>& entities, double tend)
                : m_entities(entities), m_tend(tend) {}
        const std::vector<Entity*>& entities() const { return m_entities; }
        Entity* entity(int i) const { return m_entities[i]; }
        double tend() const { return m_tend; }
        int nEntities() const { return m_entities.size(); }
private:
        const std::vector<Entity*>& m_entities;
        double m_tend;
};

void RTSimulationTask(void *cookie)
{
        TaskArgs *arg = static_cast<TaskArgs*>(cookie);
        int flag, i;
        size_t nEntities = arg->nEntities();
        double tend = arg->tend();
        RTIME start, stop;

        SetGlobalTimeOffset();
        ResetGlobalTime();
        Logger(Debug, "Initialising all the entities.\n");
        for (i=0; i<nEntities; i++) {
                if (!arg->entity(i)->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", arg->entity(i)->id());
                        return;
                }
        }

        Logger(Important, "Expected duration: %g seconds.\n", tend);
        Logger(Debug, "Starting the main loop.\n");

        flag = rt_task_set_periodic(NULL, TM_NOW, NSEC_PER_SEC*GetGlobalDt());
        if (flag != 0) {
                Logger(Critical, "Unable to make the task periodic.\n");
                return;
        }
        start = rt_timer_read();
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        arg->entity(i)->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        arg->entity(i)->step();
                rt_task_wait_period(NULL);
        }
        stop = rt_timer_read();
        rt_task_set_periodic(NULL, TM_NOW, TM_INFINITE);

        Logger(Important, "Elapsed time: %ld.%03ld ms\n",
                (long)(stop - start) / 1000000, (long)(stop - start) % 1000000);

        Logger(Debug, "Finished the main loop.\n");
        Logger(Debug, "Terminating all entities.\n");

        for (i=0; i<nEntities; i++)
                arg->entity(i)->terminate();

        Logger(Debug, "Stopping the RT timer.\n");
}

void RTSimulation(const std::vector<Entity*>& entities, double tend, bool *retval)
{
        TaskArgs cookie(entities, tend);
        int flag;

        *retval = false;

        Logger(Debug, "\n>>>>> REAL TIME THREAD STARTED <<<<<\n\n");

        // Avoids memory swapping for this program
        mlockall(MCL_CURRENT | MCL_FUTURE);
        
        flag = rt_task_create(&dynclamp_rt_task, "dynclamp", 0, 99, T_CPU(3) | T_JOINABLE);
        // Create the task
        if (flag != 0) {
                Logger(Critical, "Unable to create the real-time task (err = %d).\n", flag);
                return;
        }
        Logger(Debug, "Successfully created the real-time task.\n");

        // Start the task
        flag = rt_task_start(&dynclamp_rt_task, RTSimulationTask, &cookie);
        if (flag != 0) {
                Logger(Critical, "Unable to start the real-time task (err = %d).\n", flag);
                rt_task_delete(&dynclamp_rt_task);
        }
        Logger(Debug, "Successfully started the real-time task.\n");

        // Wait for the task to finish
        flag = rt_task_join(&dynclamp_rt_task);
        if (flag == 0) {
                // There's no need to delete the task if we've joined successfully
                Logger(Debug, "Successfully joined with the child task.\n");
        }
        else {
                Logger(Important, "Unable to join with the child task.\n");
                // Delete the task
                Logger(Debug, "Deleting the task.\n");
                rt_task_delete(&dynclamp_rt_task);
        }

        Logger(Debug, "\n>>>>> REAL TIME THREAD ENDED <<<<<\n\n");

        *retval = true;
}

#elif defined(HAVE_LIBRT)

static inline void tsnorm(struct timespec *ts)
{
	while (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec++;
	}
}

static bool CheckPrivileges()
{
        int policy = sched_getscheduler(0);
        struct sched_param param, old_param;

        /* 
         * if we're already running a realtime scheduler
         * then we *should* be able to change things later
         */
        if (policy == SCHED_FIFO || policy == SCHED_RR)
                return true;

        /* first get the current parameters */
        if (sched_getparam(0, &old_param)) {
                Logger(Critical, "Unable to get scheduler parameters.\n");
                return false;
        }
        param = old_param;

        /* try to change to SCHEDULER */
        param.sched_priority = 1;
        if (sched_setscheduler(0, SCHEDULER, &param)) {
                Logger(Critical, "Unable to change scheduling policy!\n");
                Logger(Critical, "Either run as root or join realtime group.\n");
                return false;
        }

        /* we're good; change back and return success */
        return sched_setscheduler(0, policy, &old_param) == 0;
}

static inline int64_t calcdiff_ns(struct timespec t1, struct timespec t2)
{
        int64_t diff;
        diff = NSEC_PER_SEC * (int64_t)((int) t1.tv_sec - (int) t2.tv_sec);
        diff += ((int) t1.tv_nsec - (int) t2.tv_nsec);
        return diff;
}

void RTSimulation(const std::vector<Entity*>& entities, double tend, bool *retval)
{
        int priority, flag, i;
        size_t nEntities = entities.size();
        struct timespec now, period;
        struct sched_param schedp;

        *retval = false;

        priority = sched_get_priority_max(SCHEDULER);
        if (priority < 0) {
                Logger(Critical, "Unable to get maximum priority.\n");
                perror("sched_get_priority_max");
                return;
        }
        Logger(Debug, "The maximum priority is #%d.\n", priority);

	memset(&schedp, 0, sizeof(schedp));
	schedp.sched_priority = priority;
	flag = sched_setscheduler(0, SCHEDULER, &schedp);
        if (flag != 0){
		Logger(Critical, "Unable to set maximum priority.\n");
		return; 
	}
        Logger(Debug, "Successfully set maximum priority.\n");

        // Reset simulation time
        ResetGlobalTime();

        // Initialise all entities
        for (i=0; i<nEntities; i++) {
                if (!entities[i]->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities[i]->id());
                        return;
                }
        }
        Logger(Debug, "Initialised all entities.\n");
        
        // The period of execution of the main loop
	period.tv_sec = 0;
	period.tv_nsec = GetGlobalDt() * NSEC_PER_SEC;

	// Get current time
	flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag < 0) {
                Logger(Critical, "Unable to get time from the system.\n");
                perror("clock_gettime");
                return;
        }

        // Set the time offset, i.e. the absolute time of the beginning of the simulation
        SetGlobalTimeOffset(now);

        Logger(Important, "Expected duration: %g seconds.\n", tend);

        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                
                // Process the events and have all entities read their inputs
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();

                // Compute the time at which the thread will have to resume
	        now.tv_sec += period.tv_sec;
	        now.tv_nsec += period.tv_nsec;
	        tsnorm(&now);

                // Wait for next period
		flag = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &now, NULL);
                if (flag != 0) {
                        Logger(Critical, "Error in clock_nanosleep.\n");
                        break;
                }

                // Increase the time of the simulation and step all entities forward
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
        }

        // Compute how much time has passed since the beginning
        flag = clock_gettime(CLOCK_REALTIME, &now);
        if (flag == 0) {
                Logger(Important, "Elapsed time: %g seconds.\n",
                        ((double) now.tv_sec + ((double) now.tv_nsec / NSEC_PER_SEC)) - GetGlobalTimeOffset());
        }

        // Stop all entities
        for (i=0; i<nEntities; i++)
                entities[i]->terminate();

	// Switch to normal
	schedp.sched_priority = 0;
	flag = sched_setscheduler(0, SCHED_OTHER, &schedp);
        if (flag == 0)
                Logger(Debug, "Switched to non real-time scheduler.\n");

        *retval = true;
}

#else

void NonRTSimulation(const std::vector<Entity*>& entities, double tend, bool *retval)
{        
	int i, nEntities = entities.size();
        double dt = GetGlobalDt();

        *retval = false;

        ResetGlobalTime();

        for (i=0; i<nEntities; i++) {
                if (!entities[i]->initialise()) {
                        Logger(Critical, "Problems while initialising entity #%d. Aborting...\n", entities[i]->id());
                        return;
                }
        }
        while (!TERMINATE_TRIAL() && GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
        }
        for (i=0; i<nEntities; i++)
                entities[i]->terminate();

        *retval = true;
}

#endif // HAVE_LIBLXRT


bool Simulate(const std::vector<Entity*>& entities, double tend)
{
        bool success;

        SetTrialRun(true);
	ResetGlobalTime();
#ifdef HAVE_LIBRT
        if (!CheckPrivileges())
                return success;
#endif
#ifdef REALTIME_ENGINE
        boost::thread thrd(RTSimulation, entities, tend, &success);
#else
        boost::thread thrd(NonRTSimulation, entities, tend, &success);

#endif // REALTIME_ENGINE
        thrd.join();
        if (success)
                Logger(Debug, "Simulation thread has finished running.\n");
        return success;
}

} // namespace dynclamp



/**
 * \file  job_manager.h
 * \brief Definition of JobManager class.
 *
 * FuD: FuDePAN Ubiqutous Distribution, a framework for work distribution.
 * <http://fud.googlecode.com/>
 * Copyright (C) 2009 Guillermo Biset, FuDePAN
 *
 * This file is part of the FuD project.
 *
 * Contents:       Header file for FuD providing class DistributableJob.
 *
 * System:         FuD
 * Homepage:       <http://fud.googlecode.com/>
 * Language:       C++
 *
 * Author:         Guillermo Biset
 * E-Mail:         billybiset AT gmail.com
 *
 * FuD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FuD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FuD.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include <list>

#include <boost/thread.hpp>

#include "distributable_job.h"
#include "job_unit.h"
#include "clients_manager.h"
#include "events.h"
#include "synchronized_containers.h"

namespace fud
{
    /**
     * Interface to be implemented by the handler of events generated by the JobManager.
     * \sa Event
     */
    struct JobManagerEventHandler
    {
        //ClientsManager events
        /**
         * Performs actions to handle a free client.
         * Will likely try to assign a new JobUnit to a ClientProxy. Even though the event
         * corresponds to a particular ClientProxy, concurrent execution can not assure me
         * that this particular client will be used when trying to handle the event. So no
         * real reference to a ClientProxy is used.
         *
         * \sa ClientProxy
         */
        virtual void handle_free_client_event()                                          = 0;

        /**
         * Performs actions to handle the completion of a JobUnit.
         * It will need to locate the corresponding DistributableJob and tell it to handle
         * the message.
         *
         * @param id : The JobUnitID of the completed JobUnit.
         * @param msg : The message from the processign client with the results.
         *
         * \sa DistributableJob
         * \sa JobUnit
         */
        virtual void handle_job_unit_completed_event(JobUnitID id, std::string* msg)     = 0;

        //DistributableJob events
        /**
         * Handles the completion of a DistributableJob.
         * This means that the Job will no longer produce JobUnits, it can thus be unlinked
         * from the project.
         *
         * @param distjob : The finished DistributableJob.
         *
         * \sa DistributableJob
         */
        virtual void handle_distributable_job_completed_event(DistributableJob* distjob) = 0;
    };

    /**
     * The central hub for jobs in the system.
     * Implements all Job handling functionality.
     *
     * \sa ClientsManagerListener
     * \sa DistributableJobListener
     * \sa JobManagerEventHandler
     */
    class JobManager :
        private ClientsManagerListener,
        private DistributableJobListener,
        private JobManagerEventHandler
    {
    public:
        /**
         * Singleton method.
         */
        static JobManager* get_instance();

        /**
         * Returns a pointer to the listener of DistributableJob events.
         *
         * \sa DistributableJobListener
         * \sa DistributableJob
         * \sa Event
         */
        inline DistributableJobListener* const get_distributable_job_listener()
        {
            return this;
        }

        /**
         * Enqueues a DistributableJob in the system.
         * The Job doesn't need to be ready to produce, this just means that the framework
         * will be handling it.
         *
         * \sa DistributableJob
         */
        void   enqueue(DistributableJob* distjob);

        /**
         * Start or resume the scheduler thread.
         * It does nothing if the scheduler thread is currently in a running state.
         *
         * \sa stop_scheduler
         */
        void   start_scheduler();

        /**
         * Pauses the scheduler.
         * The scheduler threads continues to run, this only changes its internal state.
         * It will continue to listen for new events, but won't handle them until a call
         * to start_scheduler is invoked.
         *
         * \sa start_scheduler
         */
        void   stop_scheduler();

        inline ClientsManager* get_clients_manager()
        {
            return _clients_manager;
        }

    private:
        /* Override these, as per -Weffc++ warnings */
        JobManager(const JobManager&);

        JobManager& operator=(const JobManager&);

        enum Status {kStopped, kPaused, kRunning};

        JobManager();

        /*methods*/
        void              run_scheduler();

        DistributableJob* jobs_available();
        bool              job_queue_full(); //const

        void              create_another_job_unit();

        /* Enqueuing ClientsManager events */
        virtual void      free_client_event();
        virtual void      job_unit_completed_event(JobUnitID id, std::string* msg);

        /* Enqueuing DistributableJob events */
        virtual void      distributable_job_completed_event(DistributableJob* distjob);


        /* handling ClientsManager events */
        virtual void handle_free_client_event();
        virtual void handle_job_unit_completed_event(JobUnitID id, std::string* msg);

        /* handling DistributableJob events */
        virtual void handle_distributable_job_completed_event(DistributableJob* distjob);


        void              handle_new_job_event();

        /* local events*/
        void              job_queue_not_full_event();
        void              handle_job_queue_not_full_event();

        /* Attr. */
        static JobManager*              _instance;

        ClientsManager*                 _clients_manager;

        std::list<DistributableJob *>          _producingJobs;
        std::list<DistributableJob *>          _waitingJobs;
        std::list<JobUnit *>                   _jobQueue;
        std::list<JobUnit *>                   _pendingList;
        std::map<JobUnitID, DistributableJob* > _ids_to_job_map;

        JobUnitSize                     _current_job_unit_size;

        Status                          _status;

        boost::mutex                    _mutex;

        LockingQueue<Event<JobManagerEventHandler> *>    _event_queue;
    };
}
#endif


/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    recorders.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef RECORDERS_H
#define RECORDERS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <hdf5.h>
#include <vector>
#include <queue>

#include "utils.h"
#include "entity.h"
#include "common.h"

#include "h5rec.h"

#define NUMBER_OF_EVENTS_DATASETS 3

namespace lcg {

namespace recorders {

class Recorder : public Entity {
public:
        Recorder(uint id = GetId());
        virtual ~Recorder();
        virtual double output();
};

class BaseH5Recorder : public H5RecorderCore, public Recorder {
public:
        BaseH5Recorder(bool compress, hsize_t bufferSize = 20480, const char *filename = NULL, uint id = GetId());
        BaseH5Recorder(bool compress, hsize_t chunkSize, uint numberOfChunks, const char *filename = NULL, uint id = GetId());
        virtual bool initialise();
        virtual void terminate();

protected:
        virtual bool finaliseInit() = 0;
        
        virtual void addPre(Entity *entity);
        virtual void finaliseAddPre(Entity *entity) = 0;

        virtual bool allocateForEntity(Entity *entity, int dataRank,
                                       const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims);

        virtual bool allocateEventsDatasets(int dataRank,
                                       const hsize_t *dataDims, const hsize_t *maxDataDims, const hsize_t *chunkDims);
#ifdef REALTIME_ENGINE
        // sets the priority of the calling thread to max_priority - 1
        virtual void reducePriority() const;
#endif // REALTIME_ENGINE

protected:
        // number of inputs
        uint m_numberOfInputs;
};

class H5Recorder : public BaseH5Recorder {
public:
        H5Recorder(bool compress = true, const char *filename = NULL, uint id = GetId());
        ~H5Recorder();
        virtual void step();
        virtual void firstStep();
        virtual void terminate();
        void handleEvent(const Event *event);
public:
        /*!
         * The number of buffers used for storing the input data: they need to be at least 2,
         * so that the realtime thread writes in one, while the thread created by H5Recorder
         * saves the data in the other buffer to file.
         */
        static const uint numberOfBuffers;
        static const int  rank;

protected:
        virtual bool finaliseInit();
        virtual void finaliseAddPre(Entity *entity);

private:
        void startWriterThread();
        void stopWriterThread();
        static void* buffersWriter(void *arg);

private:
        // the data
        std::vector<double**> m_data;
        // the events data
        int32_t **m_eventsData[NUMBER_OF_EVENTS_DATASETS];
        // the queue of the indices of the buffers to save
        std::deque<uint> m_dataQueue;
        // the queue of the indices of the buffers to save
        std::deque<uint> m_eventsDataQueue;
        
	// position in the buffer
        uint m_bufferPosition;
        // the length of each buffer
        hsize_t *m_bufferLengths;
        uint m_bufferInUse;
        // position in the events buffer
        uint m_eventsBufferPosition;
        // the length of each events buffer
        hsize_t *m_eventsBufferLengths;
        // the index of the events buffer in which the main thread saves data
        uint m_eventsBufferInUse;
        
        // the thread that continuosly waits for data to save
        pthread_t m_writerThread;
        // multithreading stuff for controlling access to the queue m_dataQueue;
        pthread_mutex_t m_mutex;
        pthread_cond_t m_cv;
        bool m_threadRun;
        uint m_runCount;

        hsize_t m_datasetSize;
        hsize_t m_eventsDatasetSize;
};

class TriggeredH5Recorder : public BaseH5Recorder {
public:
        TriggeredH5Recorder(double before, double after, bool compress = true, const char *filename = NULL, uint id = GetId());
        ~TriggeredH5Recorder();
        virtual void step();
        virtual void terminate();
        virtual void handleEvent(const Event *event);

public:
        /*!
         * The number of buffers used for storing the input data: they need to be at least 2,
         * so that the realtime thread writes in one, while the thread created by H5Recorder
         * saves the data in the other buffer to file.
         */
        static const uint numberOfBuffers;
        static const int rank;

protected:
        virtual bool finaliseInit();
        virtual void finaliseAddPre(Entity *entity);

private:
        static void* buffersWriter(void *arg);

private:
        bool m_recording;
        // the data
        std::vector<double**> m_data;
        // a temporary buffer for unrolling the circular buffers
        double *m_tempData;
        // position in the buffer
        uint m_bufferPosition;
        // the index of the buffer in which the main thread saves data
        uint m_bufferInUse;
        // the number of steps to take after a trigger event is received, before writing the buffer to file
        uint m_maxSteps;
        // the number of steps that have been taken
        uint m_nSteps;
        // the thread that saves the data once the buffers are full
        pthread_t m_writerThread;
        hsize_t m_datasetSize[2];
};

} // namespace recorders

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* ASCIIRecorderFactory(string_dict& args);
lcg::Entity* H5RecorderFactory(string_dict& args);
lcg::Entity* TriggeredH5RecorderFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif


#endif


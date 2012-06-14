#include <sstream>
#include <boost/filesystem.hpp>
#include "waveform.h"
#include "generate_trial.h"
#include "events.h"
#include "engine.h"
namespace fs = boost::filesystem;

dynclamp::Entity* WaveformFactory(dictionary& args)
{
        uint id;
        bool triggered;
        std::string filename, units;
        const char *filenamePtr;
        id = dynclamp::GetIdFromDictionary(args);
        if (dynclamp::CheckAndExtractValue(args, "filename", filename))
                filenamePtr = filename.c_str();
        else
                filenamePtr = NULL;
        if (!dynclamp::CheckAndExtractBool(args, "triggered", &triggered))
                triggered = false;
        if (!dynclamp::CheckAndExtractValue(args, "units", units))
                units = "N/A";
        return new dynclamp::generators::Waveform(filenamePtr, triggered, units, id);
}

namespace dynclamp {

namespace generators {

Waveform::Waveform(const char *stimulusFile, bool triggered, const std::string& units, uint id)
        : Generator(id), m_stimulus(NULL), m_stimulusMetadata(NULL),
          m_stimulusLength(0), m_triggered(triggered)
{
        if (stimulusFile != NULL)
            setFilename(stimulusFile);
        setName("Waveform");
        setUnits(units);
}

Waveform::~Waveform()
{
        freeMemory();
}

bool Waveform::initialise()
{
    if (!m_triggered) {
        m_position = 0;
    }
    else {
        m_position = m_stimulusLength + 1;
        Logger(Info, "Waveform is waiting for events.\n");
    }
    return true;
}

bool Waveform::setFilename(const char *filename)
{
        bool retval = true;
        int i, j, flag;
        double **metadata;

        if (!fs::exists(filename)) {
                Logger(Critical, "%s: no such file.\n", filename);
                return false;
        }
        strncpy(m_filename, filename, FILENAME_MAXLEN);

        freeMemory();

        flag = generate_trial(m_filename, GetLoggingLevel() <= Debug,
                              0, NULL, &m_stimulus, &m_stimulusLength,
                              1.0/GetGlobalDt(), GetGlobalDt());

        if (flag == -1) {
                if (m_stimulus != NULL)
                        free(m_stimulus);
                Logger(Critical, "Error in <generate_trial>\n");
                return false;
        }

        metadata = new double*[MAXROWS];
        if (metadata == NULL) {
                Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                return false;
        }
        for (i=0; i<MAXROWS; i++)  { 
                metadata[i] = new double[MAXCOLS];
                if (metadata[i] == NULL) {
                        Logger(Critical, "Unable to allocate memory for <parsed_data>.\n");
                        for (j=0; j<i; i++)
                                delete metadata[j];
                        delete metadata;
                        return false;
                }
        }
        
        if (readmatrix(m_filename, metadata, &m_stimulusRows, &m_stimulusCols) == -1) {
                Logger(Critical, "Unable to parse file [%s].\n", m_filename);
                retval = false;
                goto endSetFilename;
        }

        m_stimulusMetadata = new double[m_stimulusRows * m_stimulusCols];
        for (i=0; i<m_stimulusRows; i++) {
                for (j=0; j<m_stimulusCols; j++) {
                        m_stimulusMetadata[i*m_stimulusCols + j] = metadata[i][j];
                        Logger(Debug, "%7.1lf ", m_stimulusMetadata[i*m_stimulusCols + j]);
                }
                Logger(Debug, "\n");
        }

endSetFilename:
        for (i=0; i<MAXROWS; i++)
                delete metadata[i];
        delete metadata;

        return retval;
}

double Waveform::duration() const
{
        return stimulusLength() * GetGlobalDt();
}

uint Waveform::stimulusLength() const
{
        return m_stimulusLength;
}

bool Waveform::hasNext() const
{
        return true;
}

void Waveform::step()
{
        m_position++;
}

bool Waveform::hasMetadata(size_t *ndims) const
{
        *ndims = 2;
        return true;
}

const double* Waveform::metadata(size_t *dims, char *label) const
{
        sprintf(label, "Stimulus_Matrix");
        dims[0] = m_stimulusRows;
        dims[1] = m_stimulusCols;
        return m_stimulusMetadata;
}

void Waveform::freeMemory()
{
        if (m_stimulus != NULL) {
                delete m_stimulus;
                delete m_stimulusMetadata;
                m_stimulus = NULL;
                m_stimulusMetadata = NULL;
        }
}

/**
 * Note: a RESET event is sent when the waveform ends.
 */
double Waveform::output() const 
{
        if (m_position < m_stimulusLength)
                return m_stimulus[m_position];
        if (m_position == m_stimulusLength)
            emitEvent(new ResetEvent(this));
        return 0.0;
}

void Waveform::handleEvent(const Event *event)
{
        switch(event->type()) {
        case TRIGGER:
                if (m_triggered && m_position >= m_stimulusLength){
                    Logger(Debug, "Waveform: triggered by event.\n");
                    reset();
                }
                break;
        default:
                Logger(Important, "Waveform: unknown event type.\n");
        }
}

void Waveform::reset()
{
        m_position = 0;
}

void Waveform::terminate()
{
        m_position = m_stimulusLength;
}

} // namespace generators

} // namespace dynclamp

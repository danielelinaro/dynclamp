#include "analog_io.h"
#include "utils.h"

#ifdef HAVE_LIBCOMEDI

dynclamp::Entity* AnalogInputFactory(dictionary& args)
{
        uint inputSubdevice, readChannel, range, reference, id;
        std::string deviceFile, rangeStr, referenceStr;
        double inputConversionFactor;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor)) {
                dynclamp::Logger(dynclamp::Debug, "AnalogInputFactory: missing parameter.\n");
                dictionary::iterator it;
                dynclamp::Logger(dynclamp::Critical, "Unable to build an analog input.\n");
                return NULL;
        }

        if (! dynclamp::CheckAndExtractValue(args, "range", rangeStr)) {
                range = PLUS_MINUS_TEN;
        }
        else {
                if (rangeStr.compare("PlusMinusTen") == 0 ||
                    rangeStr.compare("[-10,+10]") == 0 ||
                    rangeStr.compare("+-10") == 0) {
                        range = PLUS_MINUS_TEN;
                }
                else if (rangeStr.compare("PlusMinusFive") == 0 ||
                         rangeStr.compare("[-5,+5]") == 0 ||
                         rangeStr.compare("+-5") == 0) {
                        range = PLUS_MINUS_FIVE;
                }
                else if (rangeStr.compare("PlusMinusOne") == 0 ||
                         rangeStr.compare("[-1,+1]") == 0 ||
                         rangeStr.compare("+-1") == 0) {
                        range = PLUS_MINUS_ONE;
                }
                else if (rangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         rangeStr.compare("[-0.2,+0.2]") == 0 ||
                         rangeStr.compare("+-0.2") == 0) {
                        range = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        dynclamp::Logger(dynclamp::Critical, "Unable to build an analog input.\n");
                        return NULL;
                }
        }

        if (! dynclamp::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        dynclamp::Logger(dynclamp::Critical, "Unable to build an analog input.\n");
                        return NULL;
                }
        }

        return new dynclamp::AnalogInput(deviceFile.c_str(), inputSubdevice, readChannel,
                        inputConversionFactor, range, reference, id);
}

dynclamp::Entity* AnalogOutputFactory(dictionary& args)
{
        uint outputSubdevice, writeChannel, reference, id;
        std::string deviceFile, referenceStr;
        double outputConversionFactor;

        id = dynclamp::GetIdFromDictionary(args);

        if ( ! dynclamp::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! dynclamp::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! dynclamp::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                dynclamp::Logger(dynclamp::Critical, "Unable to build an analog output.\n");
                return NULL;
        }

        if (! dynclamp::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        dynclamp::Logger(dynclamp::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        dynclamp::Logger(dynclamp::Critical, "Unable to build an analog output.\n");
                        return NULL;
                }
        }

        return new dynclamp::AnalogOutput(deviceFile.c_str(), outputSubdevice, writeChannel,
                        outputConversionFactor, reference, id);
}

namespace dynclamp {

AnalogInput::AnalogInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel, double inputConversionFactor,
                         uint range, uint aref,
                         uint id)
        : Entity(id),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, range, aref)
{}

void AnalogInput::initialise()
{
        m_input.initialise();
        m_data = m_input.read();
}

void AnalogInput::step()
{
        m_data = m_input.read();
}

double AnalogInput::output() const
{
        return m_data;
}

//~~~

AnalogOutput::AnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor, uint aref,
                           uint id)
        : Entity(id),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, aref)
{}

AnalogOutput::~AnalogOutput()
{
        terminate();
}

void AnalogOutput::terminate()
{
        m_output.write(0.0);
}

void AnalogOutput::initialise()
{
        m_output.initialise();
        m_output.write(0.0);
}

void AnalogOutput::step()
{
        uint i, n = m_inputs.size();
        m_data = 0.0;
        for (i=0; i<n; i++)
                m_data += m_inputs[i];
        m_output.write(m_data);
}

double AnalogOutput::output() const
{
        return m_data;
}

} // namespace dynclamp

#endif // HAVE_LIBCOMEDI


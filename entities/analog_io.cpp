#include <string.h>
#include "analog_io.h"
#include "utils.h"

lcg::Entity* AnalogInputFactory(string_dict& args)
{
        uint inputSubdevice, readChannel, range, reference, id;
        std::string deviceFile, rangeStr, referenceStr, units;
        double inputConversionFactor;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! lcg::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor)) {
                lcg::Logger(lcg::Debug, "AnalogInputFactory: missing parameter.\n");
                string_dict::iterator it;
                lcg::Logger(lcg::Critical, "Unable to build an analog input.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "range", rangeStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog input.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog input.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "mV";
        }

        return new lcg::AnalogInput(deviceFile.c_str(), inputSubdevice, readChannel,
                                         inputConversionFactor, range, reference, units, id);
}

lcg::Entity* AnalogIOFactory(string_dict& args)
{
        uint inputSubdevice, readChannel, inputRange, outputSubdevice, writeChannel, reference, id;
        std::string deviceFile, rangeStr, referenceStr, units;
        double inputConversionFactor, outputConversionFactor;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! lcg::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! lcg::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                lcg::Logger(lcg::Debug, "AnalogIOFactory: missing parameter.\n");
                lcg::Logger(lcg::Critical, "Unable to build an analog IO.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "inputRange", rangeStr)) {
                inputRange = PLUS_MINUS_TEN;
        }
        else {
                if (rangeStr.compare("PlusMinusTen") == 0 ||
                    rangeStr.compare("[-10,+10]") == 0 ||
                    rangeStr.compare("+-10") == 0) {
                        inputRange = PLUS_MINUS_TEN;
                }
                else if (rangeStr.compare("PlusMinusFive") == 0 ||
                         rangeStr.compare("[-5,+5]") == 0 ||
                         rangeStr.compare("+-5") == 0) {
                        inputRange = PLUS_MINUS_FIVE;
                }
                else if (rangeStr.compare("PlusMinusOne") == 0 ||
                         rangeStr.compare("[-1,+1]") == 0 ||
                         rangeStr.compare("+-1") == 0) {
                        inputRange = PLUS_MINUS_ONE;
                }
                else if (rangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         rangeStr.compare("[-0.2,+0.2]") == 0 ||
                         rangeStr.compare("+-0.2") == 0) {
                        inputRange = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        lcg::Logger(lcg::Critical, "Unknown input range: [%s].\n", rangeStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog IO.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog IO.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "mV";
        }

        return new lcg::AnalogIO(deviceFile.c_str(), inputSubdevice, readChannel,
                                      inputConversionFactor, outputSubdevice,
                                      writeChannel, outputConversionFactor, inputRange,
                                      reference, units, id);
}

lcg::Entity* AnalogOutputFactory(string_dict& args)
{
        uint outputSubdevice, writeChannel, reference, id;
        std::string deviceFile, referenceStr, units;
        double outputConversionFactor;
        bool resetOutput;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! lcg::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor)) {
                lcg::Logger(lcg::Critical, "Unable to build an analog output.\n");
                return NULL;
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
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
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build an analog output.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "units", units)) {
                units = "pA";
        }

        if (! lcg::CheckAndExtractBool(args, "resetOutput", &resetOutput)) {
                resetOutput = (getenv("LCG_RESET_OUTPUT") == NULL ? false :
                        (strcmp(getenv("LCG_RESET_OUTPUT"),"yes") ? false : true));
        }

        return new lcg::AnalogOutput(deviceFile.c_str(), outputSubdevice, writeChannel,
                                          outputConversionFactor, reference, units, resetOutput, id);
}

namespace lcg {

AnalogInput::AnalogInput(const char *deviceFile, uint inputSubdevice,
                         uint readChannel, double inputConversionFactor,
                         uint range, uint aref, const std::string& units,
                         uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, range, aref)
#elif defined(HAVE_LIBANALOGY)
          m_input(deviceFile, inputSubdevice, &readChannel, 1, range, aref)
#endif
{
        setName("AnalogInput");
        setUnits(units);
}

bool AnalogInput::initialise()
{

        if (! m_input.initialise())
                return false;
        m_data = m_input.read();
        return true;
}

void AnalogInput::step()
{
        m_data = m_input.read();
}

double AnalogInput::output()
{
        return m_data;
}

//~~~

AnalogOutput::AnalogOutput(const char *deviceFile, uint outputSubdevice,
                           uint writeChannel, double outputConversionFactor, uint aref,
                           const std::string& units, bool resetOutput, uint id)
        : Entity(id), m_resetOutput(resetOutput),
#if defined(HAVE_LIBCOMEDI)
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, aref, resetOutput)
#elif defined(HAVE_LIBANALOGY)
          m_output(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref, resetOutput)
#endif
{
        setName("AnalogOutput");
        setUnits(units);
}

AnalogOutput::~AnalogOutput()
{
        terminate();
}

void AnalogOutput::terminate()
{
        if (m_resetOutput || ABNORMAL_TERMINATION())
                m_output.write(0.0);
}

bool AnalogOutput::initialise()
{
        if (! m_output.initialise())
                return false;
        if (m_resetOutput)
                m_output.write(0.0);
        return true;
}

void AnalogOutput::step()
{
        uint i, n = m_inputs.size();
        m_data = 0.0;
        for (i=0; i<n; i++)
                m_data += m_inputs[i];
        m_output.write(m_data);
}

double AnalogOutput::output()
{
        return m_data;
}

AnalogIO::AnalogIO(const char *deviceFile, uint inputSubdevice,
                 uint readChannel, double inputConversionFactor,
                 uint outputSubdevice, uint writeChannel, double outputConversionFactor,
                 uint inputRange, uint aref, const std::string& units, uint id)
        : Entity(id),
#if defined(HAVE_LIBCOMEDI)
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, aref),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, aref)
#elif defined(HAVE_LIBANALOGY)
          m_input(deviceFile, inputSubdevice, &readChannel, 1, inputRange, aref),
          m_output(deviceFile, outputSubdevice, &writeChannel, 1, PLUS_MINUS_TEN, aref)
#endif
{
        setName("AnalogIO");
        setUnits(units);
}

AnalogIO::~AnalogIO()
{
        terminate();
}

bool AnalogIO::initialise()
{
        if (! m_output.initialise() ||
            ! m_input.initialise())
                return false;
        m_data = m_input.read();
        m_output.write(0.0);
        return true;
}

void AnalogIO::terminate()
{
        m_output.write(0.0);
}

void AnalogIO::step()
{
        m_data = m_input.read();
        uint i, n = m_inputs.size();
        double output = 0.0;
        for (i=0; i<n; i++)
                output += m_inputs[i];
        m_output.write(output);
}

double AnalogIO::output()
{
        return m_data;
}

} // namespace lcg


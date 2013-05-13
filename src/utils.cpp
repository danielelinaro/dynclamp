#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <sstream>
#include <algorithm>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using boost::property_tree::ptree;

#include "utils.h"
#include "events.h"
#include "entity.h"
#include "types.h"
#include "engine.h"

/* colors */
#define ESC ''
#define RED "[31m"
#define GREEN "[32m"
#define YELLOW "[33m"
#define NORMAL "[00m"

namespace lcg
{

LogLevel verbosity = Info;
uint progressiveId = -1;

void SetLoggingLevel(LogLevel level)
{
	verbosity = level;
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
}

LogLevel GetLoggingLevel()
{
#ifdef NDEBUG
        fprintf(stderr, "Logging has been disabled by defining NDEBUG.\n");
#endif
        return verbosity;
}

#ifndef NDEBUG
void Logger(LogLevel level, const char *fmt, ...)
{
	if (level >= verbosity) {
		va_list argp;
		va_start(argp, fmt);
                switch (level) {
                case Critical:
                        fprintf(stderr, "%c%s", ESC, RED);
                        break;
                case Important:
                        fprintf(stderr, "%c%s", ESC, YELLOW);
                        break;
                }
		vfprintf(stderr, fmt, argp);
                fprintf(stderr, "%c%s", ESC, NORMAL);
		va_end(argp);
	}
}
#endif

void ResetIds()
{
        progressiveId = -1;
}

uint GetId()
{
        progressiveId++;
        return progressiveId;
}

uint GetIdFromDictionary(string_dict& args)
{
        uint id;
        if (args.count("id") == 0) {
                id = GetId();
        }
        else {
                std::stringstream ss(args["id"]);
                ss >> id;
        }
        return id;
}

ullong GetRandomSeed()
{
        ullong seed;
        int fd = open("/dev/urandom",O_RDONLY);
        if (fd == -1) {
                seed = time(NULL);
                Logger(Debug, "seed = time(NULL) = 0x%llx.\n", seed);
                goto endGetRandomSeed;
        }
        if (read(fd, (void *) &seed, sizeof(ullong)) != sizeof(ullong)) {
                seed = time(NULL);
                Logger(Debug, "seed = time(NULL) = 0x%llx.\n", seed);
        }
        else {
                seed &= 0xffffffff;
                Logger(Debug, "seed = 0x%llx\n", seed);
        }
        if (close(fd) != 0)
                perror("Error while closing /dev/urandom: ");
endGetRandomSeed:
        return seed;
}

ullong GetSeedFromDictionary(string_dict& args)
{
        ullong s;
        if (args.count("seed") == 0 || (s = atoll(args["seed"].c_str())) == -1)
                s = GetRandomSeed();
        return s;
}

bool CheckAndExtractValue(string_dict& dict, const std::string& key, std::string& value)
{
        if (dict.count(key)) {
                value = dict[key];
                return true;
        }
        Logger(Debug, "The required parameter [%s] is missing.\n", key.c_str());
        return false;
}

bool CheckAndExtractDouble(string_dict& dict, const std::string& key, double *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atof(str.c_str());
        return true;
}

bool CheckAndExtractInteger(string_dict& dict, const std::string& key, int *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atoi(str.c_str());
        return true;
}

bool CheckAndExtractLong(string_dict& dict, const std::string& key, long *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atol(str.c_str());
        return true;
}

bool CheckAndExtractLongLong(string_dict& dict, const std::string& key, long long *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        *value = atoll(str.c_str());
        return true;
}

bool CheckAndExtractUnsignedInteger(string_dict& dict, const std::string& key, unsigned int *value)
{
        int i;
        if (!CheckAndExtractInteger(dict, key, &i) || i < 0)
                return false;
        *value = (unsigned int) i;
        return true;
}

bool CheckAndExtractUnsignedLong(string_dict& dict, const std::string& key, unsigned long *value)
{
        long l;
        if (!CheckAndExtractLong(dict, key, &l) || l < 0)
                return false;
        *value = (unsigned long) l;
        return true;
}

bool CheckAndExtractUnsignedLongLong(string_dict& dict, const std::string& key, unsigned long long *value)
{
        long long l;
        if (!CheckAndExtractLongLong(dict, key, &l) || l < 0)
                return false;
        *value = (unsigned long long) l;
        return true;
}

bool CheckAndExtractBool(string_dict& dict, const std::string& key, bool *value)
{
        std::string str;
        if (!CheckAndExtractValue(dict, key, str))
                return false;
        if (boost::iequals(str, "true"))
                *value = true;
        else if (boost::iequals(str, "false"))
                *value = false;
        else
                return false;
        return true;
}

void MakeFilename(char *filename, const char *extension)
{
        time_t rawTime;
        struct tm * timeInfo;
        int extensionLen, cnt;
        char *base;

        extensionLen = strlen(extension);

        base = new char[FILENAME_MAXLEN - extensionLen];

        time(&rawTime);
        timeInfo = localtime(&rawTime);

        sprintf(base, "%d%02d%02d%02d%02d%02d",
                timeInfo->tm_year+1900, timeInfo->tm_mon+1, timeInfo->tm_mday,
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

        sprintf(filename, "%s.%s", base, extension);

        cnt = 1;
        while (fs::exists(filename)) {
                sprintf(filename, "%s-%02d.%s", base, cnt, extension);
                cnt++;
        }

        delete base;
}

void ParseCommandLineOptions(int argc, char *argv[], CommandLineOptions *opt)
{
        SetLoggingLevel(Info);

        double tend, iti, ibi, freq;
        int verbosity;
        std::string configfile, kernelfile, stimfile, stimdir;
        po::options_description description("Allowed options");
        po::variables_map options;

        try {
                char msg[100];
                sprintf(msg, "select verbosity level (%d for maximum, %d for minimum verbosity)", All, Critical);
                description.add_options()
                        ("help,h", "print help message")
                        ("version,v", "print version number")
                        ("verbosity,V", po::value<int>(&verbosity)->default_value(Info), msg)
                        ("config-file,c", po::value<std::string>(&configfile), "specify configuration file")
                        ("kernel-file,k", po::value<std::string>(&kernelfile), "specify kernel file")
                        ("stimulus-file,f", po::value<std::string>(&stimfile), "specify stimulus file")
                        ("stimulus-dir,d", po::value<std::string>(&stimdir), "specify the directory where stimulus files are located")
                        ("time,t", po::value<double>(&tend), "specify the duration of the simulation (in seconds)")
                        ("iti,i", po::value<double>(&iti)->default_value(0.25), "specify inter-trial interval (in seconds)")
                        ("ibi,I", po::value<double>(&ibi)->default_value(0.25), "specify inter-batch interval (in seconds)")
                        ("frequency,F", po::value<double>(&freq), "specify the sampling frequency (in Hertz)")
                        ("ntrials,n", po::value<uint>(&opt->nTrials)->default_value(1), "specify the number of trials (how many times a stimulus is repeated)")
                        ("nbatches,N", po::value<uint>(&opt->nBatches)->default_value(1), "specify the number of trials (how many times a batch of stimuli is repeated)")
                        ("enable-replay,r", po::value<bool>(&opt->enableReplay)->default_value(true), "enable saving of configuration and stimulus files");

                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    

                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }

                if (options.count("version")) {
                        Logger(Info, "%s version %s\n", fs::path(argv[0]).filename().c_str(), VERSION);
                        exit(0);
                }

                if (options.count("frequency")) {
                        if (freq <= 0) {
                                Logger(Critical, "The sampling frequency must be positive.\n");
                                exit(1);
                        }
                        opt->dt = 1.0 / freq;
                }
                else {
                        opt->dt = -1.0;
                }

                if (options.count("config-file")) {
                        if (!fs::exists(configfile)) {
                                Logger(Critical, "Configuration file [%s] not found.\n", configfile.c_str());
                                exit(1);
                        }
                        opt->configFile = configfile;
                }

                if (options.count("kernel-file")) {
                        if (!fs::exists(kernelfile)) {
                                Logger(Critical, "Kernel file [%s] not found.\n", kernelfile.c_str());
                                exit(1);
                        }
                        opt->kernelFile = kernelfile;
                }

                if (options.count("stimulus-file")) {
                        if (!fs::exists(stimfile)) {
                                Logger(Critical, "Stimulus file [%s] not found.\n", stimfile.c_str());
                                exit(1);
                        }
                        opt->stimulusFiles.push_back(stimfile);
                }
                else if (options.count("stimulus-dir")) {
                        if (!fs::exists(stimdir)) {
                                Logger(Critical, "Directory [%s] not found.\n", stimdir.c_str());
                                exit(1);
                        }
                        for (fs::directory_iterator it(stimdir); it != fs::directory_iterator(); it++) {
                                if (! fs::is_directory(it->status())) {
                                        opt->stimulusFiles.push_back(it->path().string());
                                }
                        }
                }

                if (options.count("time"))
                        opt->tend = tend;

                if (options.count("iti"))
                        opt->iti = (useconds_t) (iti * 1e6);

                if (options.count("ibi"))
                        opt->ibi = (useconds_t) (ibi * 1e6);

                if (verbosity < All || verbosity > Critical) {
                        Logger(Important, "The verbosity level must be between %d and %d.\n", All, Critical);
                        exit(1);
                }

                SetLoggingLevel(static_cast<LogLevel>(verbosity));

        }
        catch (std::exception e) {
                Logger(Critical, "Missing argument or unknown option.\n");
                exit(1);
        }
}

bool ParseConfigurationFile(const std::string& filename, std::vector<Entity*>& entities, double *tend, double *dt)
{
        ptree pt;
        uint id;
        std::string name, conn;
        std::map< uint, std::vector<uint> > connections;
        std::map< uint, Entity* > ntts;

        try {
                read_xml(filename, pt);

                /*** simulation time and time step ***/
                try {
                        *tend = pt.get<double>("lcg.simulation.tend");
                } catch(...) {
                        *tend = -1;
                }

                try {
                        *dt = pt.get<double>("lcg.simulation.dt");
                } catch(...) {
                        *dt = -1;
                        try {
                                *dt = 1.0 / pt.get<double>("lcg.simulation.rate");
                        } catch(...) {
                                Logger(Info, "dt = %g sec.\n", *dt);
                        }
                }

                SetGlobalDt(*dt); // So that the entities are loaded with the proper sampling rate.
                SetRunTime(*tend);

                /*** entities ***/
                BOOST_FOREACH(ptree::value_type &ntt, pt.get_child("lcg.entities")) {
                        string_dict args;
                        name = ntt.second.get<std::string>("name");
                        id = ntt.second.get<uint>("id");
                        if (ntts.count(id) == 1) {
                                Logger(Critical, "Duplicate ID in configuration file: [%d].\n", id);
                                entities.clear();
                                return false;
                        }
                        args["id"] = ntt.second.get<std::string>("id");
                        BOOST_FOREACH(ptree::value_type &pars, ntt.second.get_child("parameters")) {
                                if (pars.first.substr(0,12).compare("<xmlcomment>") != 0)
                                        args[pars.first] = std::string(pars.second.data());
                        }
                        try {
                                conn = ntt.second.get<std::string>("connections");
                                // this test allows to have <connections></connections> in the configuration file
                                if (conn.length() > 0) {
                                        connections[id] = std::vector<uint>();
                                        size_t start=0, stop;
                                        int post;
                                        Logger(Debug, "Entity #%d is connected to entities", id);
                                        while ((stop = conn.find(",",start)) != conn.npos) {
                                                std::stringstream ss(conn.substr(start,stop-start));
                                                ss >> post;
                                                connections[id].push_back(post);
                                                start = stop+1;
                                                Logger(Debug, " #%d", post);
                                        }
                                        std::stringstream ss(conn.substr(start,stop-start));
                                        ss >> post;
                                        connections[id].push_back(post);
                                        Logger(Debug, " #%d.\n", post);
                                }
                        } catch(std::exception e) {
                                Logger(Debug, "No connections for entity #%d.\n", id);
                        }
                        Entity *entity;
                        try {
                                entity = EntityFactory(name.c_str(), args);
                                if (entity == NULL)
                                        throw "Entity factory is missing";
                        } catch(const char *err) {
                                Logger(Critical, "Unable to create entity [%s]: %s.\n", name.c_str(), err);
                                for (int i=0; i<entities.size(); i++)
                                        delete entities[i];
                                entities.clear();
                                return false;
                        }
                        entities.push_back(entity);
                        ntts[id] = entity;
                }

                EntitySorter sorter;
                std::sort(entities.begin(), entities.end(), sorter);

                /*** connections ***/
                uint idPre, idPost;
                for (int i=0; i<entities.size(); i++) {
                        idPre = entities[i]->id();
                        Logger(Debug, "Id = %d.\n", idPre);
                        for (int j=0; j<connections[idPre].size(); j++) {
                                idPost = connections[idPre][j];
                                entities[i]->connect(ntts[idPost]);
                                Logger(Debug, "Connecting entity #%d to entity #%d.\n", idPre, idPost);
                        }
                }

        } catch(std::exception e) {
                Logger(Critical, "Error while parsing configuration file: %s.\n", e.what());
                entities.clear();
                return false;
        }

        return true;
}

Entity* EntityFactory(const char *entityName, string_dict& args)
{
        Entity *entity = NULL;
        Factory builder;
        void *library, *addr;
        char symbol[50] = {0};

        library = dlopen(LIBNAME, RTLD_LAZY);
        if (library == NULL) {
                Logger(Critical, "Unable to open library %s.\n", LIBNAME);
                return NULL;
        }
        Logger(Debug, "Successfully opened library %s.\n", LIBNAME);

        sprintf(symbol, "%sFactory", entityName);

        addr = dlsym(library, symbol);
        if (addr == NULL) {
                Logger(Critical, "Unable to find symbol %s.\n", symbol);
                goto close_lib;
        }
        else {
                Logger(Debug, "Successfully found symbol %s.\n", symbol);
        }

        builder = (Factory) addr;
        entity = builder(args);

close_lib:
        if (dlclose(library) == 0) {
                Logger(Debug, "Successfully closed library %s.\n", LIBNAME);
        }
        else {
                Logger(Critical, "Unable to close library %s: %s.\n", LIBNAME, dlerror());
        }

        return entity;
}

bool ConvertUnits(double x, double *y, const std::string& unitsIn, const std::string& unitsOut)
{
        if (boost::equals(unitsIn, unitsOut)) {
                *y = x;
                return true;
        }

        if ((boost::equals(unitsIn, "s") && boost::equals(unitsOut, "Hz")) ||
            (boost::equals(unitsIn, "Hz") && boost::equals(unitsOut, "s"))) {
                *y = 1.0 / x;
                return true;
        }

        return false;
}
} // namespace lcg


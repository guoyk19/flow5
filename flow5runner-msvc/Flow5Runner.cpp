#include <algorithm>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <api.h>
#include <flow5-io.h>
#include <fuse.h>
#include <gmsh.h>
#include <objects2d.h>
#include <objects3d.h>
#include <panelanalysis.h>
#include <planepolar.h>
#include <planetask.h>
#include <planexfl.h>

#ifdef INTEL_MKL
#include <mkl.h>
#endif

namespace fs = std::filesystem;

struct Options
{
    fs::path preload;
    fs::path plane;
    fs::path polar;
    fs::path output;
    double alphaStart = -3.0;
    double alphaEnd = 13.0;
    double alphaStep = 1.0;
    int threads = 1;
    bool stdio = false;
};

static void usage()
{
    std::cout <<
        "Usage:\n"
        "  Flow5Runner --preload PRELOAD.fl5 --plane PLANE.xml --polar POLAR.xml "
        "--output RESULT.csv [options]\n\n"
        "Options:\n"
        "  --alpha-start VALUE   First angle of attack, default -3\n"
        "  --alpha-end VALUE     Last angle of attack, default 13\n"
        "  --alpha-step VALUE    Angle increment, default 1\n"
        "  --threads VALUE       Panel-analysis threads, default 1\n"
        "  --stdio               Print calculation progress\n";
}

static bool requirePath(fs::path const &path, char const *label)
{
    if(path.empty())
    {
        std::cerr << "Missing " << label << "\n";
        return false;
    }
    if(!fs::exists(path))
    {
        std::cerr << label << " does not exist: " << path.string() << "\n";
        return false;
    }
    return true;
}

static bool readDouble(int argc, char **argv, int &i, double &value)
{
    if(i + 1 >= argc)
        return false;
    try
    {
        value = std::stod(argv[++i]);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

static bool readInt(int argc, char **argv, int &i, int &value)
{
    if(i + 1 >= argc)
        return false;
    try
    {
        value = std::stoi(argv[++i]);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

static bool parseOptions(int argc, char **argv, Options &options)
{
    for(int i=1; i<argc; ++i)
    {
        std::string arg = argv[i];
        auto nextPath = [&](fs::path &path) -> bool
        {
            if(i + 1 >= argc) return false;
            path = argv[++i];
            return true;
        };

        if(arg == "--help" || arg == "-h")
        {
            usage();
            return false;
        }
        if(arg == "--preload" && nextPath(options.preload)) continue;
        if(arg == "--plane" && nextPath(options.plane)) continue;
        if(arg == "--polar" && nextPath(options.polar)) continue;
        if(arg == "--output" && nextPath(options.output)) continue;
        if(arg == "--alpha-start" && readDouble(argc, argv, i, options.alphaStart)) continue;
        if(arg == "--alpha-end" && readDouble(argc, argv, i, options.alphaEnd)) continue;
        if(arg == "--alpha-step" && readDouble(argc, argv, i, options.alphaStep)) continue;
        if(arg == "--threads" && readInt(argc, argv, i, options.threads)) continue;
        if(arg == "--stdio")
        {
            options.stdio = true;
            continue;
        }

        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return false;
    }

    if(options.alphaStep <= 0.0 || options.alphaEnd < options.alphaStart || options.threads <= 0)
    {
        std::cerr << "Invalid alpha range or thread count\n";
        return false;
    }
    return requirePath(options.preload, "preload project")
        && requirePath(options.plane, "plane XML")
        && requirePath(options.polar, "polar XML");
}

static std::vector<double> makeAlphaList(Options const &options)
{
    std::vector<double> values;
    for(double alpha=options.alphaStart; alpha<=options.alphaEnd + 0.5*options.alphaStep; alpha+=options.alphaStep)
        values.push_back(alpha);
    return values;
}

int main(int argc, char **argv)
{
    Options options;
    if(!parseOptions(argc, argv, options))
        return argc > 1 ? 2 : 0;

#ifdef INTEL_MKL
    mkl_set_dynamic(0);
#endif

    bool gmshInitialized = false;
    try
    {
        std::string logmsg;
        if(!io::loadProject(options.preload.string(), logmsg))
        {
            std::cerr << "Could not load preload project:\n" << logmsg << "\n";
            return 3;
        }

        std::cout << std::format("Loaded {} foil(s) and {} 2D polar(s)\n",
                                 Objects2d::nFoils(), Objects2d::nPolars());
        if(!Objects2d::foil("NACA 0015"))
        {
            std::cerr << "Preload project does not contain foil 'NACA 0015'\n";
            globals::deleteObjects();
            return 4;
        }

        // Gmsh must be initialized after MKL to avoid the threading conflict
        // documented by the original PlaneRun4 example.
        gmsh::initialize();
        gmshInitialized = true;
        gmsh::option::setNumber("General.Terminal", options.stdio ? 1 : 0);
        gmsh::option::setNumber("Geometry.OCCParallel", 1.0);
        gmsh::option::setNumber("General.NumThreads", 0);

        PlaneXfl *plane = io::importPlaneFromXML(options.plane.string(), logmsg);
        if(!plane)
        {
            std::cerr << "Could not import plane XML:\n" << logmsg << "\n";
            globals::deleteObjects();
            gmsh::finalize();
            return 5;
        }
        Objects3d::insertPlane(plane);

        // Build the triangular meshes required by TRIUNIFORM analysis.
        plane->makePlane(false, false, true);

        PlanePolar *polar = io::importAnalysisFromXML(options.polar.string(), logmsg);
        if(!polar)
        {
            std::cerr << "Could not import polar XML:\n" << logmsg << "\n";
            globals::deleteObjects();
            gmsh::finalize();
            return 6;
        }
        polar->setPlaneName(plane->name());
        Objects3d::insertPlanePolar(polar);

        // The XML files supplied for this batch use precomputed XFoil data.
        // Do not switch this to setViscous(false): that would change the model.
        polar->setViscous(true);
        polar->setViscOnTheFly(false);

        PanelAnalysis::setMaxThreadCount(options.threads);
        PlaneTask task;
        task.outputToStdIO(options.stdio);
        task.setKeepOpps(true);
        task.setObjects(plane, polar);
        task.setComputeDerivatives(false);
        task.setOppList(makeAlphaList(options));
        task.run();

        if(!options.output.empty())
        {
            if(!options.output.parent_path().empty())
                fs::create_directories(options.output.parent_path());
            std::ofstream out(options.output, std::ios::binary);
            if(!out)
            {
                std::cerr << "Could not open output file: " << options.output.string() << "\n";
                globals::deleteObjects();
                gmsh::finalize();
                return 7;
            }
            out << polar->exportToString(",");
        }

        std::cout << std::format("Computed {} operating point(s)\n", task.planeOppList().size());
        if(task.hasErrors())
            std::cerr << "flow5 reported calculation errors\n";
        if(task.hasWarning())
            std::cerr << "flow5 reported calculation warnings\n";

        bool failed = task.hasErrors();
        globals::deleteObjects();
        gmsh::finalize();
        return failed ? 8 : 0;
    }
    catch(std::exception const &error)
    {
        std::cerr << "Unhandled exception: " << error.what() << "\n";
        globals::deleteObjects();
        if(gmshInitialized)
            gmsh::finalize();
        return 9;
    }
}

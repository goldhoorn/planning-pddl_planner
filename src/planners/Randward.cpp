#include <pddl_planner/planners/Randward.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <boost/filesystem.hpp>
#include <base/logging.h>
#include <base/time.h>
#include <list>

namespace fs = boost::filesystem;

namespace pddl_planner
{
namespace randward
{

const std::string Planner::msDomainFileBasename = "domain.pddl";
const std::string Planner::msProblemFileBasename = "problem.pddl";
const std::string Planner::msResultFileBasename = "plan";
const std::string Planner::msTempDirBasename = "/tmp";

PlanCandidates Planner::plan(const std::string& problem, const std::string& actionDescriptions, const std::string& domainDescriptions, double timeout)
{
    LOG_DEBUG("Planner called with problem: '%s'", problem.c_str());

    int result = system("which randward-planner");
    if(result != 0)
    {
        std::string msg = "Could not find 'randward-planner' script";
        LOG_ERROR("%s",msg.c_str());
        throw PlanGenerationException(msg);
    }

    std::string currentTime = base::Time::now().toString();
    fs::path path(msTempDirBasename + "/" + currentTime + "_randward");

    if(!fs::exists(path))
    {
        if (!fs::create_directory(path))
        {
            LOG_ERROR("Could not create directory: %s", path.string().c_str());
        }
    }
    mTempDir = path.string();
    mTimeout = timeout;
    prepare(problem, actionDescriptions, domainDescriptions);
    PlanCandidates planCandidates = generatePlanCandidates();
    return planCandidates;
}

void Planner::prepare(const std::string& problem, const std::string& actionDescriptions, const std::string& domainDescriptions)
{
    mDomainFilename = mTempDir + "/" + msDomainFileBasename;
    std::ofstream out(mDomainFilename.c_str());

    out << domainDescriptions;
    out << "\n";
    out << actionDescriptions;

    out.close();

    mProblemFilename = mTempDir + "/" + msProblemFileBasename;
    std::ofstream problemOut(mProblemFilename.c_str());
    LOG_DEBUG("Prepare problem '%s'", problem.c_str());
    problemOut << problem;
    problemOut << "\n";
    problemOut.close();

    mResultFilename = mTempDir + "/" + msResultFileBasename;
}

PlanCandidates Planner::generatePlanCandidates()
{
    std::string cmd = "randward-planner " + mDomainFilename + " " + mProblemFilename + " " + mResultFilename;
    
    PlanCandidates planCandidates = generateCandidates(cmd, mTempDir, mResultFilename, getName(), mTimeout);
    
    std::list<std::string> files;
    files.push_back(std::string("output"));
    files.push_back(std::string("output.sas"));
    files.push_back(std::string("all.groups"));
    files.push_back(std::string("test.groups"));
    
    cleanup(mTempDir, files);
    return planCandidates;
}

}

}

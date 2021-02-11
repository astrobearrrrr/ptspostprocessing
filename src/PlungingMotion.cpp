#include<fstream>
#include<map>
#include<cmath>
#include<algorithm>
#include<set>
#include "Util.h"
#include "PlungingMotion.h"

PlungingMotion::PlungingMotion(std::string dataconfigue) {
    std::ifstream conf(dataconfigue.c_str());
    if(!conf.is_open()) {
        printf("error: unable to open configue file %s\n", dataconfigue.c_str());
    }
    char buffer[1000];
    std::map<std::string, std::string> param;
    while(!conf.eof()) {
        conf.getline(buffer, sizeof(buffer));
        std::vector<std::string> p;
        parserString(buffer, p);
        if(p.size()>1) {
            param[p[0]] = p[1];
        }
    }
    if(param.count("k")) {
        m_k = StringToDouble(param["k"]);
    } else {
        m_k = 0.;
    }
    if(param.count("A")) {
        m_A = StringToDouble(param["A"]);
    } else {
        m_A = 0.;
    }
    if(param.count("phi")) {
        m_phi = StringToDouble(param["phi"]);
    } else {
        m_phi = 0.;
    }
    if(param.count("input")) {
        m_inputformat = param["input"];
    } else {
        m_inputformat = "unsetinputfile%d";
    }
    if(param.count("output")) {
        m_outputformat = param["output"];
    } else {
        m_outputformat = "unsetoutputfile%d";
    }
    if(param.count("filesnumber")) {
        parserInt(param["filesnumber"].c_str(), m_file);
    } else {
        m_file.resize(3, 0);
    }
    GenerateFileSeries();
    if(param.count("phase")) {
        parserDouble(param["phase"].c_str(), m_phase);
    }
    if(param.count("body")) {
        m_airfoil = param["body"];
    } else {
        m_airfoil = "0000";
    }
    if(param.count("AoA")) {
        m_AoA = StringToDouble(param["AoA"].c_str());
        m_AoA = m_AoA / 180. * M_PI;
    } else {
        m_AoA = 0.;
    }
    if(param.count("span")) {
        parserDouble(param["span"].c_str(), m_span);
    } else {
        m_span.push_back(std::numeric_limits<double>::min());
        m_span.push_back(std::numeric_limits<double>::max());
    }
    if(param.count("threshold")) {
        m_threshold = StringToDouble(param["threshold"].c_str());
    } else {
        m_threshold = 0.;
    }
    if(param.count("vortexcorevar")) {
        parserInt(param["vortexcorevar"].c_str(), m_vortexcoreVar);
    } else {
        m_vortexcoreVar = std::vector<int>(4,0);
    }
    if(param.count("stoponwall")) {
        m_stoponwall = myRound<double>(StringToDouble(param["stoponwall"].c_str()));
    } else {
        m_stoponwall = 0;
    }
    if(param.count("translation")) {
        m_translation = myRound<double>(StringToDouble(param["translation"].c_str()));
    } else {
        m_translation = 0;
    }
    if(param.count("calculateVorticityQ")) {
        m_calculateVorticityQ = myRound<double>(StringToDouble(param["calculateVorticityQ"].c_str()));
    } else {
        m_calculateVorticityQ = 0;
    }
    if(param.count("N")) {
        parserUInt(param["N"].c_str(), m_N);
    }
    if(param.count("range")) {
        parserDouble(param["range"].c_str(), m_range);
    }
    if(param.count("sigma")) {
        parserDouble(param["sigma"].c_str(), m_sigma);
    } else {
        m_sigma = {-1.};
    }
    if(param.count("initcenter")) {
        parserDouble(param["initcenter"].c_str(), m_initcenter);
    }
}

double PlungingMotion::GetFilePhase(int n) {
    return m_phase[0] + m_phase[1] * (n - m_file[0]) / m_file[1];
}

std::string PlungingMotion::GetInFileName(int n) {
    char buffer[100];
    sprintf(buffer, m_inputformat.c_str(), n);
    std::string res(buffer);
    return res;
}

std::string PlungingMotion::GetOutFileName(int n) {
    char buffer[100];
    sprintf(buffer, m_outputformat.c_str(), n);
    std::string res(buffer);
    return res;
}

double PlungingMotion::PlungingVelocity(double phase, double phi) {
    return -m_k*m_A*sin(phase*2.*M_PI + phi);
}

double PlungingMotion::PlungingLocation(double phase, double phi) {
    return 0.5*m_A*cos(phase*2.*M_PI + phi);
    //0.5 A cos(2 k t + phi)
}

int PlungingMotion::Dumppoints() {
    int count  = 0;
    for(auto f=m_fileseries.begin(); f!=m_fileseries.end(); ++f) {
        IncFlow flow(m_N, m_range, m_airfoil, {m_AoA});
        flow.OutputData(GetOutFileName(*f));
        ++count;
    }
    return count;
}

int PlungingMotion::GenerateFileSeries() {
    m_fileseries.clear();
    if(m_file.size()<3 || m_file[1]==0) {
        return 0;
    }
    if(m_file[1]>0) {
        for(int i=m_file[0]; i<m_file[2]; i+=m_file[1]) {
            m_fileseries.push_back(i);
        }
    } else {
        for(int i=m_file[0]; i>m_file[2]; i+=m_file[1]) {
            m_fileseries.push_back(i);
        }
    }
    return m_fileseries.size();
}

int PlungingMotion::TransformBathCoord(IncFlow &flow) {
    // after = before
    // y = z
    // x = -x
    // z = 5 - y
    std::clock_t c_start = std::clock();
    int Np = flow.GetTotPoints();
    for(int i=0; i<Np; ++i) {
        double x = flow.GetCoordValue(0, i);
        double y = flow.GetCoordValue(1, i);
        double u = flow.GetPhysValue(0, i);
        double v = flow.GetPhysValue(1, i);
        flow.SetCoordValue(-x, 0, i);
        flow.SetCoordValue(5.-y, 1, i);
        flow.SetPhysValue(-u, 0, i);
        flow.SetPhysValue(-v, 1, i);
    }
    std::map<int, int> vm = {{2, 1}, {1, 2}, {0,0}};
    std::vector<int> dir = {-1, -1, 1};
    std::map<int, int> pm = {{0,0},{1,2},{2,1},{3,3}};
    flow.ShuffleIndex(vm, dir, pm);
    std::vector<int> N = flow.GetN();
    for(size_t i=0; i<N.size(); ++i) {
        N[i] = N[i]*4 - 3;
    }
    IncFlow flow2(N, flow.GetRange());
    std::map<int, double> field;
    for(int i=0; i<flow.GetNumPhys(); ++i) {
        field[i] = 0.;
    }
    flow2.InterpolateFrom(flow, field);
    flow = flow2;
    std::clock_t c_end = std::clock();
    double time_elapsed_ms = (c_end-c_start) * 1. / CLOCKS_PER_SEC;
    printf("transform Bath exp data, cpu time %fs\n", time_elapsed_ms);
    return 3*Np;
}

int PlungingMotion::ProcessEXPWingData(int dir) {
    std::vector<int> filen = m_fileseries;
    if(dir<0) {
        std::reverse(filen.begin(), filen.end());;
    }
    for(int k=0; k<(int)filen.size(); ++k) {
        int n = filen[k];
        IncFlow flow(m_N, m_range, m_airfoil, {m_AoA, m_span[0], m_span[1]});
        flow.InputData(GetInFileName(n));
        if(m_translation) {
            double h0 = PlungingLocation(GetFilePhase(n), m_phi) - 0.5*m_A;
            flow.TransformCoord({0., h0, 0.});
        }
        TransformBathCoord(flow);
        ProcessFiniteWingData(flow, k);
    }
    return m_fileseries.size();
}

int PlungingMotion::ProcessCFDWingData(int dir) {
    std::vector<int> filen = m_fileseries;
    if(dir<0) {
        std::reverse(filen.begin(), filen.end());;
    }
    for(int k=0; k<(int)filen.size(); ++k) {
        int n = filen[k];
        IncFlow flow(m_N, m_range, m_airfoil, {m_AoA, m_span[0], m_span[1]});
        flow.InputData(GetInFileName(n));
        if(m_airfoil.compare("0000")!=0) {
            double v0 = PlungingVelocity(GetFilePhase(n), m_phi);
            flow.OverWriteBodyPoint({0., v0, 0.}, {0., 0., 0.}, {0., 0., 0.});
        }
        if(m_translation) {
            double h0 = PlungingLocation(GetFilePhase(n), m_phi);
            flow.TransformCoord({0., h0, 0.});
        }
        ProcessFiniteWingData(flow, n);
    }
    return m_fileseries.size();
}

int PlungingMotion::ProcessVortexCore(IncFlow &flow, int n, double sigma,
        std::vector<std::vector<double> > &cores) {
    ProcessSmoothing(flow, sigma);
    if(m_calculateVorticityQ) {
        ProcessVorticity(flow);
    }
    std::clock_t c_start = std::clock();
    if(m_vortexcoreVar.size()<4) {
        printf("error incorrect vortex variables.\n");
        return -1;
    }
    for(int i=0; i<4; ++i) {
        if(m_vortexcoreVar[i]<=0 || m_vortexcoreVar[i]>flow.GetNumPhys()) {
            printf("error incorrect vortex variables.\n");
            return -1;
        }
    }
    std::string filename("vcore");
    std::string fname = GetOutFileName(n);
    if(cores.size()==0) {
        std::set<int> searchhist;
        flow.ExtractCoreByPoint(cores, searchhist, m_initcenter, m_vortexcoreVar,
            m_vortexcoreVar[3], m_stoponwall>0, m_threshold, sigma);
        filename += fname.substr(0, (int)fname.size()-4) + ".dat";
    } else {
        flow.RefineCore(cores, m_vortexcoreVar, m_vortexcoreVar[3]);
        filename += fname.substr(0, (int)fname.size()-4) + "_" + std::to_string(cores.size()) + ".dat";
    }
    std::ofstream ofile(filename.c_str());
    ofile << "variables = \"x\",\"y\",\"z\",\"radius1\",\"radius2\",\"Gamma\"";
    for(size_t i=0; i<m_vortexcoreVar.size(); ++i) {
        ofile << ",\"" << flow.GetPhysVarName(m_vortexcoreVar[i]-1) << "\"";
    }
    ofile << "\n";
    for(size_t i=0; i<cores.size(); ++i) {
        for(size_t j=0; j<cores[i].size(); ++j) {
            ofile << cores[i][j] << " ";
        }
        ofile << "\n";
    }
    ofile.close();
    std::clock_t c_end = std::clock();
    double time_elapsed_ms = (c_end-c_start) * 1. / CLOCKS_PER_SEC;
    if(cores.size()==0) {
        printf("no vortex core found with threshold %f, cpu time %fs\n", m_threshold, time_elapsed_ms);
        return 0;
    } else {
        printf("vortex core file %s, cpu time %fs\n", filename.c_str(), time_elapsed_ms);
    }
    return (int)cores.size();
}

int PlungingMotion::ProcessFiniteWingData(IncFlow &flow, int n) {
    std::vector<std::vector<double> > cores;
    IncFlow rawdata = flow; // backed raw data;
    ProcessVortexCore(flow, n, m_sigma[0], cores);
    flow.OutputData(GetOutFileName(n));
    if(m_sigma.size()>1) {
        ProcessVortexCore(rawdata, n, m_sigma[1], cores);
        rawdata.OutputData("refine_" + GetOutFileName(n));
    }
    return cores.size();
}

int PlungingMotion::ProcessSmoothing(IncFlow &flow, double sigma) {
    if(sigma>0.) {
        std::clock_t c_start = std::clock();
        std::vector<int> field;
        for(int i=0; i<flow.GetNumPhys(); ++i) {
            field.push_back(i);
        }
        flow.Smoothing(sigma, field);
        std::clock_t c_end = std::clock();
        double time_elapsed_ms = (c_end-c_start) * 1. / CLOCKS_PER_SEC;
        printf("do smooth, cpu time %fs\n", time_elapsed_ms);
    }
    return 1;
}

int PlungingMotion::ProcessVorticity(IncFlow &flow) {
    //vorticity Q
    //u, v, w, p, W_x, W_y, W_z, Q
    
    std::clock_t c_start = std::clock();
    flow.CalculateVorticity();
    std::clock_t c_end = std::clock();
    double time_elapsed_ms = (c_end-c_start) * 1. / CLOCKS_PER_SEC;
    printf("calculate vorticity, cpu time %fs\n", time_elapsed_ms);
    return 1;
}
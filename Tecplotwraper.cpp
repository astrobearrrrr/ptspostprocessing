#include<iostream>
#include "Tecplotwraper.h"

int OutputTec360_ascii(const std::string filename, const std::vector<std::string> &variables,
                 const std::vector<int> &N, const std::vector<std::vector<double> > data,
                 int isdouble,
                 int debug,
                 int filetype,
                 int fileformat)
{
    std::string varlist = variables[0];
    for(int i=1; i<variables.size(); ++i) {
        varlist += "," + variables[i];
    }
    std::ofstream ofile(filename.c_str());
    ofile << "variables = " << varlist << std::endl;
    ofile << "zone I = " << N[0] << ", J = " << N[1] << ", K = " << N[2] << std::endl;
    int Np = N[0] * N[1] * N[2];
    for(int i=0; i<Np; ++i) {
        for(int j=0; j<data.size(); ++j) {
            ofile << data[j][i] << " ";
        }
        ofile << "\n";
    }
    ofile.close();
    return 0;
}

int BinaryWrite(std::ofstream &ofile, std::string str) {
    int tmp = 0;
    for(int i=0; i<str.size(); ++i) {
        tmp = str[i];
        ofile.write((char*)&tmp, 4);
    }
    ofile.write((char*)&tmp, 4);
}

int OutputTec360_binary(const std::string filename, const std::vector<std::string> &variables,
                 const std::vector<int> &N, const std::vector<std::vector<double> > data,
                 int isdouble,
                 int debug,
                 int filetype,
                 int fileformat)
{
    std::ofstream odata;
    odata.open(filename, std::ios::binary);
	if(!odata.is_open())
	{
        printf("error unable to open file %s\n", filename.c_str());
		return -1;
	}
    char tecplotversion[] = "#!TDV112";
	odata.write((char*)tecplotversion, 8);
    int value1 = 1;
	odata.write((char*)&value1, 4);
    int filetype = 0;
	odata.write((char*)&filetype, 4);
	//read file title and variable names
	int tempi = 0;
    std::string filetitle = "";
    BinaryWrite(odata, filetitle);
    int nvar = variables.size();
    odata.write((char*)&nvar, 4);//number of variables
    std::vector<std::string> vartitle;
	for(int i=0; i<nvar; i++)
	{
        BinaryWrite(odata, variables[i]);
	}
    float marker299I = 299.0f;
	odata.write((char*)&marker299I, 4);
	//zone title
    std::string zonetitle("ZONE 0");
	BinaryWrite(odata, zonetitle);
    int parentzone = -1;
	odata.write((char*)&parentzone, 4);
    int strandid = -1;
	odata.write((char*)&strandid, 4);
    double soltime = 0.0;
    odata.write((char*)&soltime, 8);
    int unused = -1;
    odata.write((char*)&unused, 4);
    int zonetype = 0;
    odata.write((char*)&zonetype, 4);
    int zero = 0;
    odata.write((char*)&zero, 4);
    odata.write((char*)&zero, 4);
    odata.write((char*)&zero, 4);
	for(int i=0; i<3; ++i) {
        int tmp = N[i];
        odata.write((char*)&tmp, 4);
    }

    odata.write((char*)&zero, 4);
    float marker357 = 357.0f;
	odata.write((char*)&marker357, 4);
    float marker299II = 299.0f;
	odata.write((char*)&marker299II, 4);
	std::vector<int> binarydatatype(nvar, 2);
	odata.write((char*)binarydatatype.data(), 4*nvar);
    odata.write((char*)&zero, 4);
    odata.write((char*)&zero, 4);
    int minus1 = -1;
    odata.write((char*)&minus1, 4);

    for(int i=0; i<nvar; ++i) {
        double minv = 0., maxv=1.;
        odata.write((char*)&minv, 8);
        odata.write((char*)&maxv, 8);
    }

    int datanumber, datasize;
	datanumber = N[0] * N[1] * N[2];
	datasize = N[0] * N[1] * N[2] * 8;
    std::vector<float> vardata(datanumber);
    for(int i=0; i<nvar; ++i) {
        odata.write((char*)data[i].data(), datasize);
    }
    return 0;
}

int InputTec360_binary(const std::string filename, std::vector<std::string> &variables,
                 std::vector<int> &N, std::vector<std::vector<double> > data,
                 int &isdouble,
                 int debug,
                 int filetype,
                 int fileformat) {
	std::ifstream indata;
    indata.open(filename, std::ios::binary);
	if(!indata.is_open())
	{
        printf("error unable to open file %s\n", filename.c_str());
		return -1;
	}
    char tecplotversion[8];
	indata.read((char*)tecplotversion, 8);
    int value1;
	indata.read((char*)&value1, 4);
    int filetype;
	indata.read((char*)&filetype, 4);
	//read file title and variable names
	int tempi;
    std::string filetitle;
	while(1)
	{
        indata.read((char*)&tempi, 4);
        if(tempi==0) break;
        char c = tempi;
        filetitle.push_back(c);
	}
    int nvar;
    indata.read((char*)&nvar, 4);//number of variables
    variables.clear();
	for(int i=0; i<nvar; i++)
	{
        std::string vname;
        while(1)
        {
            indata.read((char*)&tempi, 4);
            if(tempi==0) break;
            char c = tempi;
            vname.push_back(c);
        }
        variables.push_back(vname);
	}
    float marker299I;
	indata.read((char*)&marker299I, 4);
	if(marker299I!=299.)
	{
		printf("error in reading file %s\n", filename.c_str());
		return -1;
	}
	//zone title
    std::string zonentitle;
	while(1)
	{
        indata.read((char*)&tempi, 4);
        if(tempi==0) break;
        char c = tempi;
        zonentitle.push_back(c);
	}
    int parentzone;
	indata.read((char*)&parentzone, 4);
    int strandid;
	indata.read((char*)&strandid, 4);
    double soltime;
    indata.read((char*)&soltime, 8);
    int unused;
    indata.read((char*)&unused, 4);
    int zonetype;
    indata.read((char*)&zonetype, 4);
    int zero;
    indata.read((char*)&zero, 4);
    indata.read((char*)&zero, 4);
    indata.read((char*)&zero, 4);
    N.resize(3);
	indata.read((char*)N.data(), 3*4);
    indata.read((char*)&zero, 4);
    float marker357;
	indata.read((char*)&marker357, 4);
    float marker299II;
	indata.read((char*)&marker299II, 4);
	if(marker357!=357.||marker299II!=299.)
	{
		printf("error in reading file %s\n", filename.c_str());
		return -1;
	}
	int * binarydatatype = new int[nvar];
	indata.read((char*)binarydatatype, 4*nvar);
    isdouble = binarydatatype[0] == 2;
    indata.read((char*)&zero, 4);
    indata.read((char*)&zero, 4);
    int minus1;
    indata.read((char*)&minus1, 4);

    for(int i=0; i<nvar; ++i) {
        double minv, maxv;
        indata.read((char*)&minv, 8);
        indata.read((char*)&maxv, 8);
    }

    int datanumber, datasize;
	datanumber = N[0] * N[1] * N[2];
	datasize = N[0] * N[1] * N[2] * 4;
    for(int i=0; i<nvar; ++i) {
        if(isdouble) {
            std::vector<double> vardata(datanumber);
            indata.read((char*)vardata.data(), datasize * 2);
            data.push_back(vardata);
        } else {
            std::vector<float> vardata(datanumber);
            std::vector<double> doublevardata(datanumber);
            indata.read((char*)vardata.data(), datasize);
            for(int i=0; i<datanumber; ++i) {
                doublevardata[i] = vardata[i];
            }
            data.push_back(doublevardata);
        }
    }
    return 0;
}